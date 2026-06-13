#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// This is the max number of bytes which can be written at once during a page write
#define MAX_PAGE_WRITE 32

// This is the 24LC32's capacity in bytes
#define EEPROM_SIZE 4096

// Acknowledge polling can be used to see when a write cycle is complete
// Wait for the i2c slave to acknowledge our empty write
bool ack_poll(uint8_t i2c_addr, i2c_inst_t *i2c_instance) {
    int timeout = 100;
    for (int i = 0; i < timeout; i++) {
        if (i2c_write_blocking(i2c_instance, i2c_addr, NULL, 1, false) == 1) {
            return true;
        }
        sleep_ms(1);
    }
    printf("ack timeout\n");
    return false;
}

// Read num bytes of memory starting at given address into result
bool read_eeprom(uint16_t address, uint8_t *result, uint16_t num, uint8_t i2c_addr, i2c_inst_t *i2c_instance) {
    if ((address + num - 1) > EEPROM_SIZE) {
        printf("ERROR: Tried to read memory outside EEPROM's range.\n");
        return false;
    }

    for (int i = 0; i < num; i++) {
        uint8_t byte1 = (uint8_t)(address >> 8);
        uint8_t byte2 = (uint8_t)(address);

        uint8_t buf[2] = {byte1, byte2};

        i2c_write_blocking(i2c_instance, i2c_addr, buf, 2, true);
        i2c_read_blocking(i2c_instance, i2c_addr, (result + i), 1, false);

        address++;
    }
    
    return true;
}

// Write a single byte of data at a specified address in memory
bool byte_write_eeprom(uint16_t address, uint8_t data, uint8_t i2c_addr, i2c_inst_t *i2c_instance) {
    if (address >= EEPROM_SIZE) {
        printf("ERROR: Tried to write to memory outside EEPROM's range.\n");
        return false;
    } 

    uint8_t byte1 = (uint8_t)(address >> 8);
    uint8_t byte2 = (uint8_t)(address);

    uint8_t buf[3] = {byte1, byte2, data};

    i2c_write_blocking(i2c_instance, i2c_addr, buf, 3, false);

    // ack_poll to wait for write to be complete
    return ack_poll(i2c_addr, i2c_instance);
}

// Write a block of data to eeprom 
bool page_write_eeprom(uint16_t address, uint8_t *data, uint8_t num, uint8_t i2c_addr, i2c_inst_t *i2c_instance) {
    if (num > MAX_PAGE_WRITE) {
        printf("ERROR: Tried to write more than %i bytes.\n", MAX_PAGE_WRITE);
        return false;
    } else if ((address + num - 1) > EEPROM_SIZE) {
        printf("ERROR: Tried to write to memory outside EEPROM's range.\n");
        return false;
    }

    uint8_t byte1 = (uint8_t)(address >> 8);
    uint8_t byte2 = (uint8_t)(address);
    uint8_t buf[2 + num];
    buf[0] = byte1;
    buf[1] = byte2;

    // transfer data into buffer
    for (int i = 0; i < num; i++) {
        buf[i + 2] = data[i];
    }

    i2c_write_blocking(i2c_instance, i2c_addr, buf, 2 + num, false);

    // ack_poll to wait for write to be complete
    return ack_poll(i2c_addr, i2c_instance);
}

// Initialise i2c for given gpio at given frequency
void eeprom_init(int sda_pin, int scl_pin, int freq, i2c_inst_t *i2c_instance) {
    i2c_init(i2c_instance, freq);
    
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

