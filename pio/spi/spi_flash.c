/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pio_spi.h"

// This example uses PIO to erase, program and read back a SPI serial flash
// memory.

// ----------------------------------------------------------------------------
// Generic serial flash code

#define FLASH_PAGE_SIZE        256
#define FLASH_SECTOR_SIZE      4096

#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_READ         0x03
#define FLASH_CMD_STATUS       0x05
#define FLASH_CMD_WRITE_EN     0x06
#define FLASH_CMD_SECTOR_ERASE 0x20

#define FLASH_STATUS_BUSY_MASK 0x01

void flash_read(const pio_spi_inst_t *spi, uint32_t addr, uint8_t *buf, size_t len) {
    uint8_t cmd[4] = {
            FLASH_CMD_READ,
            addr >> 16,
            addr >> 8,
            addr
    };
    gpio_put(spi->cs_pin, 0);
    pio_spi_write8_blocking(spi, cmd, 4);
    pio_spi_read8_blocking(spi, buf, len);
    gpio_put(spi->cs_pin, 1);
}


void flash_write_enable(const pio_spi_inst_t *spi) {
    uint8_t cmd = FLASH_CMD_WRITE_EN;
    gpio_put(spi->cs_pin, 0);
    pio_spi_write8_blocking(spi, &cmd, 1);
    gpio_put(spi->cs_pin, 1);
}

void flash_wait_done(const pio_spi_inst_t *spi) {
    uint8_t status;
    do {
        gpio_put(spi->cs_pin, 0);
        uint8_t cmd = FLASH_CMD_STATUS;
        pio_spi_write8_blocking(spi, &cmd, 1);
        pio_spi_read8_blocking(spi, &status, 1);
        gpio_put(spi->cs_pin, 1);
    } while (status & FLASH_STATUS_BUSY_MASK);
}

void flash_sector_erase(const pio_spi_inst_t *spi, uint32_t addr) {
    uint8_t cmd[4] = {
            FLASH_CMD_SECTOR_ERASE,
            addr >> 16,
            addr >> 8,
            addr
    };
    flash_write_enable(spi);
    gpio_put(spi->cs_pin, 0);
    pio_spi_write8_blocking(spi, cmd, 4);
    gpio_put(spi->cs_pin, 1);
    flash_wait_done(spi);
}

void flash_page_program(const pio_spi_inst_t *spi, uint32_t addr, uint8_t data[]) {
    flash_write_enable(spi);
    uint8_t cmd[4] = {
            FLASH_CMD_PAGE_PROGRAM,
            addr >> 16,
            addr >> 8,
            addr
    };
    gpio_put(spi->cs_pin, 0);
    pio_spi_write8_blocking(spi, cmd, 4);
    pio_spi_write8_blocking(spi, data, FLASH_PAGE_SIZE);
    gpio_put(spi->cs_pin, 1);
    flash_wait_done(spi);
}

// ----------------------------------------------------------------------------
// Example program

void printbuf(const uint8_t buf[FLASH_PAGE_SIZE]) {
    for (int i = 0; i < FLASH_PAGE_SIZE; ++i)
        printf("%02x%c", buf[i], i % 16 == 15 ? '\n' : ' ');
}

int main() {
    stdio_init_all();
#if !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning pio/spi/spi_flash example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    puts("PIO SPI Example");

    pio_spi_inst_t spi = {
            .pio = pio0,
            .sm = 0,
            .cs_pin = PICO_DEFAULT_SPI_CSN_PIN
    };

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);

    uint offset = pio_add_program(spi.pio, &spi_cpha0_program);
    printf("Loaded program at %d\n", offset);

    pio_spi_init(spi.pio, spi.sm, offset,
                 8,       // 8 bits per SPI frame
                 31.25f,  // 1 MHz @ 125 clk_sys
                 false,   // CPHA = 0
                 false,   // CPOL = 0
                 PICO_DEFAULT_SPI_SCK_PIN,
                 PICO_DEFAULT_SPI_TX_PIN,
                 PICO_DEFAULT_SPI_RX_PIN
    );
    // Make the 'SPI' pins available to picotool
    bi_decl(bi_4pins_with_names(PICO_DEFAULT_SPI_RX_PIN, "SPI RX", PICO_DEFAULT_SPI_TX_PIN, "SPI TX", PICO_DEFAULT_SPI_SCK_PIN, "SPI SCK", PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    uint8_t page_buf[FLASH_PAGE_SIZE];

    const uint32_t target_addr = 0;

    flash_sector_erase(&spi, target_addr);
    flash_read(&spi, target_addr, page_buf, FLASH_PAGE_SIZE);

    puts("After erase:");
    printbuf(page_buf);

    for (int i = 0; i < FLASH_PAGE_SIZE; ++i)
        page_buf[i] = i;
    flash_page_program(&spi, target_addr, page_buf);
    flash_read(&spi, target_addr, page_buf, FLASH_PAGE_SIZE);

    puts("After program:");
    printbuf(page_buf);

    flash_sector_erase(&spi, target_addr);
    flash_read(&spi, target_addr, page_buf, FLASH_PAGE_SIZE);

    puts("Erase again:");
    printbuf(page_buf);

    return 0;
#endif
}
