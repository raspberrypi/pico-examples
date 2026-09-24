/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* On-device equivalent of the host tool's --create-device-identity (main.c):
 * registers this board's OTP identity key with an organisation, so the private
 * key is never handled on a host. Run this once per board, before
 * rpi_connect_ota_demo - afterwards the device obtains its own tokens through
 * the device-identity exchange, which is what the demo does at boot.
 *
 * The organisation token is typed in at the console (USB or UART) rather than
 * provisioned: it is an org-wide credential, and this way it is never written
 * to flash at all. On success the board reboots to USB boot, ready for the demo
 * image to be flashed.
 */

#include <stdio.h>

#include "connect_crypto.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/ffs.h"
#include "pico/rpi_connect.h"
#include "pico/rpi_connect_ota.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

// ffs file IDs, provisioned into the ffs blob by CMakeLists.txt. These must not
// collide with the RPI_CONNECT_FFS_* IDs (0x1-0x5) used by the OTA library.
#define FFS_WIFI_SSID       0x10
#define FFS_WIFI_PASSWORD   0x11

// Longest organisation token that survives being formatted into the API's
// "Authorization: Bearer %s" header (a 256-byte buffer in rpi_connect.c).
#define ORG_TOKEN_SIZE      234

async_context_t *rpi_connect_default_async_context(void) {
    return cyw43_arch_async_context();
}

// Read a line from stdin - whichever of USB and UART it arrives on - echoing it
// as it is typed. An empty line re-prompts, so a terminal attached after boot
// still gets a prompt.
static void read_line(const char *prompt, char *buf, size_t buf_size) {
    size_t len;
    do {
        printf("%s", prompt);
        stdio_flush();
        len = 0;
        for (int c = getchar(); c != '\r' && c != '\n'; c = getchar()) {
            if (c == '\b' || c == 0x7f) {        // backspace / delete
                if (len) {
                    len--;
                    printf("\b \b");
                }
            } else if (c >= ' ' && c < 0x7f && len + 1 < buf_size) {
                buf[len++] = (char)c;
                putchar(c);
            }
        }
        buf[len] = '\0';
        printf("\n");
    } while (len == 0);
}

int main() {
    char serial_number[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    char device_name[sizeof(serial_number) + 5];
    unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE];

    stdio_init_all();

    if (cyw43_arch_init() || ffs_initialise() != PICO_OK) {
        printf("Failed to initialise\n");
        return 1;
    }

    char *ssid = ffs_get_string(FFS_WIFI_SSID);
    char *password = ffs_get_string(FFS_WIFI_PASSWORD);
    if (!ssid || !*ssid || !password) {
        printf("No WiFi credentials in ffs\n");
        return 1;
    }

    if (!rpi_connect_ota_identity_key_programmed()) {
        printf("No identity key in OTP\n");
        return 1;
    }
    rpi_connect_ota_read_identity_key_otp(RPI_CONNECT_IDENTITY_OTP_ROW, privkey);

    printf("Connecting to WiFi %s\n", ssid);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to WiFi\n");
        return 1;
    }
    rpi_connect_set_async_context(rpi_connect_default_async_context());

    // Typed in rather than provisioned, so it only ever lives in RAM. Static
    // because it stays live across the TLS handshake below, and this stack is
    // already tight (PICO_STACK_SIZE in CMakeLists.txt).
    static char org_token[ORG_TOKEN_SIZE];
    read_line("Organisation token: ", org_token, sizeof(org_token));

    // Registering the public key derived from the OTP private key makes this
    // identity, by construction, one the device can later prove it owns.
    char *pubkey_pem = rpi_connect_crypto_ecdsa_p256_pubkey_pem(privkey);
    if (!pubkey_pem) {
        printf("Failed to derive public key\n");
        return 1;
    }

    pico_get_unique_board_id_string(serial_number, sizeof(serial_number));
    snprintf(device_name, sizeof(device_name), "pico-%s", serial_number);

    char *id = rpi_connect_create_device_identity(org_token, privkey, pubkey_pem,
                                                  "Pico 2 W OTA demo", device_name);
    if (!id) {
        printf("Failed to create device identity\n");
        return 1;
    }

    printf("Created device identity %s for %s\n", id, device_name);

    // Hand the board back in BOOTSEL, ready for the demo image to be flashed.
    printf("Rebooting to USB boot\n");
    stdio_flush();

    return 0;
}
