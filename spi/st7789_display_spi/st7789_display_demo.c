#include "st7789_display_demo.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "fonts.h"
#include "chicken_sprite.h"
#include <stdlib.h>

static uint8_t rotation = 0;

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

static void st7789_init(void) {
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

static void st7789_set_rotation(uint8_t r) {
    rotation = r % 4;
    st7789_write_cmd(0x36);
    switch (rotation) {
        case 0: st7789_write_data(0x00); break;
        case 1: st7789_write_data(0x60); break;
        case 2: st7789_write_data(0xC0); break;
        case 3: st7789_write_data(0xA0); break;
    }
}

static void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    st7789_set_address_window(x, y, x, y);
    st7789_write_data16(color);
}

static void st7789_fill_screen(uint16_t color) {
    st7789_set_address_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    uint8_t buf[2] = { color >> 8, color & 0xFF };
    for (uint32_t i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        spi_write_blocking(SPI_PORT, buf, 2);
    }
    gpio_put(PIN_CS, 1);
}

static void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
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

static void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
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

static void st7789_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
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

void draw_chicken(uint16_t top_left_x, uint16_t top_left_y) {
    for (uint8_t row = 0; row < CHICKEN_HEIGHT; row++) {
        for (uint8_t col = 0; col < CHICKEN_WIDTH; col++) {
            uint16_t color = chicken_sprite[row][col];
            if (color != TRANSPARENT) {
                st7789_draw_pixel(top_left_x + col, top_left_y + row, color);
            }
        }
    }
}

static void draw_initial_screen() {

    // Cycle screen
    st7789_fill_screen(RED);
    sleep_ms(1000);
    st7789_fill_screen(GREEN);
    sleep_ms(1000);
    st7789_fill_screen(BLUE);
    sleep_ms(1000);
    st7789_fill_screen(BLACK);

    // Header
    st7789_draw_string(20, 10, "Pico Display Demo", WHITE, BLACK, 2);
    st7789_draw_line(10, 35, 310, 35, WHITE);

    // Placeholder lines -> adjust as necessary
    st7789_draw_string(20, 50, "Perhaps one day,", GREEN, BLACK, 1);
    st7789_draw_string(20, 85, "a hen will come out to play", MAGENTA, BLACK, 1);
    st7789_draw_string(20, 120, "and lead the world astray.", YELLOW, BLACK, 1);
    st7789_draw_string(20, 155, "She is coming now.", WHITE, BLACK, 1);
    st7789_draw_string(20, 190, "Beware the demon chicken,", CYAN, BLACK, 1);

    // Footer
    st7789_draw_string(10, 210, "the demon chicken is coming,", RED, BLACK, 1);
    st7789_draw_string(10, 230, "and you cannot escape her.", RED, BLACK, 1);
}

int main() {
    st7789_init();
    st7789_set_rotation(1); // Landscape
    draw_initial_screen();
    draw_chicken(240, 80); // optional chicken
    return 0;
}