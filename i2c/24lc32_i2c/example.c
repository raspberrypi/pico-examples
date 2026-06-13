#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "24lc32_i2c_lib.h"

// Default I2C address of 24LC32
#define I2C_ADDRESS 0x50

int main() {
    stdio_init_all();

    #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
        #warning 24lc32_i2c example requires a board with i2c pins
            puts("Default I2C pins were not defined");
        return 0;
    #else
        // useful information for picotool
        bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
        bi_decl(bi_program_description("24LC32 I2C example for the Raspberry Pi Pico"));

        eeprom_init(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, 400*1000, i2c_default);

        printf("24LC32 I2C EEPROM example\nStarting test...\n\n");

        // Write the byte at location 0x00 to 0
        byte_write_eeprom(0x00, 0, I2C_ADDRESS, i2c_default);

        // Make an array of integers from 1 to 31 and write the array to eeprom starting at 0x01
        uint8_t data_list[31];
        for (int i = 0; i < 31; i++) {
            data_list[i] = i + 1;
        }
        page_write_eeprom(0x01, data_list, 31, I2C_ADDRESS, i2c_default);

        // Read eeprom, and ensure we observe ascending integers from 0 to 31
        uint8_t result[32];
        read_eeprom(0x0, result, 32, I2C_ADDRESS, i2c_default);

        for (int i = 0; i < 32; i++) {
            printf("read: %i, expected: %i\n", result[i], i);
            if (result[i] != i) {
                printf("Unexpected read!\n");
                return 1;
            }
        }
        
        // Now re-write and re-read eeprom
        printf("\nReversing order...\n\n");
        for (int i = 0; i < 31; i++) {
            data_list[i] = 31 - i;
        }
        page_write_eeprom(0x00, data_list, 31, I2C_ADDRESS, i2c_default);
        byte_write_eeprom(0x1F, 0, I2C_ADDRESS, i2c_default);

        // Read eeprom, and ensure we observe descending integers from 31 to 0
        read_eeprom(0x0, result, 32, I2C_ADDRESS, i2c_default);

        for (int i = 0; i < 32; i++) {
            printf("read: %i, expected: %i\n", result[i], 31 - i);
            if (result[i] != 31 - i) {
                printf("Unexpected read!\n");
                return 1;
            }
        }
        
        printf("Test passed!\n");
        return 0;
    #endif
}