/*
 * Copyright (C) 2017 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

//#define BTSTACK_FILE__ "hog_mouse_demo.c"

// *****************************************************************************
/* EXAMPLE_START(hog_mouse_demo): HID Mouse LE
 */
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ble_pointer.h"

#include "btstack.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"

#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "pico/binary_info.h"

// for mpu6050
#include <mpu6050_i2c_lib.h>
#include <math.h>

#define ALPHA 0.05 // for complimentary filter, big alpha gives faster reponse but more noise

// FS values are 0, 1, 2, or 3
// For gyro, correspond to += 250, 500, 1000, 2000 deg/s
// For accel, correspond to += 2, 4, 8, 16g
#define GYRO_FS 0
#define ACCEL_FS 0

// how many readings taken to find gyro offsets
#define OFFSET_NUM 10000

//pins for buttons
#define LEFT_BUTTON_GPIO_NUM 15
#define RIGHT_BUTTON_GPIO_NUM 16

float roll;
float pitch;
float yaw;

float roll_offset;
float pitch_offset;
float yaw_offset;

// start in top left corner
int abs_x = 0;
int abs_y = 0;

// from USB HID Specification 1.1, Appendix B.2
const uint8_t hid_descriptor_mouse_boot_mode[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x85,  0x01,                    // Report ID 1

    0x09, 0x01,                    //   USAGE (Pointer)

    0xa1, 0x00,                    //   COLLECTION (Physical)

#if 1
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
#endif

#if 1
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
#endif

    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t l2cap_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name
    0x0a, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', ' ', 'M', 'o', 'u', 's', 'e',
    // 16-bit Service UUIDs
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Mouse (Category 15, Sub-Category 2)
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC2, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

static void hog_mouse_setup(void){

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
    }

    // setup l2cap
    l2cap_init();

    // setup SM: Display only
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();

    // setup HID Device service
    hids_device_init(0, hid_descriptor_mouse_boot_mode, sizeof(hid_descriptor_mouse_boot_mode));

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);

    // register for events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for connection parameter updates
    l2cap_event_callback_registration.callback = &packet_handler;
    l2cap_add_event_handler(&l2cap_event_callback_registration);

    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    hids_device_register_packet_handler(packet_handler);
}

// HID Report sending
static void send_report(uint8_t buttons, int8_t dx, int8_t dy){
    uint8_t report[] = { buttons, (uint8_t) dx, (uint8_t) dy };
    switch (protocol_mode){
        case 0:
            hids_device_send_boot_mouse_input_report(con_handle, report, sizeof(report));
            break;
        case 1:
            hids_device_send_input_report(con_handle, report, sizeof(report));
            break;
        default:
            break;
    }
}

static int dx;
static int dy;
static uint8_t buttons;

#define MOUSE_PERIOD_MS 15
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

static btstack_timer_source_t mousing_timer;
static int mousing_active = 0;

