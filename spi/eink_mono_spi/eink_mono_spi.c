/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

 /* Example code to talk to an SSD1681-based monochrome e-ink display.

    Connections on Raspberry Pi Pico board and an Adafruit 200x200 monochrome
    e-ink display available in examples documentation (README.adoc).

    Note: SPI devices can have a number of different naming schemes for pins. See
    the Wikipedia page at https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
    for variations. This board uses 4 wire SPI.

    Note this display also has onboard SRAM, which we use as a buffer.
 */

#define DISPLAY_HEIGHT 200
#define DISPLAY_WIDTH 200
#define FRAME_BUFLEN (DISPLAY_HEIGHT * DISPLAY_WIDTH) / 8

#define RST_PIN 11
#define BUSY_PIN 12
#define ENA_PIN 13
#define DC_PIN 20
#define SRCS_PIN 21
#define SDCS_PIN 22

#define SSD1681_DRIVER_CONTROL _u(0x01)
#define SSD1681_GATE_VOLTAGE _u(0x03)
#define SSD1681_SOURCE_VOLTAGE _u(0x04)
#define SSD1681_INIT_SETTING _u(0x08)
#define SSD1681_INIT_WRITE_REG _u(0x09)
#define SSD1681_INIT_READ_REG _u(0x0A)
#define SSD1681_BOOSTER_SOFT_START _u(0x0C)
#define SSD1681_DEEP_SLEEP _u(0x10)
#define SSD1681_DATA_MODE _u(0x11)
#define SSD1681_SW_RESET _u(0x12)
#define SSD1681_HV_DETECT _u(0x14)
#define SSD1681_VCI_DETECT _u(0x15)
#define SSD1681_TEMP_CONTROL _u(0x18)
#define SSD1681_TEMP_WRITE _u(0x1A)
#define SSD1681_TEMP_READ _u(0x1B)
#define SSD1681_EXTTEMP_WRITE _u(0x1C)
#define SSD1681_MASTER_ACTIVATE _u(0x20)
#define SSD1681_DISP_CTRL1 _u(0x21)
#define SSD1681_DISP_CTRL2 _u(0x22)
#define SSD1681_WRITE_BWRAM _u(0x24)
#define SSD1681_WRITE_REDRAM _u(0x26)
#define SSD1681_READ_RAM _u(0x27)
#define SSD1681_VCOM_SENSE _u(0x28)
#define SSD1681_VCOM_DURATION _u(0x29)
#define SSD1681_WRITE_VCOM_OTP _u(0x2A)
#define SSD1681_WRITE_VCOM_CTRL _u(0x2B)
#define SSD1681_WRITE_VCOM_REG _u(0x2C)
#define SSD1681_READ_OTP _u(0x2D)
#define SSD1681_READ_USERID _u(0x2E)
#define SSD1681_READ_STATUS _u(0x2F)
#define SSD1681_WRITE_WS_OTP _u(0x30)
#define SSD1681_LOAD_WS_OTP _u(0x31)
#define SSD1681_WRITE_LUT _u(0x32)
#define SSD1681_CRC_CALC _u(0x34)
#define SSD1681_CRC_READ _u(0x35)
#define SSD1681_PROG_OTP _u(0x36)
#define SSD1681_WRITE_DISPLAY_OPT _u(0x37)
#define SSD1681_WRITE_USERID _u(0x38)
#define SSD1681_OTP_PROGMODE _u(0x39)
#define SSD1681_WRITE_BORDER _u(0x3C)
#define SSD1681_END_OPTION _u(0x3F)
#define SSD1681_SET_RAMXPOS _u(0x44)
#define SSD1681_SET_RAMYPOS _u(0x45)
#define SSD1681_AUTOWRITE_RED _u(0x46)
#define SSD1681_AUTOWRITE_BW _u(0x47)
#define SSD1681_SET_RAMXCOUNT _u(0x4E)
#define SSD1681_SET_RAMYCOUNT _u(0x4F)
#define SSD1681_NOP _u(0xFF)

 // write bit 0x00
#define SSD1681_READ_BIT _u(0x01)

#ifdef PICO_DEFAULT_SPI_CSN_PIN
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}
#endif

#if defined(spi_default) && defined(PICO_DEFAULT_SPI_CSN_PIN)

void ssd1681_send_cmd(uint8_t cmd, bool nostop) {
    // send a command byte to the display
    // read/write procedure in datasheet

    // pull CS and DC low + send command
    cs_select();
    gpio_put(DC_PIN, 0);
    spi_write_blocking(spi_default, &cmd, 1);
    if (!nostop) {
        // pull CS high
        cs_deselect();
    }
}

void ssd1681_send_cmd_with_data(uint8_t cmd, uint8_t data) {
    // send a command byte and extra data
    ssd1681_send_cmd(cmd, true);
    gpio_put(DC_PIN, 1);
    spi_write_blocking(spi_default, &data, 1);
    cs_deselect();
}


void ssd1681_send_cmd_with_databuf(uint8_t cmd, uint8_t buf[], int buflen) {
    // send a command byte and extra data in a buffer
    ssd1681_send_cmd(cmd, true);
    gpio_put(DC_PIN, 1);
    spi_write_blocking(spi_default, buf, buflen);
    cs_deselect();
}

void ssd1681_read(uint8_t cmd, uint8_t* buf, uint16_t len) {
    ssd1681_send_cmd(cmd, true);
    spi_read_blocking(spi_default, 0, buf, len);
    cs_deselect();
}

