/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "inttypes.h"
#include "string.h"

#ifndef NDEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif
#define INFO_LOG printf
#define ERROR_LOG printf

#define LED_QUICK_FLASH_DELAY_MS 100
#define LED_SLOW_FLASH_DELAY_MS 1000

typedef enum {
    TC_OFF,
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_W4_CONNECT,
    TC_W4_SERVICE_RESULT,
    TC_W4_CHARACTERISTIC_RESULT,
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
    TC_W4_READY
} gc_state_t;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static gc_state_t state = TC_OFF;
static bd_addr_t server_addr;
static bd_addr_type_t server_addr_type;
static hci_con_handle_t con_handle;
static gatt_client_service_t server_service;
static gatt_client_characteristic_t server_characteristic;
static bool listener_registered;
static gatt_client_notification_t notification_listener;
static btstack_timer_source_t heartbeat;

// Select a security setting to explore the BLE security
//
// security setting 0: Just works (pairing), no MITM (Man In The Middle) protection
//  client and server have no input or output support
//
// security setting 1: Numeric comparison with MITM protection
//  client can query yes or no from the user, server has a display only
//  server generates and displays passkey
//  client displays passkey and user can select Yes or No if they agree the passkey is from the server
//
// security setting 2:
//  client has a keyboard and display, server has a display only
//  server generates and displays passkey
//  client user enters the passkey displayed by the server
//
// security setting 3:
//  client has a display only, server has a display and keyboard
//  Client generates and displays passkey
//  server user enters the passkey displayed by the server
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
            // in the server if you want to persist bonds across power cycles.
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

static void client_start(void){
    DEBUG_LOG("Start scanning!\n");
    state = TC_W4_SCAN_RESULT;
    gap_set_scan_parameters(0,0x0030, 0x0030);
    gap_start_scan();
}

