/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpi_connect_ota_demo.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/ffs.h"
#include "pico/rpi_connect.h"
#include "pico/rpi_connect_ota.h"
#include "pico/stdio.h"
#include "pico/unique_id.h"

// FFS file IDs for the WiFi credentials. These must not collide with the
// RPI_CONNECT_FFS_* IDs (0x1-0x5) used by the OTA library.
#define RPI_CONNECT_FFS_WIFI_SSID       0x10
#define RPI_CONNECT_FFS_WIFI_PASSWORD   0x11

static char *rpi_connect_ota_demo_wifi_password;
static char *rpi_connect_ota_demo_wifi_ssid;

static int wifi_load_credentials(void) {
    char *wifi_ssid = ffs_get_string(RPI_CONNECT_FFS_WIFI_SSID);
    char *wifi_password = ffs_get_string(RPI_CONNECT_FFS_WIFI_PASSWORD);

    if (!wifi_ssid || !*wifi_ssid || !wifi_password) {
        RPI_CONNECT_OTA_DEMO_ERROR("No WiFi credentials in FFS - provision with the combined UF2\n");
        free(wifi_ssid);
        free(wifi_password);
        return PICO_ERROR_BADAUTH;
    }

    RPI_CONNECT_OTA_DEMO_INFO("WiFi SSID: %s\n", wifi_ssid);
    rpi_connect_ota_demo_wifi_password = wifi_password;
    rpi_connect_ota_demo_wifi_ssid = wifi_ssid;
    return PICO_OK;
}

static void wifi_free_credentials(void) {
    free(rpi_connect_ota_demo_wifi_password);
    rpi_connect_ota_demo_wifi_password = NULL;

    free(rpi_connect_ota_demo_wifi_ssid);
    rpi_connect_ota_demo_wifi_ssid = NULL;
}

async_context_t *rpi_connect_default_async_context(void) {
    return cyw43_arch_async_context();
}

int main() {
    char serial_number[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    char *token = NULL;
    int rc = 0;

    stdio_init_all();

    pico_get_unique_board_id_string(serial_number, sizeof(serial_number));

    RPI_CONNECT_OTA_DEMO_INFO("rpi_connect_ota_demo version: %s (image v%d.%d)\n",
        PICO_PROGRAM_VERSION_STRING, RPI_CONNECT_VERSION_MAJOR, RPI_CONNECT_BUILD_NUMBER);

    if (cyw43_arch_init()) {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to initialise cyw43\n");
        return 1;
    }

    rc = ffs_initialise();
    if (rc != PICO_OK) {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to initialise FFS %d\n", rc);
        goto end;
    }

    rc = wifi_load_credentials();
    if (rc != PICO_OK) {
        goto end;
    }

    RPI_CONNECT_OTA_DEMO_INFO("Connecting to WiFi %s (client_id=%s serial=%s)\n",
           rpi_connect_ota_demo_wifi_ssid, rpi_connect_client_id(), serial_number);

    cyw43_arch_enable_sta_mode();
    int retries = 5;
    while (cyw43_arch_wifi_connect_timeout_ms(rpi_connect_ota_demo_wifi_ssid,
                                              rpi_connect_ota_demo_wifi_password,
                                              CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        if (--retries == 0) {
            RPI_CONNECT_OTA_DEMO_ERROR("Failed to connect to WiFi\n");
            rc = -1;
            goto end;
        }
        RPI_CONNECT_OTA_DEMO_INFO("WiFi connection failed, retrying (%d remaining)\n", retries);
        sleep_ms(2000);
    }
    RPI_CONNECT_OTA_DEMO_INFO("Connected\n");

    retries = 5;
    while ((rc = rpi_connect_ota_demo_init(rpi_connect_client_id(), serial_number, "rpi_connect_ota_demo")) != PICO_OK && --retries) {
        sleep_ms(2000);
    }
    if (rc != PICO_OK) {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to sign in to Pi Connect (%d)\n", rc);
        goto end;
    }

    token = rpi_connect_ota_get_auth_token();
    rpi_connect_ota_demo_main(token);
    rc = 0;

end:
    cyw43_arch_disable_sta_mode();
    cyw43_arch_deinit();
    RPI_CONNECT_OTA_DEMO_INFO("Disconnected\n");
    free(token);
    wifi_free_credentials();
    return rc;
}
