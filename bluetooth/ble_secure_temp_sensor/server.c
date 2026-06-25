/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "temp_sensor.h"
#include <string.h>
#include <inttypes.h>

#ifndef NDEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif
#define INFO_LOG printf
#define ERROR_LOG printf

#define HEARTBEAT_PERIOD_MS 1000
#define ADC_CHANNEL_TEMPSENSOR 4

#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

int le_notification_enabled;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint16_t current_temp;

extern uint8_t const profile_data[];
static void poll_temp(void);

static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// Select a security setting to explore the BLE security. See README.md for details
#ifndef SECURITY_SETTING
#error define SECURITY_SETTING
#endif

static int choose_security(int setting) {
    printf("Choose security in the next 5s (default %d)\n", setting);
    printf("0: IO_CAPABILITY_NO_INPUT_NO_OUTPUT\n");
    printf("1: IO_CAPABILITY_DISPLAY_YES_NO and SM_AUTHREQ_MITM_PROTECTION\n");
    printf("2: IO_CAPABILITY_KEYBOARD_DISPLAY and SM_AUTHREQ_MITM_PROTECTION\n");
    printf("3: IO_CAPABILITY_DISPLAY_ONLY and SM_AUTHREQ_MITM_PROTECTION\n");
    printf("all are using SM_AUTHREQ_SECURE_CONNECTION\n");

    int c = getchar_timeout_us(5000000);
    if (c >= 0) {
        if (c >= '0' && c <= '3') {
            setting = c - '0';
        } else {
            printf("Invalid input\n");
        }
    }
    printf("Using security setting %d\n", setting);

    return setting;
}

static void configure_security(int security_setting) {
    DEBUG_LOG("Security setting %u selected.\n", security_setting);
    sm_set_secure_connections_only_mode(true);
    switch (security_setting) {
        case 0:
            sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
            // Note: SM_AUTHREQ_BONDING is deliberately omitted. This means keys
            // are not saved to persistent storage and the device re-pairs on every
            // connection. This keeps the example simple and avoids flash wear and
            // stale bonding issues during testing. Add SM_AUTHREQ_BONDING here and
            // in the client if you want to persist bonds across power cycles.
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION);
            break;
        case 1:
            sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_YES_NO);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        case 2:
            sm_set_io_capabilities(IO_CAPABILITY_KEYBOARD_DISPLAY);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        case 3:
            sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_ONLY);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        default:
            assert(false);
            ERROR_LOG("invalid security setting %u", security_setting);
            break;
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            DEBUG_LOG("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

            // setup advertisements
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31); // ble limitation
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);

            poll_temp();

            break;
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE) {
                con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                INFO_LOG("Connection complete, con_handle=0x%04x\n", con_handle);
                sm_request_pairing(con_handle);
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            con_handle = HCI_CON_HANDLE_INVALID;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            INFO_LOG("Sending notification: temp=%d\n", current_temp);
            att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, (uint8_t*)&current_temp, sizeof(current_temp));
            break;
        default:
            break;
    }
}

static uint32_t passkey_result;
static uint8_t passkey_type;
static hci_con_handle_t passkey_handle;

// Send passkey result to BTStack
static void passkey_done_fn(__unused void * arg) {
    switch (passkey_type) {
        case SM_EVENT_PASSKEY_INPUT_NUMBER: {
            DEBUG_LOG("SM_EVENT_PASSKEY_INPUT_NUMBER %u\n", passkey_result);
            sm_passkey_input(passkey_handle, passkey_result);
            break;
        }
        default: {
            assert(false); // should not happen!
            ERROR_LOG("passkey_done_fn: Unexpected passkey type %u\n", passkey_type);
            break;
        }
    }
}
static btstack_context_callback_registration_t passkey_done = { .callback = passkey_done_fn };

// Handle a key press
static void passkey_press_callback(__unused void *user_data) {
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    switch (passkey_type) {
        case SM_EVENT_PASSKEY_INPUT_NUMBER: {
            if (key == 13) {
                // See above comment
                INFO_LOG("\n");
                btstack_run_loop_execute_on_main_thread(&passkey_done);
            } else if (key >= '0' && key <= '9') {
                INFO_LOG("%c", key);
                passkey_result = (passkey_result * 10) + (key - '0');
            }
            break;
        }
        default: {
            assert(false); // should not happen!
            ERROR_LOG("passkey_callback: Unexpected passkey type %u\n", passkey_type);
            break;
        }
    }
}

