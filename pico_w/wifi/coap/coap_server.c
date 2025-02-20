/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// A simple coap server
// Find out what requests it supports: coap-client -M $CLIENT_KEY -m get coaps://$COAP_SERVER/.well-known/core
// Get the current temperature: coap-client -M $CLIENT_KEY -m get coaps://$COAP_SERVER/temp
// Get the current led state: coap-client -M $CLIENT_KEY -m get coaps://$COAP_SERVER/led
// Turn the led on: coap-client -M $CLIENT_KEY -m put coaps://$COAP_SERVER/led -e 1
// Subscribe to temperature changes: coap-client -M $CLIENT_KEY -m get coaps://$COAP_SERVER/temp -s 60 -w
// Get time: coap-client -M $CLIENT_KEY -m get coaps://$COAP_SERVER/time
// where $COAP_SERVER is the host name of the coap server
// and $CLIENT_KEY is the file path to the server.key (private raw public key when built with tinydtls)

#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/lwip_nosys.h"
#include "pico/util/datetime.h"
#include "hardware/adc.h"

#include "lwip/netif.h"

#include "coap_config.h"
#include <coap3/coap.h>

#ifdef COAP_CERT_INC
#include COAP_CERT_INC
#endif

#define ADC_CHANNEL_TEMPSENSOR 4
#define TEMP_WORKER_TIME_S 30
#define SESSION_TIMEOUT 30

#ifdef COAP_CERT_INC
#define COAP_PORT COAPS_DEFAULT_PORT
#define COAP_PROTO COAP_PROTO_DTLS
#else
#define COAP_PORT COAP_DEFAULT_PORT
#define COAP_PROTO COAP_PROTO_UDP
#endif

static coap_context_t *main_coap_context;
static coap_resource_t *temp_resource = NULL;
static coap_resource_t *led_resource = NULL;
static coap_resource_t *time_resource = NULL;
static float temp_deg_c;

static void coap_io_process_worker_func(async_context_t *context, async_at_time_worker_t *worker)
{
    assert(worker->user_data);
    coap_context_t *coap_context = (coap_context_t*)worker->user_data;

    coap_tick_t before;
    unsigned int num_sockets;
    unsigned int timeout;

    coap_ticks(&before);
    timeout = coap_io_prepare_io(coap_context, NULL, 0, &num_sockets, before);
    if (timeout < 1000) {
        timeout = 1000;
    }
    async_context_add_at_time_worker_in_ms(context, worker, timeout);
    coap_context->timer_configured = 1;
}
static async_at_time_worker_t coap_worker = {
    .do_work = coap_io_process_worker_func
};

void check_notify_worker_func(async_context_t *async_context, async_when_pending_worker_t *worker)
{
    assert(worker->user_data);
    coap_context_t *coap_context = (coap_context_t*)worker->user_data;
    coap_check_notify(coap_context);
}

static async_when_pending_worker_t check_notify_worker = {
    .do_work = check_notify_worker_func
};

static void notify_observers(coap_resource_t *resource)
{
    coap_resource_notify_observers(resource, NULL);
    async_context_set_work_pending(cyw43_arch_async_context(), &check_notify_worker);
}

bool sample_temp(void) {
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
    if (deg_c != temp_deg_c) {
        temp_deg_c = deg_c;
        coap_log_info("Temp %.2f\n", temp_deg_c);
        return true;
    }
    return false;
}

// Periodically publish temperature
static void temp_worker_func(async_context_t *context, async_at_time_worker_t *worker) {
    assert(worker->user_data);
    if (sample_temp()) {
        notify_observers(temp_resource);
    }
    if (temp_resource->subscribers) {
        async_context_add_at_time_worker_in_ms(context, worker, TEMP_WORKER_TIME_S * 1000);
    } else {
        worker->user_data = NULL;
    }
}

static async_at_time_worker_t temp_worker = {
    .do_work = temp_worker_func
};

static void resource_get_temp(coap_resource_t *resource, coap_session_t  *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response) {
    unsigned char buf[40];
    size_t len;
    assert(temp_resource == resource);
    response->code = COAP_RESPONSE_CODE(404);
    coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
    coap_add_option(response, COAP_OPTION_MAXAGE, coap_encode_var_safe(buf, sizeof(buf), 10), buf);
    if (query == NULL || coap_string_equal(query, coap_make_str_const("chip"))) {
        sample_temp();
        len = snprintf((char *)buf, sizeof(buf), "%.2f", temp_deg_c);
        coap_add_data(response, len, buf);
        response->code = COAP_RESPONSE_CODE(205); // or COAP_RESPONSE_CODE(404) if not available
    }
    if (temp_resource->subscribers && !temp_worker.user_data) {
        temp_worker.user_data = temp_resource;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &temp_worker, TEMP_WORKER_TIME_S * 1000);
    }
}

