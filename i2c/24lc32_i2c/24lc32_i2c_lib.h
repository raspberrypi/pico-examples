#ifndef PICO_I2C_EEPROM_LIB_H
#define PICO_I2C_EEPROM_LIB_H

#include "hardware/i2c.h"

// Acknowledge polling can be used to see when a write cycle is complete
// Wait for the i2c slave to acknowledge our empty write
bool ack_poll(uint8_t i2c_addr, i2c_inst_t *i2c_instance);

// Read num bytes of memory starting at given address into result
bool read_eeprom(uint16_t address, uint8_t *result, uint16_t num, uint8_t i2c_addr, i2c_inst_t *i2c_instance);

// Write a single byte of data at a specified address in memory
bool byte_write_eeprom(uint16_t address, uint8_t data, uint8_t i2c_addr, i2c_inst_t *i2c_instance);

// Write a block of data to eeprom 
bool page_write_eeprom(uint16_t address, uint8_t *data, uint8_t num, uint8_t i2c_addr, i2c_inst_t *i2c_instance);

// Initialise i2c for given gpio at given frequency
void eeprom_init(int sda_pin, int scl_pin, int freq, i2c_inst_t *i2c_instance);

#endif