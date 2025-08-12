/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"
#include "provisioning.h"
#include "wifi_prov_lib.h"
#include "hardware/gpio.h"
#include "pico/flash.h"
#include "hardware/flash.h"

#include <string.h>

#define HEARTBEAT_PERIOD_MS 1000
#define APP_AD_FLAGS 0x06

int le_notification_enabled;
hci_con_handle_t con_handle;

// max lengths of credentials + 1 to ensure null termination
char ssid[33] = "";
char password[64] = "";

bool connection_status = false;

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

// Define flash offset towards end of flash
#ifndef PICO_FLASH_BANK_TOTAL_SIZE
#define PICO_FLASH_BANK_TOTAL_SIZE (FLASH_SECTOR_SIZE * 2u)
#endif

#ifndef PICO_FLASH_BANK_STORAGE_OFFSET
#if PICO_RP2350 && PICO_RP2350_A2_SUPPORTED 
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE)
#else
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE)
#endif
#endif

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

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
void sm_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
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
                    UNUSED(con_handle);
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

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case HCI_EVENT_META_GAP:
            switch (hci_event_gap_meta_get_subevent_code(packet)) {
                case GAP_SUBEVENT_LE_CONNECTION_COMPLETE:
                    printf("Connection complete\n");
                    con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                    UNUSED(con_handle);
                    sm_request_pairing(con_handle);
                    break;
                default:
                    break;
            }
            break;

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

            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            att_server_notify(con_handle, ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE, (uint8_t*)ssid, sizeof(ssid));
            att_server_notify(con_handle, ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE, (uint8_t*)password, sizeof(password));
            break;
        
        default:
            break;
    }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
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

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
    if (le_notification_enabled) {
        att_server_request_can_send_now_event(con_handle);
        //This occurs when the client enables notification (the download button on nrf scanner)
    }

    // First characteristic (SSID)
    if (att_handle == ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE){
        att_server_request_can_send_now_event(con_handle);
        memset(ssid, 0, sizeof(ssid));
        memcpy(ssid, buffer, buffer_size);
        //This occurs when the client sends a write request to the ssid characteristic (up arrow on nrf scanner)
        printf("Current saved SSID: %s\n", ssid);
        printf("Current saved password: %s\n", password);
    }

    // Second characteristic (Password)
    if (att_handle == ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE){
        att_server_request_can_send_now_event(con_handle);
        memset(password, 0, sizeof(password));
        memcpy(password, buffer, buffer_size);
        //This occurs when the client sends a write request to the password characteristic (up arrow on nrf scanner)
        printf("Current saved SSID: %s\n", ssid);
        printf("Current saved password: %s\n", password);
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

void save_credentials(char ssid[], char password[]) {
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
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);

    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)flash_data};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);
}

void read_credentials(void) {
    uint counter = 0;
    uint ssid_len = 0;

    // first check if the flash page begins with FF - this indicates the flash has not yet been written to 
    // so must initialise with empty write (otherwise crashes)
    if (flash_target_contents[0] == 255) {
        save_credentials("", "");
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

// this function carries out the BLE credential provisioning and also wifi connection
int start_ble_wifi_provisioning(int ble_timeout_ms) {
    stdio_init_all();

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // secure manager register handler
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // configure secure BLE (Just works) (legacy pairing)
    gatt_client_set_required_security_level(LEVEL_2);
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(0);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);

    read_credentials();
    printf("Current saved SSID: %s\n", ssid);
    printf("Current saved password: %s\n", password);

    // first attempt to connect using saved credentials
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 5000)) { 
        printf("failed to connect with saved credentials \n");
    } else {
        printf("Connected.\n");
        connection_status = true;
    }

    // If this fails, wait for user to provision credentials over BLE until timeout
    // cyw43_arch_wifi_connect_timeout_ms returns -2 for timeout and -7 for incorrect password
    // wish to keep trying if password incorrect 
    int result;
    if (connection_status == false) {
        while (true) {
            result = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, ble_timeout_ms);
            if (result == -2) {
                panic("Timed out - failed provisioning! \n");
            } else if (result == 0) {
                connection_status = true;
                printf("Succesfully provisioned credentials through BLE! \n");
                // since connected, save credentiald for future use
                save_credentials(ssid, password);
                break;
            } else if (result == -7) {
                printf("Incorrect password - retrying \n");
            } else {
                panic("Connection error - failed provisioning! \n");
            }
        }
    }
    // once finished, turn off bluetooth
    hci_power_control(HCI_POWER_OFF);
    return 0;
}
