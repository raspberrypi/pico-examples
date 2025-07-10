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
static hci_con_handle_t con_handle;
static uint16_t current_temp;

extern uint8_t const profile_data[];
static void poll_temp(void);

static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// Select a security setting to explore the BLE security
// security setting 0: Just works (pairing), no MITM protection
// security setting 1: Numeric comparison
// security setting 2: Peripheral displays passkey, client enters passkey
// security setting 3: Client displays passkey, peripheral enters passkey 
static int security_setting = 1;

static void configure_security(int security_setting) {

    sm_set_secure_connections_only_mode(true);

    switch (security_setting) {
        case 0:
            printf("Security setting 0 selected. \n");

            sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION);
            break;
        case 1:
            printf("Security setting 1 selected. \n");

            sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_YES_NO);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        case 2:
            printf("Security setting 2 selected. \n");

            sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_ONLY);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        case 3:
            printf("Security setting 3 selected. \n");

            sm_set_io_capabilities(IO_CAPABILITY_KEYBOARD_ONLY);
            sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION|SM_AUTHREQ_MITM_PROTECTION);
            break;
        default:
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
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

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
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, (uint8_t*)&current_temp, sizeof(current_temp));
            break;
        default:
            break;
    }
}

static void sm_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    hci_con_handle_t con_handle;
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint8_t status;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_META_GAP:
            switch (hci_event_gap_meta_get_subevent_code(packet)) {
                case GAP_SUBEVENT_LE_CONNECTION_COMPLETE:
                    printf("Connection complete\n");
                    con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                    sm_request_pairing(con_handle);
                    break;
                default:
                    break;
            }
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            printf("Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("Display Passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            char passkey[7];

            printf("Passkey Input requested \n");
            scanf("%6[^\n]", passkey);
            int to_send = atoi(passkey); // convert passkey to int

            printf("Sending passkey %"PRIu32"\n", to_send);
            sm_passkey_input(sm_event_passkey_input_number_get_handle(packet), to_send);
            break;
        case SM_EVENT_IDENTITY_CREATED:
            sm_event_identity_created_get_identity_address(packet, addr);
            printf("Identity created: type %u address %s\n", sm_event_identity_created_get_identity_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
            sm_event_identity_resolving_succeeded_get_identity_address(packet, addr);
            printf("Identity resolved: type %u address %s\n", sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED:
            sm_event_identity_created_get_address(packet, addr);
            printf("Identity resolving failed\n");
            break;
        case SM_EVENT_PAIRING_STARTED:
            printf("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    printf("Pairing complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    printf("Pairing failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    printf("Pairing failed, disconnected\n");
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    printf("Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                    break;
                default:
                    break;
            }
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            printf("Bonding information exists for addr type %u, identity addr %s -> re-encryption started\n",
                   sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            switch (sm_event_reencryption_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    printf("Re-encryption complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    printf("Re-encryption failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    printf("Re-encryption failed, disconnected\n");
                    break;
                case ERROR_CODE_PIN_OR_KEY_MISSING:
                    printf("Re-encryption failed, bonding information missing\n\n");
                    printf("Assuming remote lost bonding information\n");
                    printf("Deleting local bonding information to allow for new pairing...\n");
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
                    printf("GATT Query failed, Insufficient Encryption\n");
                    break;
                case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                    printf("GATT Query failed, Insufficient Authentication\n");
                    break;
                case ATT_ERROR_BONDING_INFORMATION_MISSING:
                    printf("GATT Query failed, Bonding Information Missing\n");
                    break;
                case ATT_ERROR_SUCCESS:
                    printf("GATT Query successful\n");
                    break;
                default:
                    printf("GATT Query failed, status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
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
    
    if (att_handle != ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_CLIENT_CONFIGURATION_HANDLE) return 0;
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
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
    printf("Write temp %.2f degc\n", deg_c);
 }

static void heartbeat_handler(struct btstack_timer_source *ts) {
    static uint32_t counter = 0;
    counter++;

    // Update the temp every 10s
    if (counter % 10 == 0) {
        poll_temp();
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
        printf("failed to initialise cyw43_arch\n");
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
    configure_security(security_setting);

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