static void mousing_timer_handler(btstack_timer_source_t * ts){

    if (con_handle == HCI_CON_HANDLE_INVALID) {
        mousing_active = 0;
        return;
    }

    mpu6050_fusion_output(roll_offset, pitch_offset, yaw_offset, ALPHA, MOUSE_PERIOD_MS, &roll, &pitch, &yaw, GYRO_FS);

    // move proportional to roll/yaw and pitch (if within boundary, otherwise dont move)
    // int desired_x = SCREEN_WIDTH/2 + (SCREEN_WIDTH/2) * roll/90; // fully right at 90 degrees, fully left at -90 degrees
    int desired_x = SCREEN_WIDTH/2 + (SCREEN_WIDTH/2) * - yaw/45; // alternative x based on yaw rather than roll (prone to drift)
    int desired_y = SCREEN_HEIGHT/2 + (SCREEN_HEIGHT/2) * pitch/45; // fully top at 45 degrees, fully bottom at -45 degrees

    dx = desired_x - abs_x;
    dy = desired_y - abs_y;

    if (abs_x + dx < 0 || abs_x + dx > SCREEN_WIDTH) { // about to move off side of screen
        dx = 0;
    }

    if (abs_y + dy < 0 || abs_y + dy > SCREEN_HEIGHT) { // about to move off top/bottom of screen
        dy = 0;
    }

    abs_x += dx;
    abs_y += dy;

    // trigger send
    hids_device_request_can_send_now_event(con_handle);

    // set next timer
    btstack_run_loop_set_timer(ts, MOUSE_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

// IRQ handler for mouse buttons 
void button_irq_handler(uint gpio, uint32_t events) {
    if (gpio == LEFT_BUTTON_GPIO_NUM) {
        printf("left click!\n");
        if (!gpio_get(gpio)) {
            buttons = 1;
        } else {
            buttons = 0;
        }
    } else if (gpio == RIGHT_BUTTON_GPIO_NUM) {
        printf("right click!\n");
        if (!gpio_get(gpio)) {
            buttons = 2;
        } else {
            buttons = 0;
        }
    }
}

static void hid_embedded_start_mousing(void){
    if (mousing_active) return;
    mousing_active = 1;

    printf("Start mousing..\n");

    // set one-shot timer
    mousing_timer.process = &mousing_timer_handler;
    btstack_run_loop_set_timer(&mousing_timer, MOUSE_PERIOD_MS);
    btstack_run_loop_add_timer(&mousing_timer);
}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    uint16_t conn_interval;

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected\n");
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
        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            printf("L2CAP Connection Parameter Update Complete, response: %x\n", l2cap_event_connection_parameter_update_response_get_result(packet));
            break;
        case HCI_EVENT_META_GAP:
            switch (hci_event_gap_meta_get_subevent_code(packet)) {
                case GAP_SUBEVENT_LE_CONNECTION_COMPLETE:
                    // print connection parameters (without using float operations)
                    conn_interval = gap_subevent_le_connection_complete_get_conn_interval(packet);
                    printf("LE Connection Complete:\n");
                    printf("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                    printf("- Connection Latency: %u\n", gap_subevent_le_connection_complete_get_conn_latency(packet));
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    // print connection parameters (without using float operations)
                    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                    printf("LE Connection Update:\n");
                    printf("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                    printf("- Connection Latency: %u\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                    break;
                default:
                    break;
            }
            break;  
        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)){
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));
#ifndef HAVE_BTSTACK_STDIN
                    hid_embedded_start_mousing();
#endif
                    // request connection param update via L2CAP following Apple Bluetooth Design Guidelines
                    // gap_request_connection_parameter_update(con_handle, 12, 12, 4, 100);    // 15 ms, 4, 1s

                    // directly update connection params via HCI following Apple Bluetooth Design Guidelines
                    // gap_update_connection_parameters(con_handle, 12, 12, 4, 100);    // 60-75 ms, 4, 1s

                    break;
                case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
                    printf("Boot Keyboard Characteristic Subscribed %u\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_BOOT_MOUSE_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_boot_mouse_input_report_enable_get_con_handle(packet);
                    printf("Boot Mouse Characteristic Subscribed %u\n", hids_subevent_boot_mouse_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_PROTOCOL_MODE:
                    protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
                    printf("Protocol Mode: %s mode\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    send_report(buttons, dx, dy);
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

int main(void) {
    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning Example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return 1;
#endif

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    // mouse buttons
    gpio_init(LEFT_BUTTON_GPIO_NUM);
    gpio_init(RIGHT_BUTTON_GPIO_NUM);

    gpio_set_dir(LEFT_BUTTON_GPIO_NUM, GPIO_IN);
    gpio_set_dir(RIGHT_BUTTON_GPIO_NUM, GPIO_IN);

    gpio_pull_up(LEFT_BUTTON_GPIO_NUM);
    gpio_pull_up(RIGHT_BUTTON_GPIO_NUM);

    gpio_set_irq_enabled_with_callback(LEFT_BUTTON_GPIO_NUM, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &button_irq_handler);
    gpio_set_irq_enabled(RIGHT_BUTTON_GPIO_NUM, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    // First initialise mpu6050
    // initialise to min full scale(+=250deg/s and +=2g)
    mpu6050_initialise(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GYRO_FS, ACCEL_FS);
    mpu6050_reset();

    // get gyro offsets
    mpu6050_get_gyro_offset(OFFSET_NUM, &roll_offset, &pitch_offset, &yaw_offset, GYRO_FS);

    // get initial roll and pitch values based on accelerometer
    int16_t acceleration[3], gyro[3], temp;
    mpu6050_read_raw(acceleration, gyro, &temp);
    roll = atan2(acceleration[1] , acceleration[2]) * 57.3;
    pitch = atan2((- acceleration[0]) , sqrt(acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2])) * 57.3;

    // assume yaw starts at 0 degrees (no absolute reference)
    yaw = 0;

    // now setup BLE
    hog_mouse_setup();

#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_process);
#endif

    // turn on!
    hci_power_control(HCI_POWER_ON);

    while (true) {
        sleep_ms(1000);
    }
    return 0;
}