static void sm_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint8_t status;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_META_GAP:
            // con_handle capture and sm_request_pairing handled in packet_handler
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            INFO_LOG("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            // Server always confirms the comparison
            INFO_LOG("Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            INFO_LOG("Displaying passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            INFO_LOG("Enter the passkey shown by the client:\n");
            // We want to "wait" for user input but we must not block,
            // so use a stdio callback to tell us when characters are available
            passkey_result = 0;
            passkey_type = SM_EVENT_PASSKEY_INPUT_NUMBER;
            passkey_handle = sm_event_passkey_input_number_get_handle(packet);
            stdio_set_chars_available_callback(passkey_press_callback, NULL);
            break;
        case SM_EVENT_IDENTITY_CREATED:
            sm_event_identity_created_get_identity_address(packet, addr);
            DEBUG_LOG("Identity created: type %u address %s\n", sm_event_identity_created_get_identity_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
            sm_event_identity_resolving_succeeded_get_identity_address(packet, addr);
            DEBUG_LOG("Identity resolved: type %u address %s\n", sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED:
            sm_event_identity_created_get_address(packet, addr);
            DEBUG_LOG("Identity resolving failed\n");
            break;
        case SM_EVENT_PAIRING_STARTED:
            DEBUG_LOG("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    DEBUG_LOG("Pairing complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    ERROR_LOG("Pairing failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    ERROR_LOG("Pairing failed, disconnected\n");
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    ERROR_LOG("Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                    break;
                default:
                    assert(false);
                    ERROR_LOG("Unexpected pairing status %d\n", sm_event_pairing_complete_get_status(packet));
                    break;
            }
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_LOG("Bonding information exists for addr type %u, identity addr %s -> re-encryption started\n",
                   sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            switch (sm_event_reencryption_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    DEBUG_LOG("Re-encryption complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    ERROR_LOG("Re-encryption failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    ERROR_LOG("Re-encryption failed, disconnected\n");
                    break;
                case ERROR_CODE_PIN_OR_KEY_MISSING:
                    ERROR_LOG("Re-encryption failed, bonding information missing\n\n");
                    ERROR_LOG("Assuming remote lost bonding information\n");
                    ERROR_LOG("Deleting local bonding information to allow for new pairing...\n");
                    sm_event_reencryption_complete_get_address(packet, addr);
                    addr_type = sm_event_reencryption_started_get_addr_type(packet);
                    gap_delete_bonding(addr_type, addr);
                    break;
                default:
                    break;
            }
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            status = gatt_event_query_complete_get_att_status(packet);
            switch (status){
                case ATT_ERROR_INSUFFICIENT_ENCRYPTION:
                    ERROR_LOG("GATT Query failed, Insufficient Encryption\n");
                    break;
                case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                    ERROR_LOG("GATT Query failed, Insufficient Authentication\n");
                    break;
                case ATT_ERROR_BONDING_INFORMATION_MISSING:
                    ERROR_LOG("GATT Query failed, Bonding Information Missing\n");
                    break;
                case ATT_ERROR_SUCCESS:
                    DEBUG_LOG("GATT Query successful\n");
                    break;
                default:
                    assert(false);
                    ERROR_LOG("Unexpected GATT Query failed, status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                    break;
            }
            break;
        default:
            break;
    }
}

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&current_temp, sizeof(current_temp), offset, buffer, buffer_size);
    }
    return 0;
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);

    INFO_LOG("ATT write: handle=0x%04x value=0x%04x\n", att_handle, little_endian_read_16(buffer, 0));
    if (att_handle != ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_CLIENT_CONFIGURATION_HANDLE) return 0;
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    INFO_LOG("Notifications %s\n", le_notification_enabled ? "enabled" : "disabled");
    if (le_notification_enabled) {
        att_server_request_can_send_now_event(con_handle);
    }
    return 0;
}

static void poll_temp(void) {
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    uint32_t raw32 = adc_read();
    const uint32_t bits = 12;

    // Scale raw reading to 16 bit value using a Taylor expansion (for 8 <= bits <= 16)
    uint16_t raw16 = raw32 << (16 - bits) | raw32 >> (2 * bits - 16);

    // ref https://github.com/raspberrypi/pico-micropython-examples/blob/master/adc/temperature.py
    const float conversion_factor = 3.3 / (65535);
    float reading = raw16 * conversion_factor;
    
    // The temperature sensor measures the Vbe voltage of a biased bipolar diode, connected to the fifth ADC channel
    // Typically, Vbe = 0.706V at 27 degrees C, with a slope of -1.721mV (0.001721) per degree. 
    float deg_c = 27 - (reading - 0.706) / 0.001721;
    current_temp = deg_c * 100;
    INFO_LOG("Write temp %.2f degc\n", deg_c);
 }

static void heartbeat_handler(struct btstack_timer_source *ts) {
    static uint32_t counter = 0;
    counter++;

    // Update the temp every 10s
    if (counter % 10 == 0) {
        poll_temp();
        INFO_LOG("Heartbeat: le_notification_enabled=%d con_handle=0x%04x\n", le_notification_enabled, con_handle);
        if (le_notification_enabled) {
            att_server_request_can_send_now_event(con_handle);
        }
    }

    // Invert the led
    static int led_on = true;
    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

    // Restart timer
    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

int main() {
    stdio_init_all();

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        ERROR_LOG("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Initialise adc for the temp sensor
    adc_init();
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    adc_set_temp_sensor_enabled(true);

    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // sm packet handler
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // apply security configuration settings
    int chosen_security_setting = choose_security(SECURITY_SETTING);
    configure_security(chosen_security_setting);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);
    
    // btstack_run_loop_execute is only required when using the 'polling' method (e.g. using pico_cyw43_arch_poll library).
    // This example uses the 'threadsafe background` method, where BT work is handled in a low priority IRQ, so it
    // is fine to call bt_stack_run_loop_execute() but equally you can continue executing user code.

#if 0 // btstack_run_loop_execute() is not required, so lets not use it
    btstack_run_loop_execute();
#else
    // this core is free to do it's own stuff except when using 'polling' method (in which case you should use 
    // btstacK_run_loop_ methods to add work to the run loop.
    
    // this is a forever loop in place of where user code would go.
    while(true) {      
        sleep_ms(1000);
    }
#endif
    return 0;
}