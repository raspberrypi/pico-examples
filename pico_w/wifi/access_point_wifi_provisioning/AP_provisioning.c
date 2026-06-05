/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip4_addr.h"
#include "lwip/init.h"
#include "lwip/apps/httpd.h"

#include "dhcpserver.h"
#include "dnsserver.h"

#include "pico/flash.h"
#include "hardware/flash.h" // for saving succesful credentials

static absolute_time_t wifi_connected_time;
static bool led_on = false;

// max lengths + 1
static char ssid[33];
static char password[64];

static int num_credentials;

static bool connection_status = false;

// how many sectors would you like to reserve
// each sector is 4096 bytes, so can hold 40 pairs of max length credentials
#define DESIRED_FLASH_SECTORS 1
static char ssid_list[40 * DESIRED_FLASH_SECTORS][33];
static char password_list[40 * DESIRED_FLASH_SECTORS][64];

// Define flash offset towards end of flash
#ifndef PICO_FLASH_BANK_TOTAL_SIZE
#define PICO_FLASH_BANK_TOTAL_SIZE (FLASH_SECTOR_SIZE * 2u)
#endif

#ifndef PICO_FLASH_BANK_STORAGE_OFFSET
#if PICO_RP2350 && PICO_RP2350_A2_SUPPORTED 
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE - FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS)
#else
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - PICO_FLASH_BANK_TOTAL_SIZE - FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS)
#endif
#endif

static const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

// Function prototypes
static void call_flash_range_erase(void *param);
static void call_flash_range_program(void *param);

static void save_credentials(char ssid[], char password[]);
static void read_credentials(void);

static void attempt_wifi_connection(void);

static const char *credential_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *connect_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *connect_from_saved_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
static const char *clear_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
    , uint16_t current_tag_part, uint16_t *next_tag_part
#endif
);

static tCGI cgi_handlers[] = {
    { "/credentials.cgi", credential_cgi_handler },
    { "/connect.cgi", connect_cgi_handler },
    { "/connect_from_saved.cgi", connect_from_saved_cgi_handler},
    {"/clear.cgi", clear_cgi_handler}
};

// Be aware of LWIP_HTTPD_MAX_TAG_NAME_LEN
static const char *ssi_tags[] = {
    "wifilist",
    "ssid",
    "password"
};

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    printf("intitialised\n");

    // for testing, erase memory first
    // might need to erase memory first time you use provisioning in case there are garbage values in flash
    //int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    //hard_assert(rc == PICO_OK);

    // First, try to connect to network using saved credentials
    read_credentials();

    cyw43_arch_enable_sta_mode();
    for (int i = 0; i < num_credentials; i++) {
        if (cyw43_arch_wifi_connect_timeout_ms(ssid_list[i], password_list[i], CYW43_AUTH_WPA2_AES_PSK, 5000)) { 
            printf("failed to connect with saved credentials %i \n", i);
        } else {
            printf("Connected.\n");
            connection_status = true;
            break;
        }
    }

    // If this fails, enable access point
    if (connection_status == false) {
        cyw43_arch_disable_sta_mode();
        cyw43_arch_enable_ap_mode("picow_test", "12345678", CYW43_AUTH_WPA2_AES_PSK);
        printf("\nReady, running server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

        #if LWIP_IPV6
        #define IP(x) ((x).u_addr.ip4)
        #else
        #define IP(x) (x)
        #endif

        ip4_addr_t mask;
        ip4_addr_t gw;
        IP(gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
        IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

        #undef IP
        dhcp_server_t dhcp_server;
        dhcp_server_init(&dhcp_server, &gw, &mask);

        dns_server_t dns_server;
        dns_server_init(&dns_server, &gw);

        char hostname[sizeof(CYW43_HOST_NAME) + 4];
        memcpy(&hostname[0], CYW43_HOST_NAME, sizeof(CYW43_HOST_NAME) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
        netif_set_hostname(&cyw43_state.netif[CYW43_ITF_STA], hostname);

        // start http server
        wifi_connected_time = get_absolute_time();

        // setup http server
        cyw43_arch_lwip_begin();
        httpd_init();
        http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
        http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
        cyw43_arch_lwip_end();
    }

    //wait for connection
    while(connection_status == false) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }

    printf("Finished provisioning credentials. \n");
    cyw43_arch_deinit();
    return 0;
}

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS);
}

