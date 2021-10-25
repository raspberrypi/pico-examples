#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define bus i2c0
#define SDA 16
#define SCL 17
#define FIFO_SIZE 16

/**
 * @brief initialize i2c bus on specified address
 * 
 * @param address slave address (0x21 seems to be causing problem for some reasons I cannot understand)
 */
void init_i2c_bus(uint8_t address) {
	i2c_init(bus, 100000); // BUS SPEED IS 400kHz, Clock Stretching is enabled
	gpio_set_function(SCL, GPIO_FUNC_I2C);
	gpio_set_function(SDA, GPIO_FUNC_I2C);
	gpio_pull_up(SDA);
	gpio_pull_up(SCL);
	i2c_set_slave_mode(bus, true, address);
}


/**
 * @brief Listen for i²c commands in the RX FIFO. Wait until transmit from master side is finished.
 * 
 * @param i2c i2c instance
 * @param handler_function function to handle the 16 byte message that the slave can read
 */
void i2c_listener(i2c_inst_t *i2c, void (*handler_function)(i2c_inst_t*, uint8_t*)) {
	uint8_t buffer[FIFO_SIZE];
	memset(buffer, 0, FIFO_SIZE);
	int available;

	for( ;; ) {
		available = i2c_get_read_available(i2c);

		if( available > 0 ) {

			int old_available;
			do {
				old_available = available;
				sleep_us(100);
				available = i2c_get_read_available(i2c);
			} while( available != old_available );

			// read and execute handler function
			i2c_read_raw_blocking(i2c, buffer, available);
			handler_function(i2c, buffer);
			memset(buffer, 0, FIFO_SIZE);
		}
	}
}

/**
 * @brief Handler function called from the listener
 * 
 * @param i2c i2c instance
 * @param message 16 byte i2c message, the first byte is assumed to be the command byte
 */
void on_message(i2c_inst_t *i2c, uint8_t *message) {
	printf("\nRECEIVED MESSAGE\n");
	printf("----------PAYLOAD--------------\n");
	for( int i = 0; i < FIFO_SIZE; i++ ) {
		printf("%02x ", message[i]);
	}
	printf("\n-------------------------------\n");

	// check the first byte in command for command type
	switch( message[0] ) { // handle commands in the array

		// TOGGLE LED
		case 0x10:
			printf("LED SIGNAL!!!\n");
			gpio_put(25, !gpio_get(25));
			break;
		
		// ECHO SOME BYTES BACK TO MASTER
		case 0x20:
			printf("ECHO SIGNAL!!!\n");
			const uint8_t buffer[2] = {0x20,0x30};
			i2c_write_raw_blocking(i2c0, buffer, 2);
			break;

		// NONE OF THE COMMAND BYTES APPLY
		default:
			printf("NO MATCHING COMMAND\n");
			break;
	}
}

int main() {

	// STDOUT over usb serial
	stdio_usb_init();

	// set address 0x31
	init_i2c_bus(0x31);

	// setup LED
	gpio_init(25);
	gpio_set_dir(25, GPIO_OUT);

	// start listener. This can be launched in a second core if neccecary
	// connect i2c num and handler function
	i2c_listener(bus, on_message);

	return 0;
}
