/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"
#include "pico/btstack_flash_bank.h"
#include "provisioning.h"
#include "wifi_prov_lib.h"
#include "hardware/gpio.h"
#include "pico/flash.h"
#include "hardware/flash.h"

#ifndef NDEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif
#define INFO_LOG printf
#define ERROR_LOG printf

#define HEARTBEAT_PERIOD_MS 1000
#define APP_AD_FLAGS 0x06

// max lengths of credentials + 1 to ensure null termination
static char ssid[33] = "";
static char password[64] = "";
static bool connection_status = false;
static int le_notification_enabled;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

// Provisioning status reported to the client over the status characteristic.
// Only this status byte is ever notified - credentials are never echoed back.
typedef enum {
    PROV_STATUS_IDLE = 0,
    PROV_STATUS_CREDENTIALS_RECEIVED = 1,
    PROV_STATUS_CONNECTING = 2,
    PROV_STATUS_CONNECTED = 3,
    PROV_STATUS_FAILED_AUTH = 4,
    PROV_STATUS_FAILED = 5,
} prov_status_t;
static uint8_t prov_status = PROV_STATUS_IDLE;
static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

static uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xFF,
};
static const uint8_t adv_data_len = sizeof(adv_data);

// Chose a safe place to store the WiFi credentials - well away from btstack tlv storage
#define FLASH_TARGET_OFFSET (PICO_FLASH_BANK_STORAGE_OFFSET - 2 * FLASH_SECTOR_SIZE)

static const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

// Security Manager Packet Handler 
static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t addr;
    bd_addr_type_t addr_type;

    int type = hci_event_packet_get_type(packet);
    switch (type) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            DEBUG_LOG("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
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
            ERROR_LOG("Identity resolving failed\n");
            break;
        case SM_EVENT_PAIRING_STARTED:
            DEBUG_LOG("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE: {
            int status = sm_event_pairing_complete_get_status(packet);
            switch (status){
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
                    break;
            }
            break;
        }
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_LOG("Bonding information exists for addr type %u, identity addr %s -> re-encryption started\n",
                   sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE: {
            int status = sm_event_reencryption_complete_get_status(packet);
            switch (status){
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
        }
        default:
            break;
    }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case HCI_EVENT_META_GAP:
            switch (hci_event_gap_meta_get_subevent_code(packet)) {
                case GAP_SUBEVENT_LE_CONNECTION_COMPLETE:
                    DEBUG_LOG("Connection complete\n");
                    con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                    // We don't need sm_request_pairing because the characteristics have ENCRYPTION_KEY_SIZE_16,
                    // so will trigger encryption on demand?
                    // also see https://github.com/bluekitchen/btstack/issues/738
                    // sm_request_pairing(con_handle);
                    break;
                default:
                    break;
            }
            break;

        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            INFO_LOG("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

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

            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            con_handle = HCI_CON_HANDLE_INVALID;
            break;
        default:
            break;
    }
}

// Update the provisioning status and, if the client has subscribed, notify it.
// Safe to call from the BTstack run loop context (e.g. the connection logic).
static void set_prov_status(prov_status_t status) {
    prov_status = (uint8_t)status;
    if (le_notification_enabled && con_handle != HCI_CON_HANDLE_INVALID) {
        att_server_request_can_send_now_event(con_handle);
    }
}

static void att_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case ATT_EVENT_CAN_SEND_NOW:
            // Only the provisioning status byte is notified (never credentials),
            // so a single notification per can-send-now event is sufficient.
            att_server_notify(con_handle, ATT_CHARACTERISTIC_6072c5a6_e1a2_4068_b30b_8f3f0e5c8634_01_VALUE_HANDLE, &prov_status, sizeof(prov_status));
            break;
        default:
            break;
    }
}

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    // SSID read callbaclk
    if (att_handle == ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&ssid, sizeof(ssid), offset, buffer, buffer_size);
    }

    // Password read callback
    if (att_handle == ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&password, sizeof(password), offset, buffer, buffer_size);
    }

    return 0;
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);

    // Client Characteristic Configuration write for the status characteristic:
    // the client is subscribing/unsubscribing to status notifications.
    if (att_handle == ATT_CHARACTERISTIC_6072c5a6_e1a2_4068_b30b_8f3f0e5c8634_01_CLIENT_CONFIGURATION_HANDLE) {
        le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
        hard_assert(con_handle);
        if (le_notification_enabled) {
            // Push the current status straight away so a freshly-subscribed client
            // learns where provisioning currently stands.
            att_server_request_can_send_now_event(con_handle);
        }
        return 0;
    }

    // SSID characteristic (write-only sink - never echoed back).
    if (att_handle == ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE){
        DEBUG_LOG("Setting SSID\n");
        memset(ssid, 0, sizeof(ssid));
        memcpy(ssid, buffer, buffer_size);
        DEBUG_LOG("Current saved SSID: \"%s\"\n", ssid);
        DEBUG_LOG("Current saved password length: %u\n", strlen(password));
    }

    // Password characteristic (write-only sink - never echoed back).
    if (att_handle == ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE){
        DEBUG_LOG("Setting password\n");
        memset(password, 0, sizeof(password));
        memcpy(password, buffer, buffer_size);
        DEBUG_LOG("Current saved SSID: \"%s\"\n", ssid);
        DEBUG_LOG("Current saved password length: %u\n", strlen(password));
    }

    // Once both have arrived, let a subscribed client know we have what we need.
    if (ssid[0] && password[0]) {
        set_prov_status(PROV_STATUS_CREDENTIALS_RECEIVED);
    }

    return 0;
}