// Functions for saving and reading credentials from flash
static void save_credentials(char ssid[], char password[]) {
    // create empty 256 byte list
    uint8_t flash_data[FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS] = {0};

    uint ssid_len = strlen(ssid);
    uint password_len = strlen(password);
    uint credential_count;

    //first check how many credentials are already saved
    if (flash_target_contents[1] != 255) {
        credential_count = flash_target_contents[1];
        //incriment this count since we are about to add a credential
        credential_count++;
        flash_data[1] = credential_count;
    } else {
        // first (empty) save, so dont want to incriment
        credential_count = 0;
    }

    //now need to find how far through the flash to start writing, and also add previous stuff to flash data
    uint write_start_location = 2;
    if (credential_count != 0) {
        uint count = 0;
        while (count < 2 * credential_count - 2) {
            flash_data[write_start_location] = flash_target_contents[write_start_location];
            if (flash_target_contents[write_start_location] == 0) {
                count++;
            }
            write_start_location++;
        }
    }

    // no character has ascii value 0, so we can seperate our ssid and password with a single 0
    // first add ssid 
    for (uint i = 0; i < ssid_len; i++) {
        int ascii = (int) ssid[i];
        //printf("%i\n", ascii);
        flash_data[i + write_start_location] = ascii;
    }

    //next add password
    for (uint i = 0; i < password_len; i++) {
        int ascii = (int) password[i];
        flash_data[i + ssid_len + write_start_location + 1] = ascii;
    }

    // must always erase flash before write
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);

    // write flash
    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)flash_data};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);
}

static void read_credentials(void) {
    uint credential_count;

    // first check if the flash page begins with FF - this indicates the flash has not yet been written to 
    // so must initialise with empty write
    if (flash_target_contents[0] == 255) {
        save_credentials("", "");
    }

    //second byte saves credential count (allows 255 sets of credentials, should be enough)
    credential_count = flash_target_contents[1];
    num_credentials = credential_count;
    //initialise temporary ssid and password as 1 bigger than max to ensure null termination
    char t_ssid_list[20][33] = {0};
    char t_password_list[20][64] = {0};

    uint space_count = 0;
    uint start_index = 1;

    for (uint i = 2; i < FLASH_SECTOR_SIZE * DESIRED_FLASH_SECTORS; i++) {
        if (space_count >= 2*credential_count) {
            break;
        } else if (flash_target_contents[i] == 0) {
            space_count++;
            start_index = i;
            printf("\n");
            //printf("space count %i\n", space_count);
        } else if (flash_target_contents[i] != 0 && space_count % 2 == 0) {
            // there is a char, and even space count. So we are reading a ssid
            t_ssid_list[(int) space_count / 2][i - start_index - 1] = flash_target_contents[i];
            //printf("%c", flash_target_contents[i]);
        } else if (flash_target_contents[i] != 0 && space_count % 2 == 1) {
            // there is a char and odd space count, so reading password
            t_password_list[(int) space_count / 2][i - start_index - 1] = flash_target_contents[i];
            //printf("%c", flash_target_contents[i]);
        } 
    }
    
    // update global ssid and password lists
    memset(ssid_list, 0, sizeof(ssid_list));
    memcpy(ssid_list, t_ssid_list, sizeof(t_ssid_list));

    memset(password_list, 0, sizeof(password_list));
    memcpy(password_list, t_password_list, sizeof(t_password_list));
}

static void attempt_wifi_connection(void) {
    cyw43_arch_disable_ap_mode();
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 15000)) { 
        panic("Failed to connect!");
    } else {
        printf("Connected.\n");
        connection_status = true;
        // success, so save credentials for future use
        save_credentials(ssid, password);
    }
}

static const char *credential_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("credential_cgi_handler called\n");
    strncpy(ssid, pcValue[0], sizeof(ssid) - 1);
    strncpy(password, pcValue[1], sizeof(password) - 1);
    printf("SSID AND PASSWORD: %s %s \n", ssid, password);
    return "/index.shtml";
}

static const char *connect_from_saved_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("load_from_saved_cgi_handler called\n");
    strncpy(ssid, ssid_list[atoi(pcValue[0])], sizeof(ssid) - 1);
    strncpy(password, password_list[atoi(pcValue[0])], sizeof(password) - 1);
    attempt_wifi_connection();
    return "/index.shtml";
}

static const char *connect_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("connect_cgi_handler called\n");
    attempt_wifi_connection();
    return "/index.shtml";
}

static const char *clear_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("clear_cgi_handler called\n");
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    save_credentials("", "");
    read_credentials();
    return "/index.shtml";
}

// Note that the buffer size is limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN, so use LWIP_HTTPD_SSI_MULTIPART to return larger amounts of data
static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
    , uint16_t current_tag_part, uint16_t *next_tag_part
#endif
) {
    int printed = 0;
    switch (iIndex) {
        case 0: { // wifilist
            for (int i = 0; i < num_credentials; i++) {
                printed += snprintf(pcInsert + printed, iInsertLen - printed, 
                                "<li>SSID: %s,   Password: %s,   Index: %i</li>", 
                                ssid_list[i], password_list[i], i);
            }
            break;
        }
        case 1: { // ssid
            printed = snprintf(pcInsert, iInsertLen, ssid);
            break;         
        }
        case 2: { // password
            printed = snprintf(pcInsert, iInsertLen, password);
            break;
        }
        default: { // unknown tag
            printed = 0;
            break;
        }
    }
    return printed;
}