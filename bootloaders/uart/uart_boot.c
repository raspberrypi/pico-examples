#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "hardware/flash.h"

// UART defines for uart boot
#define UART_ID uart1

// Use pins 4 and 5 for uart boot
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Use pin 3 for the RUN pin on the other chip
#define RUN_PIN 3


void reset_chip() {
    // Toggle run pin
    gpio_put(RUN_PIN, false);
    sleep_ms(1);
    gpio_put(RUN_PIN, true);
}


void uart_boot() {
    uint knocks = 0;
    char in = 0;
    while (true) {
        // Send the knock sequence
        uart_putc_raw(UART_ID, 0x56);
        uart_putc_raw(UART_ID, 0xff);
        uart_putc_raw(UART_ID, 0x8b);
        uart_putc_raw(UART_ID, 0xe4);
        uart_putc_raw(UART_ID, 'n');

        if (uart_is_readable_within_us(UART_ID, 1000)) {
            in = uart_getc(UART_ID);
            if (in != 'n') {
                printf("Incorrect response - resetting\n");
                reset_chip();
                return;
            }
            printf("%c\n", in);
            break;
        } else {
            if (knocks > 10) {
                printf("No response - resetting\n");
                reset_chip();
                return;
            }
            printf("No response - knocking again\n");
            knocks++;
        }
    }

    printf("Boot starting\n");

    // Get partition location in flash
    const int buf_words = (16 * 4) + 1;   // maximum of 16 partitions, each with maximum of 4 words returned, plus 1
    uint32_t* buffer = malloc(buf_words * 4);

    int ret = rom_get_partition_table_info(buffer, buf_words, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION | (0 << 24));
    hard_assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION));
    hard_assert(ret == 3);

    uint32_t location_and_permissions = buffer[1];
    uint32_t start_addr = XIP_BASE + ((location_and_permissions & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) * FLASH_SECTOR_SIZE;
    uint32_t end_addr = XIP_BASE + (((location_and_permissions & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) + 1) * FLASH_SECTOR_SIZE;
    printf("Start %08x, end %08x\n", start_addr, end_addr);

    free(buffer);

    printf("Writing binary\n");
    uint32_t time_start = time_us_32();
    uint32_t current_addr = start_addr;
    while (current_addr < end_addr) {
        uart_putc_raw(UART_ID, 'w');
        char *buf = (char*)current_addr;
        int i;
        for (i = 0; i < 32; i++) {
            uart_putc_raw(UART_ID, buf[i]);
        }
        if (!uart_is_readable_within_us(UART_ID, 500)) {
            // Detect hangs and reset the chip
            printf("Write has hung - resetting\n");
            reset_chip();
            return;
        }
        in = uart_getc(UART_ID);
        if (in != 'w') {
            printf("Incorrect response - resetting\n");
            reset_chip();
            return;
        }
        current_addr += i;
    }

    uint32_t time_end = time_us_32();
    printf("Write took %dus\n", time_end - time_start);
    printf("Write complete - resetting pointer\n");

    uart_putc_raw(UART_ID, 'c');
    if (!uart_is_readable_within_us(UART_ID, 500)) {
        // Detect hangs and reset the chip
        printf("Clear has hung - resetting\n");
        reset_chip();
        return;
    }
    in = uart_getc(UART_ID);
    printf("%c\n", in);
    if (in != 'c') {
        printf("Incorrect response - resetting\n");
        reset_chip();
        return;
    }

    printf("Verifying binary\n");
    time_start = time_us_32();
    current_addr = start_addr;
    while (current_addr < end_addr) {
        uart_putc_raw(UART_ID, 'r');
        char *buf = (char*)current_addr;
        if (!uart_is_readable_within_us(UART_ID, 500)) {
            // Detect hangs and reset the chip
            printf("Verify has hung - resetting\n");
            reset_chip();
            return;
        }
        int i = 0;
        while (uart_is_readable_within_us(UART_ID, 10) && i < 32) {
            in = uart_getc(UART_ID);
            if (in != buf[i]) {
                printf("Verify has incorrect data at 0x%08x - resetting\n", current_addr - start_addr + SRAM_BASE);
                reset_chip();
                return;
            }
            i++;
        }
        if (i != 32) {
            printf("Verify has incorrect data size - resetting\n");
            reset_chip();
            return;
        }
        in = uart_getc(UART_ID);
        if (in != 'r') {
            printf("Incorrect response - resetting\n");
            reset_chip();
            return;
        }
        current_addr += i;
    }

    time_end = time_us_32();
    printf("Verify took %dus\n", time_end - time_start);
    printf("Verify complete - executing\n");

    uart_putc_raw(UART_ID, 'x');
    if (!uart_is_readable_within_us(UART_ID, 500)) {
        // Detect hangs and reset the chip
        printf("Execute has hung - resetting\n");
        reset_chip();
        return;
    }
    in = uart_getc(UART_ID);
    printf("%c\n", in);
    if (in != 'x') {
        printf("Incorrect response - resetting\n");
        reset_chip();
        return;
    }
}


int main()
{
    stdio_init_all();

    // Set up our UART for booting the other device
    uart_init(UART_ID, 1000000);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set up run pin
    gpio_init(RUN_PIN);
    gpio_set_dir(RUN_PIN, GPIO_OUT);

    // Reset chip
    reset_chip();

    int attempts = 0;
    char splash[] = "RP2350";
    char hello[] = "Hello, world";

    while (true) {
        char buf[500] = {0};
        int i = 0;
        while (uart_is_readable(UART_ID) && i < sizeof(buf)) {
            char in = uart_getc(UART_ID);
            printf("%c", in);
            buf[i] = in;
            i++;
        }
        if (i > 0) {
            printf(" ...Read done\n");
        }
        char *ptr = memchr(buf, splash[0], sizeof(buf));
        if (ptr && strncmp(ptr, splash, sizeof(splash) - 1) == 0) {
            printf("Splash found\n");
            uart_boot();
            attempts = 0;
        } else {
            ptr = memchr(buf, hello[0], sizeof(buf));
            if (ptr && strncmp(ptr, hello, sizeof(hello) - 1) == 0) {
                printf("Device is running\n");
                attempts = 0;
            } else {
                if (attempts > 3) {
                    printf("Device not running - attempting reset\n");
                    reset_chip();
                    attempts = 0;
                } else {
                    printf("Device not running - waiting\n");
                    attempts++;
                }
            }
        }
        sleep_ms(1000);
    }
}