static void heartbeat_handler(struct btstack_timer_source *ts) {
    static uint32_t counter = 0;
    counter++;
   
    // Invert the led
    static int led_on = true;
    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

    // Restart timer
    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

int erase_credentials(void) {
    return flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
}

static int save_credentials(char ssid[], char password[]) {
    // create empty 256 byte list
    uint8_t flash_data[FLASH_PAGE_SIZE] = {0};

    uint ssid_len = strlen(ssid);
    uint password_len = strlen(password);

    // no character has ascii value 0, so we can seperate our ssid and password with a single 0
    // first add ssid 
    for (uint i = 0; i < ssid_len; i++) {
        int ascii = (int) ssid[i];
        flash_data[i] = ascii;
    }

    //next add password
    for (uint i = 0; i < password_len; i++) {
        int ascii = (int) password[i];
        flash_data[i + ssid_len + 1] = ascii;
    }

    //now erase and then write flash
    int rc = erase_credentials();
    if (rc != PICO_OK) {
        return rc;
    }

    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)flash_data};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    return rc;
}

static void read_credentials(void) {
    uint counter = 0;
    uint ssid_len = 0;

    // first check if the flash page begins with FF - this indicates the flash has not yet been written to 
    // so must initialise with empty write (otherwise crashes)
    if (flash_target_contents[0] == 255) {
        int rc = save_credentials("", "");
        hard_assert(rc == PICO_OK);
    }

    //initialise temporary ssid and password as 1 bigger than max to ensure null termination
    char t_ssid[33] = {0};
    char t_password[64] = {0};

    // itterate through the flash and seperate ssid and password
    for (uint i = 0; i < FLASH_PAGE_SIZE; i++) {
        // when detect first zero, increment counter and continue. update ssid_len so we can index password
        if (flash_target_contents[i] == 0 && counter == 0) {
            counter++;
            ssid_len = i;
            continue;
        } 
        // when detect second zero, have extracted both ssid and password so stop
        else if (flash_target_contents[i] == 0 && counter == 1)
        {
            break;
        }
        // otherwise just write ssid and password
        else if (counter == 0) {
            t_ssid[i] = (char) flash_target_contents[i];
        }
        else if (counter == 1) {
            t_password[i - ssid_len - 1] = (char) flash_target_contents[i];
        }
    }
    // update global ssid and password
    memset(ssid, 0, sizeof(ssid));
    memcpy(ssid, t_ssid, sizeof(t_ssid));

    memset(password, 0, sizeof(password));
    memcpy(password, t_password, sizeof(t_password));
}