static bool advertisement_report_contains_service(uint16_t service, uint8_t *advertisement_report){
    // get advertisement from report event
    const uint8_t * adv_data = gap_event_advertising_report_get_data(advertisement_report);
    uint8_t adv_len  = gap_event_advertising_report_get_data_length(advertisement_report);

    // iterate over advertisement data
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data) ; ad_iterator_has_more(&context) ; ad_iterator_next(&context)){
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_size = ad_iterator_get_data_len(&context);
        const uint8_t * data = ad_iterator_get_data(&context);
        switch (data_type){
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                for (int i = 0; i < data_size; i += 2) {
                    uint16_t type = little_endian_read_16(data, i);
                    if (type == service) return true;
                }
            default:
                break;
        }
    }
    return false;
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t att_status;
    switch(state){
        case TC_W4_SERVICE_RESULT:
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_SERVICE_QUERY_RESULT:
                    // store service (we expect only one)
                    DEBUG_LOG("Storing service\n");
                    gatt_event_service_query_result_get_service(packet, &server_service);
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        ERROR_LOG("SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                        gap_disconnect(con_handle);
                        break;  
                    } 
                    // service query complete, look for characteristic
                    state = TC_W4_CHARACTERISTIC_RESULT;
                    DEBUG_LOG("Search for env sensing characteristic.\n");
                    gatt_client_discover_characteristics_for_service_by_uuid16(handle_gatt_client_event, con_handle, &server_service, ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE);
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_CHARACTERISTIC_RESULT:
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
                    DEBUG_LOG("Storing characteristic\n");
                    gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic);
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        ERROR_LOG("CHARACTERISTIC_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                        gap_disconnect(con_handle);
                        break;  
                    } 
                    // register handler for notifications
                    listener_registered = true;
                    gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, con_handle, &server_characteristic);
                    // enable notifications
                    DEBUG_LOG("Enable notify on characteristic.\n");
                    state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, con_handle,
                        &server_characteristic, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_ENABLE_NOTIFICATIONS_COMPLETE:
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_QUERY_COMPLETE:
                    DEBUG_LOG("Notifications enabled, ATT status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                    if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS) break;
                    state = TC_W4_READY;
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_READY:
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_NOTIFICATION: {
                    uint16_t value_length = gatt_event_notification_get_value_length(packet);
                    const uint8_t *value = gatt_event_notification_get_value(packet);
                    if (value_length == 2) {
                        float temp = little_endian_read_16(value, 0);
                        INFO_LOG("read temp %.2f degc\n", temp / 100);
                    } else {
                        ERROR_LOG("Unexpected length %d\n", value_length);
                    }
                    break;
                }
                default:
                    ERROR_LOG("Unknown packet type 0x%02x\n", hci_event_packet_get_type(packet));
                    break;
            }
            break;
        default:
            assert(false);
            ERROR_LOG("error bad state %u\n", state);
            break;
    }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;
    uint8_t status;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                DEBUG_LOG("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                client_start();
            } else {
                state = TC_OFF;
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            if (state != TC_W4_SCAN_RESULT) return;
            // check name in advertisement
            if (!advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING, packet)) return;
            // store address and type
            gap_event_advertising_report_get_address(packet, server_addr);
            server_addr_type = gap_event_advertising_report_get_address_type(packet);
            // stop scanning, and connect to the device
            state = TC_W4_CONNECT;
            gap_stop_scan();
            DEBUG_LOG("Connecting to device with addr %s.\n", bd_addr_to_str(server_addr));
            gap_connect(server_addr, server_addr_type);
            break;
        case HCI_EVENT_META_GAP:
            // BTstack normalises both the legacy LE Connection Complete and the
            // LE Enhanced Connection Complete HCI events into this single GAP
            // subevent, so this works regardless of which the controller emits
            // (i.e. regardless of ENABLE_LE_ENHANCED_CONNECTION_COMPLETE_EVENT).
            if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE) break;
            if (state != TC_W4_CONNECT) break;
            con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            DEBUG_LOG("Connection complete\n");
            // Note: GATT service discovery is NOT started here. The server requires
            // ENCRYPTION_KEY_SIZE_16 so all ATT requests are rejected until the link
            // is encrypted. We request pairing now and defer discovery to
            // SM_EVENT_PAIRING_COMPLETE so the link is always encrypted before any
            // GATT traffic is sent.
            sm_request_pairing(con_handle);
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            status = gatt_event_query_complete_get_att_status(packet);
            switch (status){
                case ATT_ERROR_INSUFFICIENT_ENCRYPTION:
                    ERROR_LOG("GATT Query result: Insufficient Encryption\n");
                    break;
                case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                    ERROR_LOG("GATT Query result: Insufficient Authentication\n");
                    break;
                case ATT_ERROR_BONDING_INFORMATION_MISSING:
                    ERROR_LOG("GATT Query result: Bonding Information Missing\n");
                    break;
                case ATT_ERROR_SUCCESS:
                    DEBUG_LOG("GATT Query result: OK\n");
                    break;
                default:
                    assert(false);
                    ERROR_LOG("Unexpected GATT Query result: 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // unregister listener
            con_handle = HCI_CON_HANDLE_INVALID;
            if (listener_registered){
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            DEBUG_LOG("Disconnected %s\n", bd_addr_to_str(server_addr));
            if (state == TC_OFF) break;
            client_start();
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
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST: {
            if (passkey_result) {
                DEBUG_LOG("SM_EVENT_NUMERIC_COMPARISON_REQUEST confirm\n");
                sm_numeric_comparison_confirm(passkey_handle);
            } else {
                DEBUG_LOG("SM_EVENT_NUMERIC_COMPARISON_REQUEST decline\n");
                sm_bonding_decline(passkey_handle);
            }
            break;
        }
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
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST: {
            if (key == 'y' || key == 'Y') {
                passkey_result = 1;
            } else {
                passkey_result = 0;
            }
            // We want to call a BTStack function now,
            // but we might be in an IRQ and so it's dangerous to call BTStack directly from here.
            // So we ask BTstack to callback a function from the "main thread"
            // We could also use an async (when pending) worker using async_when_pending_worker_t,
            // async_context_add_when_pending_worker and async_context_set_work_pending
            btstack_run_loop_execute_on_main_thread(&passkey_done);
            break;
        }
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

static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t addr;
    bd_addr_type_t addr_type;

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            INFO_LOG("Just works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            // Ask the user if the passkey matches the number shown by the server
            INFO_LOG("Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
            INFO_LOG("Is the server showing this number? (y/n)\n");
            // We want to "wait" for user input but we must not block,
            // so use a stdio callback to tell us when characters are available
            passkey_type = SM_EVENT_NUMERIC_COMPARISON_REQUEST;
            passkey_handle = sm_event_passkey_display_number_get_handle(packet);
            stdio_set_chars_available_callback(passkey_press_callback, NULL);
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            INFO_LOG("Displaying passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            INFO_LOG("Enter the passkey shown by the server:\n");
            // We want to "wait" for user input but we must not block,
            // so use a stdio callback to tell us when characters are available
            passkey_result = 0;
            passkey_type = SM_EVENT_PASSKEY_INPUT_NUMBER;
            passkey_handle = sm_event_passkey_input_number_get_handle(packet);
            stdio_set_chars_available_callback(passkey_press_callback, NULL);
            break;
        case SM_EVENT_PAIRING_STARTED:
            DEBUG_LOG("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            stdio_set_chars_available_callback(NULL, NULL); // stop key press notifications
            switch (sm_event_pairing_complete_get_status(packet)){
                case ERROR_CODE_SUCCESS:
                    DEBUG_LOG("Pairing complete, success\n");
                    // Now that the link is encrypted, start GATT service discovery.
                    DEBUG_LOG("Search for env sensing service.\n");
                    state = TC_W4_SERVICE_RESULT;
                    gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, con_handle, ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING);
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
            DEBUG_LOG("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
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
                    ERROR_LOG("Deleting local bonding information and start new pairing...\n");
                    sm_event_reencryption_complete_get_address(packet, addr);
                    addr_type = sm_event_reencryption_started_get_addr_type(packet);
                    gap_delete_bonding(addr_type, addr);
                    sm_request_pairing(sm_event_reencryption_complete_get_handle(packet));
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void heartbeat_handler(struct btstack_timer_source *ts) {
    // Invert the led
    static bool quick_flash;
    static bool led_on = true;

    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    if (listener_registered && led_on) {
        quick_flash = !quick_flash;
    } else if (!listener_registered) {
        quick_flash = false;
    }

    // Restart timer
    btstack_run_loop_set_timer(ts, (led_on || quick_flash) ? LED_QUICK_FLASH_DELAY_MS : LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(ts);
}

int main() {
    stdio_init_all();

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        ERROR_LOG("failed to initialise cyw43_arch\n");
        return -1;
    }

    l2cap_init();
    sm_init();

    // setup empty ATT server - only needed if LE Peripheral does ATT queries on its own, e.g. Android and iOS
    att_server_init(NULL, NULL, NULL);

    gatt_client_init();

    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // apply security configuration settings
    int chosen_security_setting = choose_security(SECURITY_SETTING);
    configure_security(chosen_security_setting);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // turn on!
    hci_power_control(HCI_POWER_ON);

    // btstack_run_loop_execute is only required when using the 'polling' method (e.g. using pico_cyw43_arch_poll library).
    // This example uses the 'threadsafe background` method, where BT work is handled in a low priority IRQ, so it
    // is fine to call bt_stack_run_loop_execute() but equally you can continue executing user code.

#if 1 // this is only necessary when using polling (which we aren't, but we're showing it is still safe to call in this case)
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