void ssd1681_hw_reset() {
    // perform a hardware reset by toggling the RST pin
    gpio_put(RST_PIN, 0);
    sleep_ms(10);
    gpio_put(RST_PIN, 1);
    sleep_ms(10);
}

void ssd1681_busy_wait() {
    // wait until the display is ready for the next task by polling busy pin
    bool is_busy = gpio_get(BUSY_PIN);
    while (is_busy) {
        sleep_ms(10);
        is_busy = gpio_get(BUSY_PIN);
    }
}

void ssd1681_set_ram_window(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    uint8_t buf[5];

    // ram x start and end
    buf[0] = x1;
    buf[1] = x2;
    ssd1681_send_cmd_with_databuf(SSD1681_SET_RAMXPOS, buf, 2);

    // ram y start and end
    buf[0] = y1;
    buf[1] = y1 >> 8;

    buf[2] = y2;
    buf[3] = y2 >> 8;
    ssd1681_send_cmd_with_databuf(SSD1681_SET_RAMYPOS, buf, 4);
}

void ssd1681_set_ram_counters(uint8_t xpos, uint8_t ypos) {
    uint8_t buf[2];

    buf[0] = xpos;
    ssd1681_send_cmd_with_databuf(SSD1681_SET_RAMXCOUNT, buf, 1);

    buf[0] = ypos;
    buf[1] = ypos >> 8;
    ssd1681_send_cmd_with_databuf(SSD1681_SET_RAMYCOUNT, buf, 2);
}

void ssd1681_init() {
    // initialize SSD1681 driver

    sleep_ms(10); // sleep after module power on
    ssd1681_hw_reset();
    ssd1681_busy_wait();
    ssd1681_send_cmd(SSD1681_SW_RESET, false);
    sleep_ms(10);

    // driver output control
    uint8_t driver_control_data[] = { DISPLAY_WIDTH - 1, (DISPLAY_WIDTH - 1) >> 8, 0x00 };
    ssd1681_send_cmd_with_databuf(SSD1681_DRIVER_CONTROL, driver_control_data, 3);

    // set data entry mode
    ssd1681_send_cmd_with_data(SSD1681_DATA_MODE, 0x03);

    // set display RAM size 
    ssd1681_set_ram_window(0, 0, (DISPLAY_HEIGHT / 8 - 1), DISPLAY_WIDTH - 1);

    // set panel border waveform
    ssd1681_send_cmd_with_data(SSD1681_WRITE_BORDER, 0x05);

    // set temp control
    ssd1681_send_cmd_with_data(SSD1681_TEMP_CONTROL, 0x80);

    // set ram address counters
    ssd1681_set_ram_counters(0, 0);

    //ssd1681_send_cmd_with_data(SSD1681_DISP_CTRL2, 0xF7);
    //ssd1681_send_cmd(SSD1681_MASTER_ACTIVATE, false);

    ssd1681_busy_wait();
}

void ssd1681_sleep() {
    // put the display in deep sleep mode when not changing display contents
    const uint8_t sleep_data = { 0x01 };
    ssd1681_send_cmd_with_data(SSD1681_DEEP_SLEEP, sleep_data);
    sleep_ms(10);
}

void ssd1681_update() {
    ssd1681_send_cmd_with_data(SSD1681_DISP_CTRL2, 0xF7);
    ssd1681_send_cmd(SSD1681_MASTER_ACTIVATE, false);
    ssd1681_busy_wait();
}

void ssd1681_render(uint8_t buf1[], uint8_t buf2[]) {
    ssd1681_init();
    ssd1681_set_ram_counters(0, 0);
    ssd1681_send_cmd_with_databuf(SSD1681_WRITE_BWRAM, buf1, FRAME_BUFLEN);
    ssd1681_set_ram_counters(0, 0);
    ssd1681_send_cmd_with_databuf(SSD1681_WRITE_REDRAM, buf2, FRAME_BUFLEN);
    ssd1681_update();
    printf("HELLO2!!!");
    ssd1681_sleep();
}
#endif


int main() {
    stdio_init_all();
#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
    #warning spi / eink_mono_spi example requires a board with SPI pins
        puts("Default SPI pins were not defined");
#else

    printf("Hello, e-ink display! Displaying something..\n");

    // this example will use SPI0 at 4MHz.
    spi_init(spi_default, 4 * 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_init(SRCS_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_set_dir(SRCS_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    gpio_put(SRCS_PIN, 1);
    // make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "Display SPI CS"));
    bi_decl(bi_1pin_with_name(SRCS_PIN, "SRAM SPI CS"));

    // initialize DC, RST, BUSY, ENA pins
    gpio_init(DC_PIN);
    gpio_init(RST_PIN);
    gpio_init(BUSY_PIN);
    gpio_init(ENA_PIN);
    gpio_set_dir(DC_PIN, GPIO_OUT);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    gpio_set_dir(BUSY_PIN, GPIO_IN);
    gpio_set_dir(ENA_PIN, GPIO_OUT);
    gpio_put(DC_PIN, 0);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(RST_PIN, "Display D/C"));
    bi_decl(bi_1pin_with_name(RST_PIN, "Display RST"));
    bi_decl(bi_1pin_with_name(BUSY_PIN, "Display ENA"));
    bi_decl(bi_1pin_with_name(ENA_PIN, "Display BUSY"));


    uint8_t bw_buf[FRAME_BUFLEN] = { 0 };
    uint8_t red_buf[FRAME_BUFLEN] = { 0 };

    ssd1681_render(bw_buf, red_buf);

    return 0;
#endif
}
