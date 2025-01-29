/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//
// Created by elliot on 25/05/24.
//
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"

// Temperature
#ifndef TEMPERATURE_UNITS
#define TEMPERATURE_UNITS 'C' // Set to 'F' for Fahrenheit
#endif

#ifndef MQTT_SERVER
#error Need to define MQTT_SERVER
#endif

// MQTT parameters
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[100];
    uint32_t len;
    ip_addr_t mqtt_server_address;
} MQTT_CLIENT_DATA_T;

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
static float read_onboard_temperature(const char unit) {

    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C' || unit != 'F') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}

static void control_led(MQTT_CLIENT_DATA_T *state, bool on) {
    // Publish state on /state topic and on/off led board
    const char* message = on ? "On" : "Off";
    if (on)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    else
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    char state_topic[128];
    snprintf(state_topic, sizeof(state_topic), "%s/state", state->topic);
    mqtt_publish(state->mqtt_client_inst, state_topic, message, strlen(message), 0, 0, NULL, NULL);
}

static void publish_temperature(MQTT_CLIENT_DATA_T *state) {
    static float old_temperature;
    const char temperature_key[] = "/temperature";
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    if (temperature != old_temperature) {
        old_temperature = temperature;
        // Publish temperature on /temperature topic
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.2f", temperature);
        printf("Publishing %s to %s\n", temp_str, temperature_key);
        mqtt_publish(state->mqtt_client_inst, temperature_key, temp_str, strlen(temp_str), 0, 0, NULL, NULL);
    }
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    printf("Topic: %s, Message: %s\n", state->topic, state->data);

    if (strcmp(state->topic, "/led") == 0)
    {
        if (lwip_stricmp((const char *)state->data, "On") == 0)
            control_led(state, true);
        else if  (lwip_stricmp((const char *)state->data, "Off") == 0)
            control_led(state, false);
    }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(mqtt_client->topic, topic, sizeof(mqtt_client->topic));
}

static void temperature_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_temperature(state);
    async_context_add_at_time_worker_in_ms(context, worker, 10000); // repeat in 10s
}
static async_at_time_worker_t temperature_worker = { .do_work = temperature_worker_fn };

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        mqtt_sub_unsub(client, "/led", 0, NULL, arg, 1);
        printf("Connected to the /led topic successfully\n");

        // Publish temperature every 10 sec if it's changed
        temperature_worker.user_data = arg;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &temperature_worker, 0);
    } else {
        panic("Unexpected status");
    }
}

static void start_client(MQTT_CLIENT_DATA_T *state) {
    state->mqtt_client_inst = mqtt_client_new();
    if (!state->mqtt_client_inst) {
        panic("MQTT client instance creation error");
    }
    printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, MQTT_PORT, mqtt_connection_cb, state, &state->mqtt_client_info) != ERR_OK) {
        panic("MQTT broker connection error");
    }
    mqtt_set_inpub_callback(state->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    MQTT_CLIENT_DATA_T *state = (MQTT_CLIENT_DATA_T*)arg;
    if (ipaddr) {
        state->mqtt_server_address = *ipaddr;
        start_client(state);
    } else {
        panic("dns request failed");
    }
}

int main(void) {
    stdio_init_all();

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    static MQTT_CLIENT_DATA_T state;

    // Use board unique id for the client id
    static char client_id_buf[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(client_id_buf, sizeof(client_id_buf));
    state.mqtt_client_info.client_id = client_id_buf;
    state.mqtt_client_info.keep_alive = 60; // Keep alive in sec
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    state.mqtt_client_info.client_user = MQTT_USERNAME;
    state.mqtt_client_info.client_pass = MQTT_PASSWORD;
#else
    state.mqtt_client_info.client_user = NULL;
    state.mqtt_client_info.client_pass = NULL;
#endif

    if (cyw43_arch_init()) {
        panic("Failed to inizialize CYW43");
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to connect");
    }
    printf("\nConnected to Wifi\n");

    // We are not in a callback so locking is needed when calling lwip
    // Make a DNS request for the MQTT server IP address
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(MQTT_SERVER, &state.mqtt_server_address, dns_found, &state);
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        // We have the address, just start the client
        start_client(&state);
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
        panic("dns request failed");
    }

    while (true) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
    }

    return 0;
}
