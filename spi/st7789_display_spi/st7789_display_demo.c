#include "st7789.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "fonts.h"
#include "../config.h"
#include <stdlib.h>

static uint8_t rotation = 0;

void draw_potted_plant_right_half() {
    // Note: Heavily commented because this was hard.
    uint16_t pot_color = 0xA52A;    // Brown colour for pot
    uint16_t stem_color = 0x07E0;   // Green for stem and leaves
    uint16_t leaf_color = 0x07E0;   // Green for leaves
    uint16_t bg_color = 0x0000;     // Black background

    uint16_t center_y = 120;  // Center in y-axis
    uint16_t x_shift = 20;

    // Pot setup
    uint16_t pot_bottom_y = center_y + 60;  // 180y
    uint16_t pot_top_y = pot_bottom_y - 30; // 150y
    uint16_t pot_left_x = 220 + x_shift;    // 240x
    uint16_t pot_right_x = 260 + x_shift;   // 280x

    // Pot
    for (uint16_t x = pot_left_x; x < pot_right_x; x++) {
        for (uint16_t y = pot_top_y; y < pot_bottom_y; y++) {
            st7789_draw_pixel(x, y, pot_color);
        }
    }

    // Pot rim
    for (uint16_t x = pot_left_x - 5; x < pot_right_x + 5; x++) {
        for (uint16_t y = pot_top_y - 5; y < pot_top_y; y++) {
            st7789_draw_pixel(x, y, pot_color);
        }
    }

    // Stem setup
    uint16_t stem_x = (pot_left_x + pot_right_x) / 2;
    uint16_t stem_y_start = pot_top_y - 5;
    uint16_t stem_y_end = center_y - 30;

    // Stem
    st7789_draw_line(stem_x, stem_y_start, stem_x, stem_y_end, stem_color);

    // Leaves - offset is lower relative to stem top
    uint16_t leaf_base_y = stem_y_end + 30;
    uint16_t leaf_mid_y = leaf_base_y - 10;
    uint16_t leaf_top_y = leaf_mid_y - 10;

    // Left leaves
    st7789_draw_line(stem_x, leaf_base_y, stem_x - 15, leaf_mid_y, leaf_color);
    st7789_draw_line(stem_x, leaf_mid_y, stem_x - 20, leaf_top_y, leaf_color);

    // Right leaves
    st7789_draw_line(stem_x, leaf_base_y, stem_x + 15, leaf_mid_y, leaf_color);
    st7789_draw_line(stem_x, leaf_mid_y, stem_x + 20, leaf_top_y, leaf_color);

    // Asterisk leaves
    st7789_draw_char(stem_x - 8, leaf_mid_y + 2, '*', leaf_color, bg_color, 1);
    st7789_draw_char(stem_x + 8, leaf_mid_y + 2, '*', leaf_color, bg_color, 1);
    st7789_draw_char(stem_x, leaf_top_y - 2, '*', leaf_color, bg_color, 1);
}

static void st7789_write_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

static void st7789_write_data(uint8_t data) {
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

static void st7789_write_data16(uint16_t data) {
    uint8_t buf[2] = { data >> 8, data & 0xFF };
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, buf, 2);
    gpio_put(PIN_CS, 1);
}

static void st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7789_write_cmd(0x2A);
    st7789_write_data(x0 >> 8);
    st7789_write_data(x0 & 0xFF);
    st7789_write_data(x1 >> 8);
    st7789_write_data(x1 & 0xFF);

    st7789_write_cmd(0x2B);
    st7789_write_data(y0 >> 8);
    st7789_write_data(y0 & 0xFF);
    st7789_write_data(y1 >> 8);
    st7789_write_data(y1 & 0xFF);

    st7789_write_cmd(0x2C);
}

void st7789_init(void) {
    spi_init(SPI_PORT, 62.5 * 1000 * 1000);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_set_dir(PIN_BL, GPIO_OUT);

    gpio_put(PIN_BL, 1);

    gpio_put(PIN_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);

    st7789_write_cmd(0x36);
    st7789_write_data(0x00);

    st7789_write_cmd(0x3A);
    st7789_write_data(0x55);

    st7789_write_cmd(0x21);

    st7789_write_cmd(0x11);
    sleep_ms(120);

    st7789_write_cmd(0x29);
    sleep_ms(20);
}

void st7789_set_rotation(uint8_t r) {
    rotation = r % 4;
    st7789_write_cmd(0x36);
    switch (rotation) {
        case 0: st7789_write_data(0x00); break;
        case 1: st7789_write_data(0x60); break;
        case 2: st7789_write_data(0xC0); break;
        case 3: st7789_write_data(0xA0); break;
    }
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    st7789_set_address_window(x, y, x, y);
    st7789_write_data16(color);
}

void st7789_fill_screen(uint16_t color) {
    st7789_set_address_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    uint8_t buf[2] = { color >> 8, color & 0xFF };
    for (uint32_t i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_blocking(SPI_PORT, buf, 2);
    }
    gpio_put(PIN_CS, 1);
}

void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true) {
        st7789_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 126) return;
    const uint8_t *glyph = font_8x8[c - 32];
    for (int8_t i = 0; i < 8; i++) {
        uint8_t line = glyph[i];
        for (int8_t j = 0; j < 8; j++) {
            uint16_t px = x + j * size;
            uint16_t py = y + i * size;
            uint16_t col = (line & (1 << j)) ? color : bg;
            for (uint8_t sx = 0; sx < size; sx++) {
                for (uint8_t sy = 0; sy < size; sy++) {
                    st7789_draw_pixel(px + sx, py + sy, col);
                }
            }
        }
    }
}

void st7789_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    uint16_t start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 8 * size;
            x = start_x;
        } else {
            st7789_draw_char(x, y, *str, color, bg, size);
            x += 8 * size;
        }
        str++;
    }
}