static void resource_get_led(coap_resource_t *resource, coap_session_t  *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response) {
    unsigned char buf[40];
    size_t len;

    response->code = COAP_RESPONSE_CODE(404);
    if (query == NULL || coap_string_equal(query, coap_make_str_const("onboard"))) {
        coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
        coap_add_option(response, COAP_OPTION_MAXAGE, coap_encode_var_safe(buf, sizeof(buf), 10), buf);
        response->code = COAP_RESPONSE_CODE(205);
        bool value;
        cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &value);
        len = snprintf((char *)buf, sizeof(buf), "%s", value ? "1" : "0");
        coap_add_data(response, len, buf);
    }
}

static void resource_put_led(coap_resource_t *resource, coap_session_t  *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response) {
    size_t size;
    const uint8_t *data;
    bool handled = false;
    bool changed = false;
    unsigned char buf[40];
    coap_get_data(request, &size, &data);
    if (size > 0 && (query == NULL || coap_string_equal(query, coap_make_str_const("onboard")))) {
        coap_str_const_t *value = coap_new_str_const(data, size);
        if (coap_string_equal(value, coap_make_str_const("1")) || coap_string_equal(value, coap_make_str_const("on"))) {
            bool led_gpio;
            cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &led_gpio);
            if (!led_gpio) {
                cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, true);
                changed = true;
            }
            handled = true;
        } else if (coap_string_equal(value, coap_make_str_const("0")) || coap_string_equal(value, coap_make_str_const("off"))) {
            bool led_gpio;
            cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &led_gpio);
            if (led_gpio) {
                cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, false);
                changed = true;
            }
            handled = true;
        }
        coap_delete_str_const(value);
    }
    if (handled) {
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
        if (changed) {
            notify_observers(resource);
        }
    } else {
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
        coap_add_data(response, 13, (const uint8_t *)"Invalid value");
    }
}

static void time_worker_func(async_context_t *context, async_at_time_worker_t *worker) {
    assert(worker->user_data);
    notify_observers(time_resource);
    if (time_resource->subscribers) {
        async_context_add_at_time_worker_in_ms(context, worker, 1000);
    } else {
        worker->user_data = NULL;
    }
}

static async_at_time_worker_t time_worker = {
    .do_work = time_worker_func
};

static void resource_get_time(coap_resource_t *resource, coap_session_t  *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response) {
    assert(time_resource == resource);
    unsigned char buf[11];
    response->code = COAP_RESPONSE_CODE(404);
    if ((query == NULL || coap_string_equal(query, coap_make_str_const("time")))) {
        coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
        coap_add_option(response, COAP_OPTION_MAXAGE, coap_encode_var_safe(buf, sizeof(buf), 1), buf);
        response->code = COAP_RESPONSE_CODE(205);
        snprintf((char*)buf, sizeof(buf), "%u", to_ms_since_boot(get_absolute_time()) / 1000);
        coap_add_data(response, strlen((char*)buf), buf);
    }
    if (time_resource->subscribers && !time_worker.user_data) {
        time_worker.user_data = time_resource;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &time_worker, 1000);
    }
}


