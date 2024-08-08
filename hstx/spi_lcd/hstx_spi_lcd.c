// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Drive a ST7789 SPI LCD using the HSTX. The SPI clock rate is fully
// independent of (and can be faster than) the system clock.

// You'll need an LCD module for this example. It was tested with: WaveShare
// 1.3 inch ST7789 module. Wire up the signals as per PIN_xxx defines below,
// and don't forget to connect GND and VCC to GND/3V3 on your board!
//
// Theory of operation: Each 32-bit HSTX record contains 3 x 8-bit fields:
//
// 27:20  CSn x8    (noninverted CSn pin)
// 17:10  !DC x 8   (inverted DC pin)
//  7: 0  data bits (DIN pin)
//
// SCK is driven by the HSTX clock generator. We do issue extra clocks whilst
// CSn is high, but this should be ignored by the display. Packing the
// control lines in the HSTX FIFO records makes it easy to drive them in sync
// with SCK without having to reach around and do manual GPIO wiggling.

#include "pico/stdlib.h"
#include "hardware/resets.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"

// These can be any permutation of HSTX-capable pins:
#define PIN_DIN   12
#define PIN_SCK   13
#define PIN_CS    14
#define PIN_DC    15
// These can be any pin:
#define PIN_RESET 16
#define PIN_BL    17

#define FIRST_HSTX_PIN 12
#if   PIN_DIN < FIRST_HSTX_PIN || PIN_DIN >= FIRST_HSTX_PIN + 8
#error "Must be an HSTX-capable pin: DIN"
#elif PIN_SCK < FIRST_HSTX_PIN || PIN_SCK >= FIRST_HSTX_PIN + 8
#error "Must be an HSTX-capable pin: SCK"
#elif PIN_CS  < FIRST_HSTX_PIN || PIN_CS  >= FIRST_HSTX_PIN + 8
#error "Must be an HSTX-capable pin: CS"
#elif PIN_DC  < FIRST_HSTX_PIN || PIN_DC  >= FIRST_HSTX_PIN + 8
#error "Must be an HSTX-capable pin: DC"
#endif

static inline void hstx_put_word(uint32_t data) {
	while (hstx_fifo_hw->stat & HSTX_FIFO_STAT_FULL_BITS)
		;
	hstx_fifo_hw->fifo = data;
}

static inline void lcd_put_dc_cs_data(bool dc, bool csn, uint8_t data) {
	hstx_put_word(
		(uint32_t)data |
		(csn ? 0x0ff00000u : 0x00000000u) |
		// Note DC gets inverted inside of HSTX:
		(dc  ? 0x00000000u : 0x0003fc00u)
	);
}

static inline void lcd_start_cmd(uint8_t cmd) {
	lcd_put_dc_cs_data(false, true, 0);
	lcd_put_dc_cs_data(false, false, cmd);
}

static inline void lcd_put_data(uint32_t data) {
	lcd_put_dc_cs_data(true, false, data);
}

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
static const uint8_t st7789_init_seq[] = {
        1, 20, 0x01,                        // Software reset
        1, 10, 0x11,                        // Exit sleep mode
        2, 2, 0x3a, 0x55,                   // Set colour mode to 16 bit
        2, 0, 0x36, 0x00,                   // Set MADCTL: row then column, refresh is bottom to top ????
        5, 0, 0x2a, 0x00, 0x00,             // CASET: column addresses
            SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff,
        5, 0, 0x2b, 0x00, 0x00,             // RASET: row addresses
            SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff,
        1, 2, 0x21,                         // Inversion on, then 10 ms delay (supposedly a hack?)
        1, 2, 0x13,                         // Normal display on, then 10 ms delay
        1, 2, 0x29,                         // Main screen turn on, then wait 500 ms
        0                                   // Terminate list
};

static inline void lcd_write_cmd(const uint8_t *cmd, size_t count) {
    lcd_start_cmd(*cmd++);
    if (count >= 2) {
        for (size_t i = 0; i < count - 1; ++i) {
            lcd_put_data(*cmd++);
        }
    }
}

static inline void lcd_init(const uint8_t *init_seq) {
    const uint8_t *cmd = init_seq;
    while (*cmd) {
        lcd_write_cmd(cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void lcd_start_pixels(void) {
    uint8_t cmd = 0x2c; // RAMWR
    lcd_write_cmd(&cmd, 1);
}

int main() {
    stdio_init_all();

    // Switch HSTX to USB PLL (presumably 48 MHz) because clk_sys is probably
    // running a bit too fast for this example -- 48 MHz means 48 Mbps on
    // PIN_DIN. Need to reset around clock mux change, as the AUX mux can
    // introduce short clock pulses:
    reset_block(RESETS_RESET_HSTX_BITS);
    hw_write_masked(
        &clocks_hw->clk[clk_hstx].ctrl,
        CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_HSTX_CTRL_AUXSRC_LSB,
        CLOCKS_CLK_HSTX_CTRL_AUXSRC_BITS
    );
    unreset_block_wait(RESETS_RESET_HSTX_BITS);

    gpio_init(PIN_RESET);
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_RESET, GPIO_OUT);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_RESET, 1);
    gpio_put(PIN_BL, 1);

    hstx_ctrl_hw->bit[PIN_SCK - FIRST_HSTX_PIN] =
        HSTX_CTRL_BIT0_CLK_BITS;

    hstx_ctrl_hw->bit[PIN_DIN - FIRST_HSTX_PIN] =
        (7u << HSTX_CTRL_BIT0_SEL_P_LSB) |
        (7u << HSTX_CTRL_BIT0_SEL_N_LSB);

    hstx_ctrl_hw->bit[PIN_CS - FIRST_HSTX_PIN] =
        (27u << HSTX_CTRL_BIT0_SEL_P_LSB) |
        (27u << HSTX_CTRL_BIT0_SEL_N_LSB);

    hstx_ctrl_hw->bit[PIN_DC - FIRST_HSTX_PIN] =
        (17u << HSTX_CTRL_BIT0_SEL_P_LSB) |
        (17u << HSTX_CTRL_BIT0_SEL_N_LSB) |
        (HSTX_CTRL_BIT0_INV_BITS);

    // We have packed 8-bit fields, so shift left 1 bit/cycle, 8 times.
    hstx_ctrl_hw->csr =
        HSTX_CTRL_CSR_EN_BITS |
        (31u << HSTX_CTRL_CSR_SHIFT_LSB) |
        (8u << HSTX_CTRL_CSR_N_SHIFTS_LSB) |
        (1u << HSTX_CTRL_CSR_CLKDIV_LSB);

    gpio_set_function(PIN_SCK, 0/*GPIO_FUNC_HSTX*/);
    gpio_set_function(PIN_DIN, 0/*GPIO_FUNC_HSTX*/);
    gpio_set_function(PIN_CS,  0/*GPIO_FUNC_HSTX*/);
    gpio_set_function(PIN_DC,  0/*GPIO_FUNC_HSTX*/);

    lcd_init(st7789_init_seq);

    uint t = 0;
    while (true) {
        lcd_start_pixels();
        for (uint y = 0; y < SCREEN_HEIGHT; ++y) {
            for (uint x = 0; x < SCREEN_WIDTH; ++x) {
                uint r = (x + t) & 0x1f;
                uint b = (y - t) & 0x1f;
                uint g = (x + y + (t >> 1)) & 0x3f;
                uint rgb = (r << 11) + (g << 5) + b;
                lcd_put_data(rgb >> 8);
                lcd_put_data(rgb & 0xff);
            }
        }
        ++t;
    }
}
