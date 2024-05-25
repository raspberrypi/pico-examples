//
// Created by elliot on 25/05/24.
//
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"


// Temperature
#define TEMPERATURE_UNITS 'C'

// WiFi
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWD"
// MQTT
#define PORT  1883
#define MQTT_CLIENT_ID "clientID"
#define MQTT_BROKER_IP "YOUR_BROKER_IP"



typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    uint8_t data[MQTT_OUTPUT_RINGBUF_SIZE];
    uint8_t topic[100];
    uint32_t len;
} MQTT_CLIENT_DATA_T;

MQTT_CLIENT_DATA_T *mqtt;



/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
float read_onboard_temperature(const char unit) {

    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}

void control_led(bool on) {
    // Public state on /state topic and on/off led board
    const char* message = on ? "On" : "Off";
    if (on)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    else
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    char state_topic[128];
    snprintf(state_topic, sizeof(state_topic), "%s/state", mqtt->topic);
    mqtt_publish(mqtt->mqtt_client_inst, state_topic, message, strlen(message), 0, 0, NULL, NULL);
}

void publish_temperature() {
    //Public temperature on /temperature topic
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.2f", temperature);
    mqtt_publish(mqtt->mqtt_client_inst, "/temperature", temp_str, strlen(temp_str), 0, 0, NULL, NULL);
}
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(mqtt_client->data, data, len);
    mqtt_client->len = len;
    mqtt_client->data[len] = '\0';
    //Stampo i messaggi e  i topic
    printf("Topic: %s, Message: %s\n", mqtt_client->topic, mqtt_client->data);


    if (strcmp(mqtt->topic, "/led") == 0)
    {
        if (strcmp((const char *)mqtt_client->data, "On") == 0)
            control_led(true);
        else if  (strcmp((const char *)mqtt_client->data, "Off") == 0)
            control_led(false);
    }

}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    strcpy(mqtt_client->topic, topic);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        mqtt_sub_unsub(client, "/led", 0, NULL, arg, 1);
        printf("Connected to the /led topic successfully\n");
    }
}
int main() {

    stdio_init_all();

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    mqtt = (MQTT_CLIENT_DATA_T*)calloc(1, sizeof(MQTT_CLIENT_DATA_T));
    if (!mqtt) {
        printf("Failed to inizialize MQTT client \n");
        return 1;
    }
    // MQTT CLIENT INFO
    mqtt->mqtt_client_info.client_id = MQTT_CLIENT_ID;
    mqtt->mqtt_client_info.keep_alive = 60; // Keep alive in secondi

    if (cyw43_arch_init()) {
        printf("Failed to inizialize CYW43\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Wi-Fi error\n");
        return 1;
    }
    printf("\nConnected to Wifi\n");

    ip_addr_t addr;
    if (!ip4addr_aton(MQTT_BROKER_IP, &addr)) {
        printf("MQTT ip Address not valid !\n");
        return 1;
    }

    mqtt->mqtt_client_inst = mqtt_client_new();
    if (!mqtt->mqtt_client_inst) {
        printf("MQTT client instance creation error\n");
        return 1;
    }

    if (mqtt_client_connect(mqtt->mqtt_client_inst, &addr, PORT, mqtt_connection_cb, mqtt, &mqtt->mqtt_client_info) != ERR_OK) {
        printf("MQTT broker connection error\n");
        return 1;
    }
    printf("Successfully connected to the MQTT broker\n");
    mqtt_set_inpub_callback(mqtt->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, mqtt);

    while (1) {
        publish_temperature(); // Public temperature every 5 sec
        sleep_ms(5000);
        tight_loop_contents();
    }

    return 0;
}