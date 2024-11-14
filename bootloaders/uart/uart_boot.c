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
    while (true) {
        // Send the knock sequence
        uart_putc_raw(UART_ID, 0x56);
        uart_putc_raw(UART_ID, 0xff);
        uart_putc_raw(UART_ID, 0x8b);
        uart_putc_raw(UART_ID, 0xe4);
        uart_putc_raw(UART_ID, 'n');

        if (uart_is_readable_within_us(UART_ID, 1000)) {
            char in = uart_getc(UART_ID);
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
    assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION));
    assert(ret == 3);

    uint32_t location_and_permissions = buffer[1];
    uint32_t saddr = XIP_BASE + ((location_and_permissions >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * FLASH_SECTOR_SIZE;
    uint32_t eaddr = XIP_BASE + (((location_and_permissions >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) & 0x1fffu) + 1) * FLASH_SECTOR_SIZE;
    printf("Start %08x, end %08x\n", saddr, eaddr);

    free(buffer);

    printf("Writing binary\n");
    uint32_t tstart = time_us_32();
    uint32_t caddr = saddr;
    while (caddr < eaddr) {
        uart_putc_raw(UART_ID, 'w');
        char *buf = (char*)caddr;
        for (int i=0; i < 32; i++) {
            uart_putc_raw(UART_ID, buf[i]);
        }
        if (!uart_is_readable_within_us(UART_ID, 500)) {
            // Detect hangs and reset the chip
            printf("Write has hung - resetting\n");
            reset_chip();
            return;
        }
        char in = uart_getc(UART_ID);
        printf("%c\n", in);
        caddr += 32;
    }

    uint32_t tend = time_us_32();
    printf("Write took %dus\n", tend - tstart);
    printf("Write complete - executing\n");
    uart_putc_raw(UART_ID, 'x');
    if (!uart_is_readable_within_us(UART_ID, 500)) {
        // Detect hangs and reset the chip
        printf("Execute has hung - resetting\n");
        reset_chip();
        return;
    }
    char in = uart_getc(UART_ID);
    printf("%c\n", in);
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


    while (true) {
        char splash[] = "RP2350";
        char hello[] = "Hello";
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
        char *ptr = memchr(buf, 'R', sizeof(buf));
        if (ptr && strncmp(ptr, splash, sizeof(splash) - 1) == 0) {
            printf("Splash found\n");
            uart_boot();
        } else {
            ptr = memchr(buf, 'H', sizeof(buf));
            if (ptr && strncmp(ptr, hello, sizeof(hello) - 1) == 0) {
                printf("Device is running\n");
            } else {
                printf("Device not running - attempting reset\n");
                reset_chip();
            }
        }
        sleep_ms(1000);
    }
}
