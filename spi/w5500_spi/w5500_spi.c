/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "loopback.h"


/* Example code to talk to a wiznet MEMS accelerometer and gyroscope.
   Ignores the magnetometer, that is left as a exercise for the reader.

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor SPI) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board and a generic wiznet board, other
   boards may vary.
   GPIO 2 (pin 4) SCK/spi0_sclk -> SCL on wiznet board
   GPIO 3 (pin 5) MOSI/spi0_tx -> SDA on wiznet board
   GPIO 4 (pin 6) MISO/spi0_rx-> ADO on wiznet board
   GPIO 5 (pin 7) Chip select -> NCS on wiznet board
   GPIO 6 (pin 9) SCK/spi0_sclk -> SCL on wiznet board
   3.3v (pin 36) -> VCC on wiznet board
   GND (pin 38)  -> GND on wiznet board

   Note: SPI devices can have a number of different naming schemes for pins. See
   the Wikipedia page at https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
   for variations.
   The particular device used here uses the same pins for I2C and SPI, hence the
   using of I2C names
*/

#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_RST  6

#define SPI_PORT spi0
#define READ_BIT 0x00//0x80
#define TCP_SOCKET 0

wiz_NetInfo gWIZNETINFO = { .mac = {0x00,0x08,0xdc,0x78,0x91,0x71},
                            .ip = {192,168,0,15},
                            .sn = {255,255,255,255},
                            .gw = {192,168,0,1},
                            .dns = {168,126,63,1},
                            .dhcp = NETINFO_STATIC};
#define ETH_MAX_BUF 2048

unsigned char ethBuf0[ETH_MAX_BUF];
void print_network_information(void);


static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

static void wiznet_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here

    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);
    bi_decl(bi_1pin_with_name(PIN_RST, "W5500 RST"));

    
    

}
  uint8_t  wiznet_read(void) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    uint8_t rx = 0, tx = 0xFF;
        spi_read_blocking(SPI_PORT, &tx,  &rx, 1);
        return rx;
        
    }
    static void wiznet_write(uint8_t reg){
 
        spi_write_blocking(SPI_PORT, &reg, 1);
    }
    

int main() {
    int8_t phy_link =0;
    uint8_t id=0;
    stdio_init_all();

    printf("Hello, WIZ850! Reading raw data from registers via SPI...\n");

    // This example will use SPI0 at 0.5MHz.
    spi_init(SPI_PORT, 5000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PIN_CS, "SPI CS"));

    wiznet_reset();

    reg_wizchip_spi_cbfunc(wiznet_read,wiznet_write);
    reg_wizchip_cs_cbfunc(cs_select,cs_deselect);
    id =getVERSIONR();
    printf("Version is 0x%x\n", id);;
 
    do{//check phy status
        if(ctlwizchip(CW_GET_PHYLINK, &phy_link)== -1){
            printf("Unknow PHY LINK status.\r\n");
            sleep_ms(10);
        }
    }while(phy_link == PHY_LINK_OFF);
    ctlnetwork(CN_SET_NETINFO,&gWIZNETINFO);
    print_network_information();

    while(1){
         loopback_tcps(TCP_SOCKET, ethBuf0,5000);
    }

    return 0;
}

void print_network_information(void)
{
	wizchip_getnetinfo(&gWIZNETINFO);
	printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	printf("IP address : %d.%d.%d.%d\n\r",gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	printf("SM Mask	   : %d.%d.%d.%d\n\r",gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	printf("Gate way   : %d.%d.%d.%d\n\r",gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	printf("DNS Server : %d.%d.%d.%d\n\r",gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
}