static bool init_coap_resources(coap_context_t *ctx) {
    // chip temp
    temp_resource = coap_resource_init(coap_make_str_const("temp"), 0);
    assert(temp_resource);
    if (!temp_resource) {
        return false;
    }
    coap_resource_set_get_observable(temp_resource, true);
    coap_register_handler(temp_resource, COAP_REQUEST_GET, resource_get_temp);
    coap_add_attr(temp_resource, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(temp_resource, coap_make_str_const("rt"), coap_make_str_const("\"chip\""), 0);
    coap_add_attr(temp_resource, coap_make_str_const("if"), coap_make_str_const("\"temp\""), 0);
    coap_add_resource(ctx, temp_resource);
    // onboard led
    led_resource = coap_resource_init(coap_make_str_const("led"), 0);
    assert(led_resource);
    if (!led_resource) {
        return false;
    }
    coap_resource_set_get_observable(led_resource, true);
    coap_register_handler(led_resource, COAP_REQUEST_GET, resource_get_led);
    coap_register_handler(led_resource, COAP_REQUEST_PUT, resource_put_led);
    coap_add_attr(led_resource, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(led_resource, coap_make_str_const("rt"), coap_make_str_const("\"onboard\""), 0);
    coap_add_attr(led_resource, coap_make_str_const("if"), coap_make_str_const("\"led\""), 0);
    coap_add_resource(ctx, led_resource);
    // time
    time_resource = coap_resource_init(coap_make_str_const("time"), 0);
    assert(time_resource);
    if (!time_resource) {
        return false;
    }
    coap_resource_set_get_observable(time_resource, true);
    coap_register_handler(time_resource, COAP_REQUEST_GET, resource_get_time);
    coap_add_attr(time_resource, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(time_resource, coap_make_str_const("rt"), coap_make_str_const("\"time\""), 0);
    coap_add_attr(time_resource, coap_make_str_const("if"), coap_make_str_const("\"seconds\""), 0);
    coap_add_resource(ctx, time_resource);
    return true;
}

static coap_context_t* start_server(void)
{
    coap_startup();

#if 1
    coap_set_log_level(COAP_LOG_DEBUG);
#else
    coap_set_log_level(COAP_LOG_INFO);
#endif

    coap_context_t *ctx = coap_new_context(NULL);
    assert(ctx);
    if (!ctx) {
        coap_log_warn("failed to create main context\n");
        return NULL;
    }

    coap_address_t addr;
    addr.addr = netif_list->ip_addr;
    coap_address_set_port(&addr, COAP_PORT);

#ifdef COAP_CERT_INC
    // Set up security stuff
    static const uint8_t coap_key[] = COAP_KEY;
    static const uint8_t coap_pub_key[] = COAP_PUB_KEY;
    static coap_dtls_pki_t dtls_pki;
    memset(&dtls_pki, 0, sizeof(dtls_pki));
    dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
    dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM_BUF;
    dtls_pki.is_rpk_not_cert = 1; // tinydtls only supports raw public keys
    dtls_pki.verify_peer_cert = 1; // not implemented?
    dtls_pki.pki_key.key.pem_buf.public_cert = coap_pub_key; // rpk
    dtls_pki.pki_key.key.pem_buf.public_cert_len = sizeof(coap_pub_key);
    dtls_pki.pki_key.key.pem_buf.private_key = coap_key;
    dtls_pki.pki_key.key.pem_buf.private_key_len = sizeof(coap_key);
    if (!coap_context_set_pki(ctx, &dtls_pki)) {
        coap_log_err("Unable to set up keys\n");
        coap_free_context(ctx);
        return NULL;
    }
#endif
    coap_endpoint_t *ep = coap_new_endpoint(ctx, &addr, COAP_PROTO);
    assert(ep);
    if (!ep) {
        coap_log_warn("cannot create endpoint\n");
        coap_free_context(ctx);
        return NULL;
    }
    assert(MEMP_NUM_COAPSESSION > 1);
    coap_context_set_max_idle_sessions(ctx, MEMP_NUM_COAPSESSION - 1);
    coap_context_set_session_timeout(ctx, SESSION_TIMEOUT);
    init_coap_resources(ctx);
    return ctx;
}

static void coap_server_deinit(void)
{
    coap_free_context(main_coap_context);
    main_coap_context = NULL;
    coap_cleanup();
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi... (press 'd' to disconnect)\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }
    cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM);

    main_coap_context = start_server();
    if (!main_coap_context) {
        panic("Test failed");
    }
#if LWIP_IPV4
    coap_log_info("Ready, running coap server at %s \n", ipaddr_ntoa(&(netif_list->ip_addr)));
#else
    coap_log_info("Ready, running coap server at %s\n", ipaddr_ntoa(&(netif_list->ip6_addr[1])));
#endif

    // Initialise adc for the temp sensor
    adc_init();
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    adc_set_temp_sensor_enabled(true);

    check_notify_worker.user_data = main_coap_context;
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &check_notify_worker);
    coap_worker.user_data = main_coap_context;
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &coap_worker, 1000);
    while (true) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }

    coap_server_deinit();
    lwip_nosys_deinit(cyw43_arch_async_context());
    cyw43_arch_deinit();
    panic("Test passed");
    return 0;
}

void coap_address_init(coap_address_t *addr) {
  assert(addr);
  memset(addr, 0, sizeof(coap_address_t));
}