static void delete_all_le_bonds(void) {
    int max = le_device_db_max_count();
    for (int i = 0; i < max; i++) {
        bd_addr_t addr;
        int addr_type;
        le_device_db_info(i, &addr_type, addr, NULL);
        // valid entries have a real address type; empty slots report BD_ADDR_TYPE_UNKNOWN
        if (addr_type != BD_ADDR_TYPE_UNKNOWN) {
            gap_delete_bonding((bd_addr_type_t)addr_type, addr);
        }
    }
}

// this function carries out the BLE credential provisioning and also wifi connection
int start_ble_wifi_provisioning(int ble_timeout_ms, bool wipe_bonds) {

    absolute_time_t timeout_time = make_timeout_time_ms(ble_timeout_ms);
    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // secure manager register handler
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // configure secure BLE: LE Secure Connections, Just Works (no IO), with bonding
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // register for ATT event
    att_server_register_packet_handler(att_handler);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    if (wipe_bonds) {
        delete_all_le_bonds();
    }

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);

    read_credentials();
    DEBUG_LOG("Read credentials\n");
    DEBUG_LOG("Current saved SSID: \"%s\"\n", ssid);
    DEBUG_LOG("Current saved password length: %u\n", strlen(password));

    // first attempt to connect using saved credentials
    cyw43_arch_enable_sta_mode();
    connection_status = false;
    if (ssid[0] && password[0]) {
        if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, us_to_ms(absolute_time_diff_us(get_absolute_time(), timeout_time)))) { 
            ERROR_LOG("failed to connect with saved credentials\n");
        } else {
            DEBUG_LOG("Connected.\n");
            connection_status = true;
        }
    } else {
        INFO_LOG("Waiting to receive ssid and password via BLE\n");
    }

    // If this fails, wait for user to provision credentials over BLE until timeout
    // cyw43_arch_wifi_connect_timeout_ms returns -2 for timeout and -7 for incorrect password
    // keep trying if password is incorrect
    int result = PICO_OK;
    if (connection_status == false) {
        while (true) {
            if (ssid[0] && password[0]) {
                set_prov_status(PROV_STATUS_CONNECTING);
                result = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, us_to_ms(absolute_time_diff_us(get_absolute_time(), timeout_time)));
                if (result == PICO_ERROR_TIMEOUT) {
                    ERROR_LOG("Timed out - failed provisioning!\n");
                    set_prov_status(PROV_STATUS_FAILED);
                    break;
                } else if (result == PICO_OK) {
                    connection_status = true;
                    DEBUG_LOG("Succesfully provisioned credentials using wifi_prov_lib!\n");
                    set_prov_status(PROV_STATUS_CONNECTED);
                    // since connected, save credentiald for future use
                    result = save_credentials(ssid, password);
                    break;
                } else if (result == PICO_ERROR_BADAUTH) {
                    DEBUG_LOG("Incorrect password - retrying\n");
                    set_prov_status(PROV_STATUS_FAILED_AUTH);
                } else {
                    DEBUG_LOG("Connection error - failed provisioning!\n");
                    set_prov_status(PROV_STATUS_FAILED);
                    break;
                }
            } else {
                if (absolute_time_diff_us(get_absolute_time(), timeout_time) < 0) {
                    ERROR_LOG("Timed out - no ssid or password received!\n");
                    result = PICO_ERROR_TIMEOUT;
                    break;
                }
                cyw43_arch_poll();
                cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
            }
        }
    }
    // once finished, turn off bluetooth
    hci_power_control(HCI_POWER_OFF);
    return result;
}