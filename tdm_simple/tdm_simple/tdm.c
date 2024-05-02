






/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
//#include "tusb.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
//#include <hardware/flash.h>
#include "pio_tdm.h"
#include "ssd1306.h"
#include "Sydefs.h"
#include "smp_gse.h"
#include "ff.h"
#include "zmodem.h"
#include "diskio.h"
#include "zerror.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"



#define pin_txEN  6
#define pin_txADR 7
#define pin_txDAT 8

#define pin_rxCLK 16
#define pin_rxFRM 17
#define pin_rxADR 18
#define pin_rxDAT 19

#define pin_hssCLK 20
#define pin_hssFRM 21
#define pin_hssDAT 22

/*
#define BUF_SIZE 20
static uint16_t txbuf[BUF_SIZE];
static uint16_t rxbuf[BUF_SIZE];
*/

#define HssRxBufLen 256
uint16_t HssRxBufPtr= 0;
uint16_t HssRxBuf[HssRxBufLen];

uint16_t returned_sample;

// I2C
const uint I2C_DAT_PIN = 24;
const uint I2C_CLK_PIN = 25;
#define DISPLAY_ADDRESS 0x3c // 011110+SA0+RW - 0x3C or 0x3D
ssd1306_t disp;



/********************************************/
// Variables
/********************************************/
boolt execute_dump(void);
bool isOnHomepage = true;
bool isOnTailPage = false;
char globalFilename[256];
int currentIndexAtmosphere = 0;
int currentIndexClassic = 0;
int currentIndexNG = 0;
bool isOnAtmoPage = false;
bool isOnClassPage = false;
bool isOnNextGenPage = false;
bool isOnSerial = false;
bool serialClicked = false;
bool isOnNewAtmosphere = false;
bool isNewClassic = false;
bool isNewNextGen = false;
char selectedTail[20] = {0}; // For storing the selected tail
char selectedSerial[20] = {0};
bool ConfirmClassicPage = false;
bool ConfirmNGPage = false;
bool ConfirmAtmoPage = false;
bool isConfirmed = false;
bool skipNextLine = false; 
bool skipNextLineTwo = false; 
int selectedLetterIndex = 0; // Index of the currently selected character in the word
// At the top of your code, define global variables to track the menu state
bool isOnTailPageMenu = false; // To track if we're viewing the tail page menu
int tailPageMenuSelection = 0; // Index of the currently selected menu item
int mainMenuSelection = 0; // Default to first option
bool isOnMainMenu = true; // Assuming we start on the main menu
int confirmSelection = 0; // 0 for confirm, 1 for cancel
bool atmoSelectedSerial = false;
bool atmoBack = false;
bool showAtmoConfirmation = false;
bool atmoConfirmationNeeded = false;
bool classicSelectedSerial = false;
bool classicBack = false;
bool classicConfirmationNeeded = false;
bool nextGenSelectedSerial = false;
bool nextGenBack = false;
bool nextGenConfirmationNeeded = false;











/********************************************/
// FATfs
/********************************************/
FATFS fatFs;
FIL fileO;
DIR directoryO;
FILINFO fileInfo;
FRESULT resultF;
FATFS *fatFsPtr;
FIL *fp_dump;
char filename[20];

/********************************************/
// PIO
/********************************************/
pio_tdm_inst_t tdm = {
        .pio = pio0,
        .sm = 0
};

/********************************************/
// Queue Typedef & Declaration
/********************************************/
// #define BufLen 2048
#define BufLen 4096
typedef struct {
    uint16_t InPtr;
    uint16_t OutPtr;
    uint16_t Buffer[BufLen];
} q;

q RxTDMQ;
q RxHSSQ;

/********************************************/
// PushQ :
/********************************************/
char PushQ (q* QPtr, unsigned Value) {
    uint32_t IntrptStatus;

    QPtr->Buffer[QPtr->InPtr] = Value;
//    IntrptStatus = save_and_disable_interrupts();
    if (((QPtr->InPtr + 1) % BufLen) != QPtr->OutPtr) {
        QPtr->InPtr = ((QPtr->InPtr + 1) % BufLen);
//        restore_interrupts (IntrptStatus);
        return (1);
    } else {
//        restore_interrupts (IntrptStatus);
        return (0);
    }
}

/********************************************/
// EmptyQ :
/********************************************/
char EmptyQ (q* QPtr) {
    return (QPtr->InPtr == QPtr->OutPtr);
}

/********************************************/
// PopQ :
/********************************************/
unsigned PopQ (q* QPtr) {
    unsigned ReturnVal;
    uint32_t IntrptStatus;

    if (EmptyQ (QPtr)) {
        ReturnVal = 0xffffffff;
    } else {
//        IntrptStatus = save_and_disable_interrupts();
        ReturnVal = QPtr->Buffer[QPtr->OutPtr];
        QPtr->OutPtr = ((QPtr->OutPtr + 1) % BufLen);
//        restore_interrupts (IntrptStatus);
    }
    return (ReturnVal);
}

/********************************************/
// RxTDM ISR
/********************************************/
static void __not_in_flash_func(pio_irq0_handler)(void) {
    while (!pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm))) {
            PushQ (&RxTDMQ, *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm)]));
    }
}

/********************************************/
// RxHSS ISR
/********************************************/
static void __not_in_flash_func(pio_irq1_handler)(void) {
    while (!pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm+3))) {
            PushQ (&RxHSSQ, *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm+3)]));
    }
}

/********************************************/
// TxMux - Multiplexed, byte stuffed tx
/********************************************/
/*
#define TxMuxBufLen 64 
char TxMuxBuf[TxMuxBufLen];
uint16_t TxMuxPtr = 0;
char TxMuxChan=0;
void TxMux (char chan, uint16_t len, char *message) {
    tud_cdc_n_write (0,"\x7e",1);
    tud_cdc_n_write (0,&chan,1);
    
    while (len) {
        if ((*(char *)message == 0x7d)  || (*(char *)message == 0x7e)) {
            tud_cdc_n_write (0,"\x7d",1);
            *(char *)message ^= 0x40;
        }
        tud_cdc_n_write (0,message,1);
        message++;
        len--;
    }

//    tud_cdc_n_write (0,message,len);
    tud_cdc_n_write_flush (0);
}
*/

/*
    TxMuxPtr = 0;
//    if (chan != TxMuxChan) {
        TxMuxBuf[TxMuxPtr++] = 0x7e;
        TxMuxBuf[TxMuxPtr++] = chan;
//    }

    for (int i=0; i<len; i++) {
        if ((message[i] == 0x7d)  || (message[i] == 0x7e)) {
            TxMuxBuf[TxMuxPtr++] = 0x7d;
            TxMuxBuf[TxMuxPtr++] = message[i] ^ 0x40;
        } else
            TxMuxBuf[TxMuxPtr++] = message[i];

        if (TxMuxPtr >= (TxMuxBufLen-2)) {
            tud_cdc_n_write (0,TxMuxBuf,TxMuxPtr);
            tud_cdc_n_write_flush (0);
            TxMuxPtr = 0;
        }
    }

    if (TxMuxPtr) {
        tud_cdc_n_write (0,TxMuxBuf,TxMuxPtr);
        tud_cdc_n_write_flush (0);
    }
*/

/********************************************/
// RxMux - Multiplexed, byte stuffed rx
/********************************************/
/*
#define RxMuxBufLen 64
char RxMuxBuf[RxMuxBufLen];
uint16_t RxMuxPtr = 0;
char RxMuxChan = 0;
typedef enum {
  SCAN,
  SOF,
  ESC,
  NORM
} RxMuxStateType;
RxMuxStateType RxMuxState = NORM;

uint16_t RxMux (char *chan, char *message) {
    char inchar;

    while (tud_cdc_n_available(0)) {
        tud_cdc_n_read (0, &inchar, 1);
        switch (RxMuxState) {
            case SCAN:
                if (inchar == 0x7e) 
                    RxMuxState = SOF;
            case SOF:
                RxMuxPtr = 0;
                RxMuxChan = inchar;
                RxMuxState = NORM;
                break;

            case ESC:
                RxMuxBuf[RxMuxPtr++] = inchar ^ 0x40;
                RxMuxState = NORM;
                break;

            case NORM:
                if (inchar == 0x7e) {
                    RxMuxState = SOF;
                } else if (inchar == 0x7d) {
                    RxMuxState = ESC;
                } else {
                    message[RxMuxPtr++] = inchar;
                }
                break;
        }

        if (RxMuxPtr >= 2) {
            *chan = RxMuxChan;
//            message = RxMuxBuf;
            RxMuxPtr = 0;
            return (1);
        } 
    }

    return (0);
}
*/

/********************************************/
// main
/********************************************/

void displayError(const char* line1, const char* line2, const char* line3) {
    ssd1306_clear(&disp);  // Clear the display

    // Display the first line
    ssd1306_draw_string(&disp, 0, 0, 2, line1);

    // Display the second line
    ssd1306_draw_string(&disp, 0, 16, 2, line2);

    // Display the third line
    ssd1306_draw_string(&disp, 0, 32, 2, line3);

    ssd1306_show(&disp);  // Update the display to show the new content
}

// Ensure this function tries to read a byte as a test of presence
void i2c_scan() {
    uint8_t temp;
    printf("Scanning I2C bus...\n");
    for (int addr = 1; addr < 128; addr++) {
        if (i2c_read_blocking(i2c0, addr, &temp, 1, false) == 1) {
            printf("Device found at 0x%02x\n", addr);
        }
    }
}






void test_address(uint i2c_port, uint8_t address) {
    uint8_t response[1]; // Adjust size based on expected response
    while (true) {
        int result = i2c_read_blocking(i2c_port, address, response, 1, false);
        if (result == 1) {
            printf("Received response from 0x%02x: %d\n", address, response[0]);
        }
        sleep_ms(100);
    }
}
#define I2C_PORT i2c0
#define JOYSTICK_I2C_ADDR 0x3F
#define SDA_PIN 4
#define SCL_PIN 5
#define I2C_FREQ 100000

// Define this function if not already defined
void init_i2c_with_pullups() {
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(I2C_DAT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_CLK_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_DAT_PIN);
    gpio_pull_up(I2C_CLK_PIN);
}
void init_i2c() {
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C); // SDA
    gpio_set_function(5, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(4);
    gpio_pull_up(5);
}

uint8_t read_joystick_state() {
    uint8_t state = 0;
    i2c_read_blocking(i2c0, JOYSTICK_I2C_ADDR, &state, 1, false);
    return state;
}



void test_joystick_bit_masks() {
    stdio_init_all();
    init_i2c();

    printf("Testing joystick bit masks. Please move the joystick in each direction one at a time.\n");
    while (true) {
        uint8_t state = read_joystick_state();
        printf("Joystick state: 0x%02X at time %lu ms\n", state, to_ms_since_boot(get_absolute_time()));
        sleep_ms(500);  // Adjust the delay as needed to manage the frequency of readings
    }
}

void process_joystick(uint8_t state) {
    if (state == 0xFF) {
        printf("Error reading joystick\n");
        return;
    }
    printf("Processing joystick state: 0x%02X\n", state);
    if (state & 0x01) {
        printf("Joystick Up\n");
    }
    if (state & 0x02) {
        printf("Joystick Down\n");
    }
    if (state & 0x04) {
        printf("Joystick Left\n");
    }
    if (state & 0x08) {
        printf("Joystick Right\n");
    }
    if (state & 0x10) {
        printf("Joystick Press\n");
    }
}

char temps[100];
int main() {
    // Enable UART so we can print status output
    stdio_init_all();
    getchar_timeout_us(0);
     init_i2c();
    uint8_t address = 0x3F;
    uint8_t buffer[1];
    init_i2c_with_pullups();
   

    
    
    //----------------------------
    // Enable FA2100 GSE discrete
    //----------------------------
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2,1);

    //----------------------------
    // Enable FA2100 SMP/FDP discrete
    //----------------------------
    gpio_init(0);
    gpio_set_dir(0, GPIO_OUT);
    gpio_put(0,1);

    //----------------------------
    // Temporary Display Reset
    //----------------------------
    gpio_init(9);
    gpio_set_dir(9, GPIO_OUT);
    gpio_put(9,0);
    sleep_ms(10);
    gpio_put(9,1);

    //-------------------------------
    // Initialize Scroll Switch Input
    //-------------------------------
    int i;
    for (i=3; i<=5; i++) {
      gpio_init (i);
      gpio_set_dir(i, GPIO_IN);
      gpio_pull_up(i);
    }

    printf("Initializing I2C... ");
    //----------------------------
    // Initialize I2C 
    //----------------------------

    //----------------------------
    // Initialize Display
    //----------------------------
 disp.external_vcc=false;
    sleep_ms(100);
    ssd1306_init(&disp, 128, 64, 0x3c, i2c0);

    // ssd1306_clear(&disp);
    // ssd1306_draw_string(&disp,0,0, 2,"Send (U)");
    // ssd1306_draw_string(&disp,0,20, 2,"Receive (P)");
    // ssd1306_draw_string(&disp,0,40, 2,"List (D)");
    // ssd1306_show(&disp);
   

/*
    int rs=0;
    char display_string[20];
    while (1==1) {
        ssd1306_clear(&disp);
        for (i=3; i<=5; i++) 
            if (!gpio_get(i))
                ssd1306_draw_square(&disp,(i-3)*30,30,20,20);
        sprintf (display_string,"%i",rs++);
        ssd1306_draw_string(&disp,0,0,2,display_string);
        ssd1306_show(&disp); 
        sleep_ms(100);
        }
*/
    // 
    // Initialize Flash Memory
    //
/*    
    unsigned char *p = (unsigned char *)XIP_BASE;
    int ptr;
    
//    while (!tud_cdc_n_available(0));
    while (getchar_timeout_us(0) ==  PICO_ERROR_TIMEOUT);


    uint32_t IntSave = save_and_disable_interrupts();

    flash_range_erase (0x100000, FLASH_SECTOR_SIZE);

    unsigned char WriteBuffer[FLASH_PAGE_SIZE] = "1234567890.";
    flash_range_program (0x100000, WriteBuffer, FLASH_PAGE_SIZE);

    restore_interrupts (IntSave);

    p += 0x100000;
    for (ptr=0; ptr<512; ptr++) {  
        if ((ptr % 16) == 0) {
            printf ("\r\n%08x-",p+ptr);
        }
        printf ("%02x ", p[ptr]);
    }

    while (1==1) {}
*/

    // 
    // Initialize TDM receivers and transmitters
    //
    gpio_init(pin_txEN);
    gpio_set_dir(pin_txEN, GPIO_OUT);
    gpio_put(pin_txEN, 0);
    gpio_init(pin_txADR);
    gpio_set_dir(pin_txADR, GPIO_OUT);
    gpio_put(pin_txADR, 0);
    gpio_init(pin_txDAT);
    gpio_set_dir(pin_txDAT, GPIO_OUT);
    gpio_put(pin_txDAT, 0);

    float clkdiv = 31.25f;  // 2 MHz @ 125 clk_sys
    uint TDMtx_prog_offs = pio_add_program(tdm.pio, &TDMtx_program);
    uint TDMrx_prog_offs = pio_add_program(tdm.pio, &TDMrx_program);
    uint HSSrx_prog_offs = pio_add_program(tdm.pio, &HSSrx_program);
    pio_TDMrx_init(tdm.pio, tdm.sm,
                    TDMrx_prog_offs,
                    16,       // 16 bits per TDM ADR/DAT frame
                    clkdiv/8,
                    pin_rxDAT
    );
    // Enable FIFO interrupt in the PIO itself
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq0_handler);
    pio_set_irq0_source_enabled(pio0,  pis_sm0_rx_fifo_not_empty, true);
    // Enable IRQ in the NVIC
    irq_set_enabled(PIO0_IRQ_0, true);

    pio_TDMtx_init(tdm.pio, tdm.sm + 1,
                    TDMtx_prog_offs,
                    16,       // 16 bits per TDM ADR/DAT frame
                    clkdiv/8,
                    pin_txEN,
                    pin_txDAT
    );

    pio_TDMtx_init(tdm.pio, tdm.sm + 2,
                    TDMtx_prog_offs,
                    16,       // 16 bits per TDM ADR/DAT frame
                    clkdiv/8,
                    pin_txEN,
                    pin_txADR
    );

    pio_HSSrx_init(tdm.pio, tdm.sm+3,
                    HSSrx_prog_offs,
                    16,       // 16 bits per TDM ADR/DAT frame
                    clkdiv/8,
                    pin_hssDAT
    );
     // Enable FIFO interrupt in the PIO itself
    irq_set_exclusive_handler(PIO0_IRQ_1, pio_irq1_handler);
    pio_set_irq1_source_enabled(pio0,  pis_sm3_rx_fifo_not_empty, true);
    // Enable IRQ in the NVIC
    irq_set_enabled(PIO0_IRQ_1, true);
   


    //
    // Initialize temporary CLK/FRM source
    //
/*
    uint TDMclk_prog_offs = pio_add_program(tdm.pio, &TDMclk_program);
    pio_TDMclk_init(tdm.pio, tdm.sm + 2,
                    TDMclk_prog_offs,
                    16,       // 16 bits per TDM ADR/DAT frame
                    clkdiv,
                    pin_txCLK
    );
*/
    //---------------------------------------------------
    //
    //  DMA transfer
    //
    //---------------------------------------------------
    // Get a free channel, panic() if there are none
/*    
    int chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO0_TX1);
*/

    // initialize random transmit buffer
/*    uint16_t *dst,*src;
    size_t tx_remain, rx_remain;
    for (int i = 0; i < BUF_SIZE; ++i) 
        txbuf[i] = rand() >> 16;
*/    //
    // Big forever TEST loop
    //

//    while (tud_cdc_n_available(0)) 
//        tud_cdc_n_read (0, &c, 1);

    uint32_t NextTimeUpdate=0;

    uint8_t last_state = 0xFF;
    while (1==1) {
        // uint8_t joystick_state = read_joystick_state();
        // printf("Joystick state: 0x%02X\n", joystick_state);
        //  sleep_ms(300);
    //    uint8_t joystick_state = read_joystick_state();
    //     if (joystick_state != last_state) {  // Check if state has changed
    //         if (joystick_state != 0xFF) {  // Only print if state is not idle
    //             printf("Joystick state: 0x%02X\n", joystick_state);
    //         }
    //         last_state = joystick_state;  // Update the last state
    //     }
    //     sleep_ms(300);
       
/*        
        if (!pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm))) {
            returned_sample = *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm)]);
            if (((returned_sample & 0xff00) == 0x1000) || ((returned_sample & 0xff00) == 0x6c00))
                printf ("%c",returned_sample & 0x7f);
            else    
                printf ("[%04x]", returned_sample);
        }
*/
        while (!EmptyQ (&RxTDMQ)) {
            returned_sample = PopQ (&RxTDMQ);
//            TxMux (0,2,(char *) &returned_sample);

            if (((returned_sample & 0xff00) == 0x1000) || ((returned_sample & 0xff00) == 0x6c00)) {
                putchar_raw ((char) (returned_sample & 0x7f));
                stdio_flush();
//                tud_cdc_n_write_char (0,(char) (returned_sample & 0x7f));
//                tud_cdc_write_flush();
            }
            else {   
                sprintf (temps,"[%04x]", returned_sample);
                PutString_Block (temps);
//                stdio_usb.out_chars(temps,strlen(temps));
                stdio_flush();
//                tud_cdc_n_write (0,temps,strlen(temps));
//                tud_cdc_n_write_flush (0);
            }

        }

        // Buffer HSS receptions
//        if (!pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm+3))) 
//            HssRxBuf[HssRxBufPtr++] = *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm+3)]);


        while  (!EmptyQ (&RxHSSQ)) {
//            returned_sample = PopQ (&RxHSSQ);
//            TxMux (1,2,(char *) &returned_sample);
             HssRxBuf[HssRxBufPtr++] = PopQ (&RxHSSQ);
        }


/*
        char PacifierString[20];
        if (to_ms_since_boot(get_absolute_time()) > NextTimeUpdate) {
            NextTimeUpdate = to_ms_since_boot(get_absolute_time()) + 500;
            sprintf (PacifierString,"%i\n",NextTimeUpdate);
            for (int i=0; i<strlen(PacifierString); i++) 
                TxMux (0,2,PacifierString+i);
        }
*/

        if (HssRxBufPtr == HssRxBufLen) {
//            TxMux (1,HssRxBufPtr*2,(char *)HssRxBuf);
            
            for (HssRxBufPtr=0; HssRxBufPtr<HssRxBufLen; HssRxBufPtr++) {  
                if ((HssRxBufPtr % 8) == 0) {
                    sprintf (temps,"\r\n%04x-",HssRxBufPtr);
                    PutString_Block (temps);
//                    stdio_usb.out_chars(temps,strlen(temps));
//                    tud_cdc_n_write (0,temps,strlen(temps));
                }
                sprintf (temps,"%04x ", HssRxBuf[HssRxBufPtr]);
                PutString_Block (temps);
//                stdio_usb.out_chars(temps,strlen(temps));
//                tud_cdc_n_write (0,temps,strlen(temps));
            }
 
        stdio_flush();
//            tud_cdc_write_flush();
            HssRxBufPtr = 0;                
        }

      


/*
        char RxMuxChan;
        uint16_t FAcommand;
        if (RxMux(&RxMuxChan, &FAcommand)) {
            *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+1)]) = FAcommand;
            *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+2)]) = 0x4000;
        }
*/
        uint32_t ReadLen;
        char inchar;
        if ((inchar = getchar_timeout_us(0)) !=  PICO_ERROR_TIMEOUT) {
//          if (tud_cdc_n_available(0)) {
//            if (ReadLen = tud_cdc_n_read (0, &inchar, 1)) {
                if ((inchar & 0xff) == 'X') {
                    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+1)]) = 0x0000;
                    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+2)]) = 0x4000;
                } else if ((inchar & 0xff) == 'Y') {
                    HssRxBufPtr = 0;
                    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+1)]) = 0x1301;
                    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+2)]) = 0x4000;
                } else if ((inchar & 0xff) == 'Z') {
                    ReadDFDR();
                } else if ((inchar & 0xff) == 'S') {

/*
                    sd_card_t *pSD = sd_get_by_num(0);
                    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
                    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
*/
/*
                    if (!arg1) arg1 = sd_get_by_num(0)->pcName;
                    FATFS *p_fs = sd_get_fs_by_name(arg1);
                    if (!p_fs) {
                        printf("Unknown logical drive number: \"%s\"\n", arg1);
                        return;
                    }
                    FRESULT fr = f_mount(p_fs, arg1, 1);
                    if (FR_OK != fr) {
                        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
                        return;
                    }
                    sd_card_t *pSD = sd_get_by_name(arg1);
                    myASSERT(pSD);
                    pSD->mounted = true;
*/
                    SendZmodem();
/*
                    resultF = f_mount(&fatFs, "0:", 1);
                    if (FR_OK != resultF) printf ("f_mount error\n");
                    zsend ("DUMP0001.FDR");
                    resultF = f_unmount("0:"); 
*/
                }


//            }
       }

displayMainMenu(); // Initialize with the main menu displayed
isOnMainMenu = true; // Start with the main menu active

while (true) {
    if (isOnMainMenu) {
        handleMainMenuNavigation();
    } else if (isOnTailPageMenu) {
        handleMenuNavigation(); // Handles navigation within the Tail Page Menu
    } else if (isOnClassPage) {
        handleClassicPageNavigation();
    } else if (isOnNextGenPage) {
        handleNextGenPageNavigation();
    }  else if (isOnAtmoPage){
        handleAtmospherePageNavigation();
    }  else if (ConfirmAtmoPage){
        handleAtmoConfirmationInput();
    }  else if (ConfirmClassicPage){
        handleClassicConfirmationInput();
    }  else if (ConfirmNGPage){
        handleNextGenConfirmationInput();
    } else if (isOnSerial) {
        handleInputs(); // Handle the word input process
        isOnSerial = false;
        displayReset(); // Or another appropriate function based on your flow
    }
    // }   else if (showAtmoConfirmation) {
    //     showAtmoConfirmation = false; // Reset the flag
    //     displayAtmoConfirmationPage(); // Show the confirmation page
    // }
    // Add an else-if block for handling the back action from each of these pages
    // It should set isOnTailPageMenu = true and call displayTailPageMenu()

    sleep_ms(100); // Adjust as needed for your application's responsiveness
}



        




    //     if (!gpio_get(3) && isOnTailPage) { // Assuming GPIO 3 is "scroll down"
    //         printf("Navigating to Atmosphere page...\n");
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Atmosphere");
    //         ssd1306_draw_string(&disp, 0, 20, 2, "Tails:");
    //         ssd1306_draw_string(&disp, 0, 40, 2, "Serial:");
    //         ssd1306_show(&disp);
    //             isOnAtmoPage = true;
    //             isOnClassPage = false; 
    //             isOnTailPage = false;
    //             isOnNextGenPage = false;
    //             sleep_ms(500);

    //     }

    //     if(isOnAtmoPage){
    //         if (!gpio_get(3)) { // Assuming GPIO 3 is "scroll down"   
    //             handleScrollDownAtmo();
    //         }
    //         if (!gpio_get(5)) { // Assuming GPIO 5 is "scroll up"
    //             handleScrollUpAtmo();
    //     }
    //     }

    //     if (!gpio_get(4) && isOnSerial) { 
    //         serialClicked = true;
    //         isOnNextGenPage = false;
    //         isOnClassPage = false;
    //         isOnAtmoPage = false;
    //         isOnTailPage = false;
    //         sleep_ms(500);
    //     }
    //     if(serialClicked){
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Tails:");
    //         ssd1306_draw_string(&disp, 0, 20, 2, "0000000");
    //         ssd1306_show(&disp);
    //         handleInputs();
    //         DisplayWordUpdate();

    //         sleep_ms(500);

         
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp,0,0, 2,"Send (U)");
    //         ssd1306_draw_string(&disp,0,20, 2,"Receive (P)");
    //         ssd1306_draw_string(&disp,0,40, 2,"List (D)");
    //         ssd1306_show(&disp);

    //     }


    //     if (!gpio_get(5) && isOnTailPage) { // Assuming GPIO 5 is "scroll up"
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Classic");
    //         printf("Navigating to Classic page...\n");
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Classic");
    //         ssd1306_draw_string(&disp, 0, 20, 2, "Tails:");
    //         ssd1306_draw_string(&disp, 0, 40, 2, "Serial:");
    //         ssd1306_show(&disp);
    //         isOnClassPage = true;
    //         isOnAtmoPage = false;
    //         isOnTailPage = false;
    //         isOnNextGenPage = false;
    //         sleep_ms(500);
    //     }



    //     //  if(isOnClassPage){
    //     //     if (!gpio_get(3)) { // Assuming GPIO 3 is "scroll down"   
    //     //         handleScrollDownClassic();
    //     //     }
    //     //     if (!gpio_get(5)) { // Assuming GPIO 5 is "scroll up"
    //     //         handleScrollUpClassic();
    //     // }
    //     // }       



    //     if (!gpio_get(4) && isOnTailPage) { // Assuming GPIO 4 is "push"
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Next Gen");
    //         printf("Navigating to Next Gen page...\n");
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp, 0, 0, 2, "Next Gen");
    //         ssd1306_draw_string(&disp, 0, 20, 2, "Tails:");
    //         ssd1306_draw_string(&disp, 0, 40, 2, "Serial:");
    //         ssd1306_show(&disp);
    //         isOnNextGenPage = true;
    //         isOnClassPage = false;
    //         isOnAtmoPage = false;
    //         isOnTailPage = false;
    //         sleep_ms(500);
    //     }



    //      if(isOnNextGenPage){
    //         if (!gpio_get(3)) { // Assuming GPIO 3 is "scroll down"   
    //             handleScrollDownNG();
    //         }
    //         if (!gpio_get(5)) { // Assuming GPIO 5 is "scroll up"
    //             handleScrollUpNG();
    //     }
    //     }  

    // if (!gpio_get(4) && isOnNewAtmosphere) {
    //     isOnAtmoPage = false;
    //     // User has chosen an option and now needs to confirm
    //     ssd1306_clear(&disp);
    //     ssd1306_draw_string(&disp, 0, 0, 2, "Confirm (U)");
    //     ssd1306_draw_string(&disp, 0, 20, 2, "Back (D)");
    //     ssd1306_show(&disp);
    //     isOnNewAtmosphere = false;
    //     ConfirmPage = true;
    // }


    // if (!gpio_get(4) && isNewClassic) {
    //     isOnClassPage = false;
    //     // User has chosen an option and now needs to confirm
    //     ssd1306_clear(&disp);
    //     ssd1306_draw_string(&disp, 0, 0, 2, "Confirm (U)");
    //     ssd1306_draw_string(&disp, 0, 20, 2, "Back (D)");
    //     ssd1306_show(&disp);
    //     isNewClassic = false;
    //     ConfirmPage = true;
    // }

    //     if (ConfirmPage) { // Assuming GPIO 4 is "push"
    //         if (!gpio_get(5)) { // Assuming GPIO 5 is "Confirm"
    //         // User confirmed
    //         ReadDFDR();
    //         ConfirmPage = false; // Reset confirmation page flag
    //         // Optionally, navigate user to another page or refresh current page
    //     }
        
    //     if (!gpio_get(3)) { // Assuming GPIO 3 is "Back"
    //         //Atmosphere
         
    //         ssd1306_clear(&disp);
    //         ssd1306_draw_string(&disp,0,0, 2,"Send (U)");
    //         ssd1306_draw_string(&disp,0,20, 2,"Receive (P)");
    //         ssd1306_draw_string(&disp,0,40, 2,"List (D)");
    //         ssd1306_show(&disp);
    //     }
    //         sleep_ms(500);
    //     }

    //     if (!gpio_get(4) && isNewNextGen) {
    //     isOnNextGenPage = false;
    //     // User has chosen an option and now needs to confirm
    //     ssd1306_clear(&disp);
    //     ssd1306_draw_string(&disp, 0, 0, 2, "Confirm (U)");
    //     ssd1306_draw_string(&disp, 0, 20, 2, "Back (D)");
    //     ssd1306_show(&disp);
    //     isNewNextGen = false;
    //     ConfirmPage = true;
    // }


        // If GPIO 3 (ReadDFDR) is pressed and we are on the homepage
        // if (!gpio_get(3) && isOnHomepage) {
        //     list_files();
        //     // ReadDFDR();
           
        // }
        // // If GPIO 5 (SendZmodem) is pressed and we are on the homepage
        // if (!gpio_get(5) && isOnHomepage) {
        //     SendZmodem(); // Call function to handle SendZmodem action
        // }


}


//            printf ("{");
//            for (int i=0; i<8; i++) {
//                while (pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm+3)));
//                returned_sample = *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm+3)]);
//                printf ("%04x", returned_sample);
//            }
//            printf ("}");

        

 
/*
        uint16_t returned_sample=0;
        // transmit a randow word 
        while (pio_sm_is_tx_fifo_full(tdm.pio, tdm.sm + 1));
        uint16_t random_sample = rand() >> 16;
        *((io_rw_16 *) &tdm.pio->txf[tdm.sm + 1]) = random_sample;
        // .... and await a response
        uint32_t timeout =  time_us_32();
        timeout += 10000;
        while (timeout > time_us_32()) {
            if (!pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm))) {
                if ((returned_sample = *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm)])) == random_sample)
                    break;
            }
        }
        if (returned_sample == random_sample)
            printf ("%04x ", returned_sample);
        else
            printf ("[%04x %04x]",random_sample,returned_sample);;
*/

/*
        // new random TX data
        for (int i = 0; i < BUF_SIZE; ++i) 
            txbuf[i] = rand() >> 16;

        // Test transmitter/receiver
        printf("TX:");
        for (int i = 0; i < BUF_SIZE; ++i) {
            rxbuf[i] = 0;
            printf(" %04x", (int) txbuf[i]);
        }
        printf("\n");

        tx_remain = BUF_SIZE;
        rx_remain = BUF_SIZE;
        dst = rxbuf;
        src = txbuf;
        while (tx_remain || rx_remain) {
            if (tx_remain && !pio_sm_is_tx_fifo_full(tdm.pio, tdm.sm + 1)) {
                *((io_rw_16 *) &tdm.pio->txf[tdm.sm + 1]) = *src++;
                --tx_remain;
            }
            if (rx_remain && !pio_sm_is_rx_fifo_empty(tdm.pio, (tdm.sm))) {
                *dst = *((io_rw_16 *) &tdm.pio->rxf[(tdm.sm)]);
                if (*dst != 0xffff)
                    {
                    dst++;
                    --rx_remain;
                }
            }
        }

        
        printf("RX:");
        bool mismatch = false;
        for (int i = 0; i < BUF_SIZE; ++i) {
            printf(" %04x", (int) rxbuf[i]);
            mismatch = mismatch || rxbuf[i] != txbuf[i];
        }
        if (mismatch)
            printf("\nNope-------------------------------------------------------------------------------------------------\n");
        else
            printf("\nOK\n");

*/

/*
        // We could choose to go and do something else whilst the DMA is doing its
        // thing. In this case the processor has nothing else to do, so we just
        // wait for the DMA to finish.
       dma_channel_configure(
            chan,                       // Channel to be configured
            &c,                         // The configuration we just created
            &tdm.pio->txf[tdm.sm + 1],  // The initial write address
            txbuf,                      // The initial read address
            20,                         // Number of transfers
            true                        // Start immediately.
        );
       dma_channel_wait_for_finish_blocking(chan);
*/

/*        
        printf("\nI2C Bus Scan\n");
        printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

        for (int addr = 0; addr < (1 << 7); ++addr) {
            if (addr % 16 == 0) {
                printf("%02x ", addr);
            }

            // Perform a 1-byte dummy read from the probe address. If a slave
            // acknowledges this address, the function returns the number of bytes
            // transferred. If the address byte is ignored, the function returns
            // -1.

            // Skip over any reserved addresses.
            int ret;
            uint8_t rxdata;
            if (reserved_addr(addr))
                ret = PICO_ERROR_GENERIC;
            else
                ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

            printf(ret < 0 ? "." : "@");
            printf(addr % 16 == 15 ? "\n" : "  ");
        }
        printf("Done.\n\n");
*/

//        sleep_ms (100);
}

/********************************************/
// Send file via Zmodem 
/********************************************/
// void SendZmodem () {
//     resultF = f_mount(&fatFs, "0:", 1);
//     if (FR_OK != resultF) printf ("f_mount error\n");
//     zsend ("DUMP0001.FDR");
//     resultF = f_unmount("0:"); 
// }





// void SendZmodem() {
//     DIR dir;
//     FILINFO fno;
//     FRESULT fr;
//     char newestFileName[256] = {0}; // Assuming a max filename length of 255
//     DWORD newestFileDate = 0; // To track the date of the newest file

//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Uploading");
//     ssd1306_draw_string(&disp, 0, 20, 2, "Data Files");
//     ssd1306_show(&disp);

//     // Mount the filesystem
//     resultF = f_mount(&fatFs, "0:", 1);
//     if (resultF != FR_OK) {
//         printf("f_mount error\n");
//         return;
//     }

//     // Open the root directory
//     fr = f_opendir(&dir, "/");
//     if (fr == FR_OK) {
//         while (1) {
//             fr = f_readdir(&dir, &fno); // Read a directory item
//             if (fr != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir
//             // Skip directories and files starting with ._
//             if (!(fno.fattrib & AM_DIR) && strncmp(fno.fname, "._", 2) != 0) {
//                 // Compare the last modified date to find the newest file
//                 if ((fno.fdate << 16 | fno.ftime) > newestFileDate) {
//                     newestFileDate = fno.fdate << 16 | fno.ftime; // Update the newest file date
//                     strcpy(newestFileName, fno.fname); // Update the newest file name found
//                 }
//             }
//         }
//         f_closedir(&dir);
//     } else {
//         printf("Failed to open directory\n");
//     }

//     // Proceed only if a newest file has been identified
//     if (newestFileName[0] != 0) {
//         zsend(newestFileName);
//     } else {
//         printf("No suitable files found to send\n");
//     }

//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Upload");
//     ssd1306_draw_string(&disp, 0, 20, 2, "Complete");
//     ssd1306_show(&disp);

//     // Unmount SD card
//     resultF = f_unmount("0:");

//     sleep_ms(5000);

//     // Clear the display and show the original "Send" and "Receive" page
//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Send (U)");
//     ssd1306_draw_string(&disp, 0, 20, 2, "Receive (P)");
//     ssd1306_draw_string(&disp, 0, 40, 2, "List (D)");
//     ssd1306_show(&disp);
// }


void SendZmodem() {
    FRESULT fr;
    FILINFO fno;
     DIR dir;
    char newestFileName[256] = {0}; // Buffer for the file name with the highest sequence number
    unsigned int highestSequenceNumber = 0; // To track the highest sequence number found

    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Uploading");
    ssd1306_draw_string(&disp, 0, 20, 2, "Data Files");
    ssd1306_show(&disp);

    // Mount the filesystem
    resultF = f_mount(&fatFs, "0:", 1);
    if (resultF != FR_OK) {
        printf("f_mount error\n");
        return;
    }

    // Open the root directory
    fr = f_findfirst(&dir, &fno, "/", "*.*");
    if (fr == FR_OK && fno.fname[0]) {
        do {
            unsigned int currentSequenceNumber = 0;
            if (sscanf(fno.fname, "%*[^_]_%u.FDR", &currentSequenceNumber) == 1) {
                if (currentSequenceNumber > highestSequenceNumber) {
                    highestSequenceNumber = currentSequenceNumber;
                    strcpy(newestFileName, fno.fname); // Update the file name with the highest sequence number
                }
            }
        } while (f_findnext(&dir, &fno) == FR_OK && fno.fname[0]);
        f_closedir(&dir);
    } else {
        printf("Failed to open directory\n");
    }

    // Unmount SD card not needed before sending
    // resultF = f_unmount("0:");

    if (newestFileName[0] != '\0') {
        printf("Attempting to send file: %s\n", newestFileName);
        zsend(newestFileName); // Call zsend directly without pre-opening the file
    } else {
        printf("No suitable files found to send.\n");
    }

    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Upload");
    ssd1306_draw_string(&disp, 0, 20, 2, "Complete");
    ssd1306_show(&disp);

    sleep_ms(5000);

    // Clear the display for user navigation
    displayReset();
}




/********************************************/
// ReadDFDR
/********************************************/


// void ReadDFDR () {
//     int i;
//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Downloading");
//     ssd1306_draw_string(&disp, 0, 20, 2, "FDR Data..");
//     ssd1306_show(&disp);
//     boolt execute_dump (void);

//     // clear any incoming STDIO
//     while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);

//      // Execute DFDR Download
//     resultF = f_mount(&fatFs, "0:", 1);
//     fp_dump = &fileO;
//     i=0;
//     do {
//         sprintf (filename,"DUMP%4.4i.FDR",i++);
//     } while (f_open(fp_dump, filename, FA_WRITE | FA_CREATE_NEW) != FR_OK);
//     // f_open(fp_dump, "DUMP0001.FDR", FA_WRITE);
//     execute_dump();
//     f_close (fp_dump);
//     resultF = f_unmount("0:"); 
//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "FDR");
//     ssd1306_draw_string(&disp, 0, 20, 2, "Download");
//     ssd1306_draw_string(&disp, 0, 40, 2, "Complete");
//     ssd1306_show(&disp);

//     sleep_ms(5000);

//     // Clear the display and show the original "Send" and "Receive" page
//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Send (U)");
//     ssd1306_draw_string(&disp, 0, 20, 2, "Receive (D)");
//     ssd1306_draw_string(&disp,0,40, 2,"List (P)");
//     ssd1306_show(&disp);
// }
                   

char selectedWord[8] = "0000000";


#include "stdio.h"
#include "stdlib.h"

// Reads the current sequence number from a file
#include "ff.h" // Make sure you include the FatFs library header

// Reads the current sequence number from a file using FatFs
// unsigned int readSequenceNumber() {
//     FIL fil; // File object
//     unsigned int seqNum = 0;
//     FRESULT fr = f_open(&fil, "sequence_number_test.txt", FA_READ);
//     if (fr == FR_OK) {
//         char buffer[10];
//         UINT bytesRead;
//         f_read(&fil, buffer, sizeof(buffer)-1, &bytesRead);
//         buffer[bytesRead] = '\0'; // Null terminate the string
//         seqNum = atoi(buffer); // Convert to integer
//         f_close(&fil);
//     } else {
//         printf("Failed to open sequence_number.txt for reading.\n");
//     }
//     return seqNum;
// }

// // Writes the sequence number to a file using FatFs
// void writeSequenceNumber(unsigned int seqNum) {
//     FIL fil; // File object
//     FRESULT fr = f_open(&fil, "sequence_number_test.txt", FA_WRITE | FA_CREATE_ALWAYS);
//     if (fr == FR_OK) {
//         char buffer[10];
//         sprintf(buffer, "%u", seqNum); // Convert integer to string
//         UINT bytesWritten;
//         f_write(&fil, buffer, strlen(buffer), &bytesWritten);
//         f_close(&fil);
//     } else {
//         printf("Failed to open sequence_number.txt for writing.\n");
//     }
// }

// FATFS fatFs;
// FIL fileO;
// DIR directoryO;
// FILINFO fileInfo;
// FRESULT resultF;
// FATFS *fatFsPtr;
// FIL *fp_dump;
// char filename[20];

// void ReadDFDR() {
//     ssd1306_clear(&disp);
//     ssd1306_draw_string(&disp, 0, 0, 2, "Downloading");
//     ssd1306_draw_string(&disp, 0, 20, 2, "FDR Data..");
//     ssd1306_show(&disp);

//     // Clear any incoming STDIO
//     while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);

//     // Execute DFDR Download
//     resultF = f_mount(&fatFs, "", 1); // Mount the internal storage

//     // Increment the sequence number for the next file
//     unsigned int sequenceNumber = readSequenceNumber();
//     sequenceNumber++;

//     // Use the sequence number to generate a unique filename
//     char filename[20];
//     sprintf(filename, "DUMP%03u.FDR", sequenceNumber); // Format: DUMP001.FDR, DUMP002.FDR, ...

//     // Try to open the file. If it exists or can be created, proceed with the dump.
//     if (f_open(&fileO, filename, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
//         // If the file is successfully opened/created, execute the dump.
//         if (execute_dump()) {
//             // Dump was successful
//             ssd1306_clear(&disp);
//             ssd1306_draw_string(&disp, 0, 0, 2, "FDR");
//             ssd1306_draw_string(&disp, 0, 20, 2, "Download");
//             ssd1306_draw_string(&disp, 0, 40, 2, "Complete");
//             ssd1306_show(&disp);
//         } else {
//             // Handle dump failure
//             ssd1306_clear(&disp);
//             ssd1306_draw_string(&disp, 0, 0, 2, "Dump Error");
//             ssd1306_show(&disp);
//         }
//         f_close(&fileO); // Always close the file after the operation
//     } else {
//         // If the file couldn't be opened/created, display an error message
//         ssd1306_clear(&disp);
//         ssd1306_draw_string(&disp, 0, 0, 2, "File Error");
//         ssd1306_show(&disp);
//     }

//     // Update the sequence number (if needed)
//     writeSequenceNumber(sequenceNumber);

//     resultF = f_unmount(""); // Unmount the filesystem

//     sleep_ms(5000); // Wait a bit before clearing the display

//     // Clear the display and show the original "Send", "Receive", and "List" page
//     displayReset();
// }

#include "ff.h" // Include FatFs library

void ReadDFDR() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Downloading");
    ssd1306_draw_string(&disp, 0, 20, 2, "FDR Data..");
    ssd1306_show(&disp);

    // Clear any incoming STDIO
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);

    // Execute DFDR Download
    resultF = f_mount(&fatFs, "", 1); // Mount the filesystem for internal memory

    // Check if mounting was successful
    if (resultF != FR_OK) {
        printf("Failed to mount filesystem\n");
        return;
    }

    fp_dump = &fileO;

    // Find the highest existing sequence number
    unsigned int sequenceNumber = 1;
    char filename[30]; // Increase the filename buffer size
    while (1) {
        sprintf(filename, "%s_%03u.FDR", selectedTail, sequenceNumber); // Incorporate selected tail into filename
        if (f_stat(filename, NULL) != FR_OK) {
            // File doesn't exist, use this sequence number
            break;
        }
        sequenceNumber++;
    }

    // Use the sequence number to generate a unique filename
    sprintf(filename, "%s_%03u.FDR", selectedTail, sequenceNumber); // Incorporate selected tail into filename

    // Try to open the file. If it exists or can be created, proceed with the dump.
    if (f_open(fp_dump, filename, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        // If the file is successfully opened/created, execute the dump.
        if (execute_dump()) {
            // Dump was successful
            ssd1306_clear(&disp);
            ssd1306_draw_string(&disp, 0, 0, 2, "FDR");
            ssd1306_draw_string(&disp, 0, 20, 2, "Download");
            ssd1306_draw_string(&disp, 0, 40, 2, "Complete");
            ssd1306_show(&disp);
        } else {
            // Handle dump failure
            ssd1306_clear(&disp);
            ssd1306_draw_string(&disp, 0, 0, 2, "Dump Error");
            ssd1306_show(&disp);
        }
        f_close(fp_dump); // Always close the file after the operation
    } else {
        // If the file couldn't be opened/created, display an error message
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "File Error");
        ssd1306_show(&disp);
    }

    resultF = f_unmount(""); // Unmount the filesystem from internal memory

    sleep_ms(5000); // Wait a bit before clearing the display

    // Clear the display and show the original "Send", "Receive", and "List" page
    displayReset();
}



#include "ff.h"

FATFS fatFs;

void clearInternalStorage() {
    FRESULT fr;

    // Mount the filesystem
    fr = f_mount(&fatFs, "", 0);
    if (fr != FR_OK) {
        printf("Failed to mount filesystem\n");
        return;
    }

    DIR dir;
    FILINFO fileInfo;
    fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        printf("Failed to open directory\n");
        f_mount(NULL, "", 0); // Unmount the filesystem
        return;
    }

    for (;;) {
        fr = f_readdir(&dir, &fileInfo);
        if (fr != FR_OK || fileInfo.fname[0] == 0) break; // Break on error or end of dir

        if (!(fileInfo.fattrib & AM_DIR) && strncmp(fileInfo.fname, "._", 2) != 0) {
            // Check if it is a file and not starting with "._"
            fr = f_unlink(fileInfo.fname);
            if (fr != FR_OK) {
                printf("Failed to delete file: %s\n", fileInfo.fname);
            }
        }
    }

    f_closedir(&dir); // Close the directory

    // Unmount the filesystem
    fr = f_mount(NULL, "", 0);
    if (fr != FR_OK) {
        printf("Failed to unmount filesystem\n");
        return;
    }

    printf("Internal storage cleared successfully\n");
}


// Temporary File Functions
char pstring[200];
//char *fp_dump=NULL;

/*
unsigned char *XipBase = (unsigned char *)XIP_BASE;
char *FlashOpen() {
    // 
    // Initialize Flash Memory
    //
    int ptr;
    
//    while (!tud_cdc_n_available(0));
//    while (getchar_timeout_us(0) ==  PICO_ERROR_TIMEOUT);

    uint32_t IntSave = save_and_disable_interrupts();
    flash_range_erase (0x100000, FLASH_SECTOR_SIZE);

    unsigned char WriteBuffer[FLASH_PAGE_SIZE] = "1234567890.";
    flash_range_program (0x100000, WriteBuffer, FLASH_PAGE_SIZE);

    restore_interrupts (IntSave);

}
*/

/*
void f_putc (char c, char *file) {
    putchar_raw (c);
}

void f_write(char *fp_dump, char *buffer, int size, unsigned int *WriteCount) {
    int i;

    for (i=0; i<size; i++) {
        putchar_raw (buffer[i]);
    }
    *WriteCount = size;
    return (true);
}
*/

void PutString_Block (unsigned char *s) {
  int i=0;
  while (s[i] != 0)
    putchar_raw (s[i++]);
}

/* ------------------------------------------------------------------------ */
/*                      CRC Calculation Table                               */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This 256-word table is used for the table-lookup implementation of the   */
/* CRC-16calculation. Note that this table must be "public" since the CRC   */
/* functions are now "static inline'ed" via definitions in an "include"     */
/* file.                                                                    */
/*                                                                          */
/* ------------------------------------------------------------------------ */
const unsigned crc16_table [] = {
   0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
   0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
   0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
   0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
   0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
   0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
   0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
   0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
   0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
   0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
   0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
   0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
   0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
   0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
   0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
   0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
   0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
   0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
   0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
   0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
   0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
   0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
   0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
   0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
   0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
   0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
   0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
   0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
   0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
   0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
   0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
   0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

/* ------------------------------------------------------------------------ */
/* crc16_word  -  Update Specified CRC-16 Value with Specified Word         */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This function calculates the new CRC-16 value given the current value    */
/* and a new word to be processed. The updated 16-bit CRC value is returned */
/* as the function value.                                                   */
/*                                                                          */
/* Inputs  : Current CRC-16 value and new word to be processed              */
/*                                                                          */
/* Outputs : None                                                           */
/*                                                                          */
/* Returns : Updated CRC-16 value                                           */
/*                                                                          */
/* ------------------------------------------------------------------------ */
unsigned crc;
static unsigned crc16_word (unsigned crc_value, unsigned inword) {
   unsigned tbl_index;

   /* ---------------------------- */
   /* Handle hi byte of input word */
   /* ---------------------------- */
    crc_value &= 0xffff;
    tbl_index   = ((inword >> 8) & 0xff) ^ (crc_value & 0x00ff);
    crc_value >>= 8;
    crc_value  ^= crc16_table [tbl_index & 0xff];

   /* ---------------------------- */
   /* Handle lo byte of input word */
   /* ---------------------------- */
    crc_value &= 0xffff;
    tbl_index   = (inword & 0x00ff) ^ (crc_value & 0x00ff);
    crc_value >>= 8;
    crc_value  ^= crc16_table [tbl_index & 0xff];

   return (crc_value);
}

#define PKT_DISPLAY            0  /* Nonzero = Enable Packet Display        */
#define PKT_COUNT_REPORT       0  /* Nonzero = Packet-Count progress msgs   */
#define FIFO_SYNC_IMPLEMENTED  0  /* Nonzero = FIFO Sync Flag implemented   */
#define IGNORE_FD_RETRNSMT_WDS 1  /* Nonzero = Ignore TDM FD Retransmit Wds */
#define SHOW_TIMING            0  /* Nonzero = Show time next to msgs (RNS) */
#define SHOW_TRACE             0  /* Nonzero = Show SMP trace strings (RNS) */

/* ------------------------------------------------------------------------ */
/* Miscellaneous Constants and Definitions                                  */
/* ------------------------------------------------------------------------ */

#define SMP_TMOUT_MSECS     5000  /* FA2100 TDM timeout in 0.1 secs            */
#define RD_PKT_TMOUT_MSECS   500 /* Packet timeout in 0.1 secs                */
#define HSS_RETRAIN_MSECS    300  /* HSS quiet desired before packet retry  */
#define HSS_TMOUT_MSECS     5000  /* HSS timeout, giving up wait for quiet  */

#define FDR_DUMP_PKT_WDS     512  /* FDR Words per Dump Packet              */
#define CVR_DUMP_PKT_WDS     252  /* CVR Words per Dump Packet              */

#define NO_DUMP_MODE           0  /* Dump Mode Not Specified                */
#define FDR_FULL_DUMP          1  /* Full Dump of FDR Flash Memory          */
#define FDR_TIME_DUMP          2  /* Dump "FDR Last N Recorded Mins"        */
#define FDR_MARKER_DUMP        3  /* Dump "FDR Marker Range                 */
#define FDR_FROM_LAST_DUMP     4  /* Dump "FDR from Last Dump"              */
#define CVR_DUMP               5  /* CVR Dump                               */

/************************************************************************/
/***                                                                  ***/
/**                    DATA STRUCTURE DEFINITIONS                      **/
/***                                                                  ***/
/************************************************************************/

/************************************************************************/
/***                                                                  ***/
/**                    DATA SPACE                                      **/
/***                                                                  ***/
/************************************************************************/
word pkt_bfr [FDR_DUMP_PKT_WDS];

/************************************************************************/
/***                                                                  ***/
/**                    PROGRAM SPACE                                   **/
/***                                                                  ***/
/************************************************************************/
//-------------------------------------------------------------------------------------------------------------
// WriteArray
//--------------------------------------------------------------------------------------------------------------*/


/* ------------------------------------------------------------------------ */
/* reportf  - Formatted reporting messages (RNS)                            */
/* ------------------------------------------------------------------------ */
void (*_reporter)(char *) = PutString_Block;
void reportf(char *fmt, ...) {
    static char report_buffer[256];

    report_buffer[0]=0;
    
    if (_reporter != NULL) {
        #ifdef SHOW_TIMING
            sprintf (report_buffer, "%7.7u:", to_ms_since_boot(get_absolute_time()));
        #endif
        sprintf (report_buffer+strlen(report_buffer),fmt,
                                                   (long *)(&fmt+1)[0],
                                                   (long *)(&fmt+1)[1],
                                                   (long *)(&fmt+1)[2],
                                                   (long *)(&fmt+1)[3]);
        sprintf (report_buffer+strlen(report_buffer),"\xd\xa");
//      USBUART_PutString_Block (report_buffer); 
        (*_reporter) (report_buffer);
    }
}
/* ------------------------------------------------------------------------ */
/* reset_TDM_Rx_FIFO  -  Reset and Flush TDM Rx FIFO                        */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine flushes and resets the TDM Rx FIFO.                         */
/*                                                                          */
/* ------------------------------------------------------------------------ */

static void reset_TDM_Rx_FIFO (void){
   while (!EmptyQ (&RxTDMQ))
        PopQ(&RxTDMQ);

//   while (PopQ(&RxTDMQ) != 0xffffffff);
}

/* ------------------------------------------------------------------------ */
/* reset_HSS_Rx_FIFO  -  Reset and Flush HSS Rx FIFO                        */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine flushes and resets the HSS Rx FIFO.                         */
/*                                                                          */
/* ------------------------------------------------------------------------ */

static void reset_HSS_Rx_FIFO (void) {
   while (!EmptyQ (&RxHSSQ))
        PopQ(&RxHSSQ);

//   while (PopQ(&RxHSSQ) != 0xffffffff);
}

/* ------------------------------------------------------------------------ */
/* send_SMP_word  -  Xmit Word to SMP Via the CICC TDM Channel              */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine sends a single word to the FA2100 via the TDM Transmit      */
/* channel.                                                                 */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static void send_SMP_word (word tx_wrd) {
//    unsigned tx;
//   io_rw_16 *TDMfifoDAT =  &tdm.pio->txf[(tdm.sm+1)];
//   io_rw_16 *TDMfifoADR =  &tdm.pio->txf[(tdm.sm+2)];
    unsigned RetryCnt=0;

//    sleep_ms(1);

    RetryCnt = 0;
    while ((pio_sm_is_tx_fifo_full (tdm.pio, tdm.sm+1)) && (RetryCnt++<2000))
  
    RetryCnt = 0;
    while ((!gpio_get(17)) && (RetryCnt++<2000));  // Hack to avoid skew in writing ADR vs DAT, wait for FRM pulse

    RetryCnt = 0;
    while ((gpio_get(17)) && (RetryCnt++<200));  // Hack to avoid skew in writing ADR vs DAT, wait for FRM pulse

    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+2)]) = 0x4000;
    *((io_rw_16 *) &tdm.pio->txf[(tdm.sm+1)]) = tx_wrd;

//    *TDMfifoADR = 0x4000;
//    *TDMfifoDAT = tx_wrd;
//    printf ("+");
/*
    tx= 0x40000000;
    tx |= tx_wrd & 0xffff;
    PushQ (&TxTDMQ, tx);    

    if (KickTx_TDM) {
        TxTDM_isr_SetPending ();
        KickTx_TDM = 0;
    }
*/    
}

/* ------------------------------------------------------------------------ */
/* wait_HSS_word_or_tmout  - Wait for word on HSS or Timeout (RNS)          */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* Inputs  : Pointer to location to receive word; Timeout in msecs          */
/*           Timeout=0 means receive if already available, don't wait       */
/*                                                                          */
/*           Also takes start time as an input to avoid checking time       */
/*           with every single byte when receiving packets, which would     */
/*           slow things down tremendously.                                 */
/*                                                                          */
/* Outputs : Word returned in specified location                            */
/*                                                                          */
/* Returns : TRUE if word received (and stored at *pWrd), FALSE if timeout  */
/*                                                                          */
/* ------------------------------------------------------------------------ */
int wait_HSS_word_or_tmout (unsigned int stime, word *pWrd, word tmout_msecs) {
   unsigned rxWord;
    
   while ((to_ms_since_boot(get_absolute_time())-stime) <= tmout_msecs) {
        if (!EmptyQ(&RxHSSQ)) {
            rxWord = PopQ (&RxHSSQ);
            *pWrd = rxWord & 0xffff;
            return(TRUE);
        }
   }      
   return (FALSE);
}

/* ------------------------------------------------------------------------ */
/* wait_SMP_msg_or_tmout  -  Wait for SMP TDM Msg Word or Timeout           */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine waits for an SMP message word via the TDM interface. It     */
/* will continue to poll the SMP-to-GSE receive channel for a specified     */
/* number of milliseconds. If a message is received, it is written into the */
/* word location specified by the caller. If a timeout occurs, a "failure"  */
/* indicator is returned to the caller.                                     */
/*                                                                          */
/* Inputs  : Pointer to word location to receive msg; Timeout in msecs      */
/*           Timeout=0 means receive if already available, don't wait (RNS) */
/*                                                                          */
/* Outputs : Msg word returned in specified word location                   */
/*                                                                          */
/* Returns : TRUE if a msg word received, FALSE otherwise                   */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int wait_SMP_msg_or_tmout (word * pWrd, word tmout_msecs) {
    word tdat;
    unsigned int     stime;
    unsigned TDMword;

    stime = to_ms_since_boot(get_absolute_time()); /* Get start time */

    /* --------------------------------------------------------------------- */
    /* Wait for TDM word or a timeout at the SMP-to-GSE TDM FIFO.  This loop */
    /* will continue to execute until a  word is received or a               */
    /* timeout occurs. The caller of this routine must consider the effects  */
    /* of a stream of trace words on the timeout value. Note also that a     */
    /* single TDM word occupies three entries in the TDM FIFO. These are the */
    /* Hi and Lo bytes of TDAT, followed by the TADD value. The TADD value   */
    /* is discarded, but the Hi and Lo bytes of TDAT are combined into a     */
    /* 16-bit word, and returned in the caller-specified location.           */
    /* --------------------------------------------------------------------- */

    pstring[0] = 0;
    while (TRUE)  {/* Loop terminated internally */
//        while (((TDMword = PopQ(&RxTDMQ)) & 0x0f000000) != 0x08000000) {
        while (EmptyQ(&RxTDMQ)) {
//            ((TDMword = PopQ(&RxTDMQ)) & 0x0f000000) != 0x08000000) {
            if ((to_ms_since_boot(get_absolute_time()) - stime) >= tmout_msecs) {
                return (FALSE); /* Return timeout indication */
            }
        }

//        tdat = TDMword & 0xffff; 
        tdat = PopQ(&RxTDMQ); 

        #if IGNORE_FD_RETRNSMT_WDS /* Temp */
        /* -------------------------------- */
        /* Ignore any "FD Retransmit" words */
        /* -------------------------------- */
        if ((tdat & S2G_FD_RTRNSMT_DATA_MASK) == S2G_FD_RTRNSMT_DATA) {
            continue;
        }
        #endif

        /* -------------------------------- */
        /* Ignore FDP words - RDS HACK      */
        /* -------------------------------- */
        if ((tdat & 0xf000) == 0xf000) {
            continue;
        }


        if ((tdat & S2G_TDM_TRC_MASK) == S2G_TDM_TRC_WORD) {
            sprintf (pstring+strlen(pstring),"%c",tdat & 0x7f);
            if (strlen (pstring) >= 62) {
//                USBUART_PutString_Block (pstring);
                PutString_Block (pstring);
                pstring[0]=0;
            }
            continue;
        }

    if (strlen (pstring) > 0) 
//        USBUART_PutString_Block (pstring);
        PutString_Block (pstring);

    *pWrd = tdat; /* Non-trace word, into caller's loctn */
//      reportf ("TDM Rx 0x%04x",tdat);
    return (TRUE); /* Return "success" indication */
    }
}

/* ------------------------------------------------------------------------ */
/* rd_pkt_or_tmout  -  Wait for Packet from HSS or Timeout                  */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine waits for a full packet via the HSS or a timeout.           */
/*                                                                          */
/* Inputs  : Pkt size in wrds, Ptr to wrd CRC rcv loctn; Timeout in msecs.  */
/*                                                                          */
/* Outputs : CRC word returned in specified word location                   */
/*                                                                          */
/* Returns : TRUE if a Pkt received, FALSE otherwise                        */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int rd_pkt_or_tmout (word pkt_wds, word * pCRC, word tmout_msecs) {
    word crc, i, hss_wd;

    /* --------------------------------------------------- */
    /* Wait for a full packet via HSS channel or a timeout */
    /* --------------------------------------------------- */
    unsigned int stime = to_ms_since_boot(get_absolute_time());
    for (i = 0, crc = 0x0000; i < pkt_wds; i++) {                   
        /* --------------------------------------------------------------- */
        /* Wait for the HSS word or a timeout. If a timeout                */
        /* occurs, return to caller with timeout indication.               */
        /* --------------------------------------------------------------- */
        if (! wait_HSS_word_or_tmout(stime, &hss_wd, tmout_msecs)) {
            reportf ("Timeout on HSS Pkt, word %u",i);
            return(FALSE);
        }

        crc = crc16_word (crc, hss_wd); /* Updt CRC with new word */
        pkt_bfr[i] = hss_wd; /* Store word in packet buffer */
    }
   
    /* ----------------------------------------------- */
    /* Store CRC in caller's loctn - Rtrn success code */
    /* ----------------------------------------------- */
    *pCRC = crc & 0xffff;
                    
    return (TRUE);
}

/* ------------------------------------------------------------------------ */
/* get_hdr_or_tmout  -  Wait for Dump-File Hdr Text from SMP or Timeout     */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine waits for the Dump-File Header Text from the SMP via the    */
/* TDM SMP-to-GSE channel, or a timeout. It will poll the SMP-to-GSE Rx     */
/* channel for "ASCII Text" messages until one is received with the "End"   */
/* mark, or the specified number of timeout milliseconds has elapsed while  */
/* waiting for a message. As the messages are received, an ASCII character  */
/* is extracted from each one and written to the Dump File. If a timeout    */
/* occurs, or an invalid word is received, a "failure" code is returned to  */
/* the caller.                                                              */
/*                                                                          */
/* Inputs  : Timeout in msecs                                               */
/*                                                                          */
/* Outputs : ASCII Text is written to Dump File                             */
/*                                                                          */
/* Returns : TRUE if no timeout and "End" msg detected, FALSE otherwise     */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int get_hdr_or_tmout (word tmout) {
   word smp_wd, cnt = 0;
   word wc = 0;

    /* ------------------------------------------------------------------- */
    /* Keep processing SMP-to-GSE "ASCII Text" messages until timeout, bad */
    /* message, or a valid "end word" message is detected.                 */
    /* ------------------------------------------------------------------- */
    do {
        /* ---------------------------------------------------------- */
        /* Wait for, and validate next "ASCII Text" msg word from SMP */
        /* ---------------------------------------------------------- */
        if (! wait_SMP_msg_or_tmout (&smp_wd, tmout)) { /* If not rcvd */
            reportf ("Timeout on ASCII Header Word");
            return (FALSE);
        }

	    wc++;
        if ((smp_wd & S2G_ASCII_TEXT_MASK) != S2G_ASCII_TEXT) {
            reportf ("Expctd ASCII Hdr Word - Rcvd 0x%04x", smp_wd);
            return (FALSE);
        }

        cnt++; /* Bump the count of words received */

        /* ------------------------------------------------------- */
        /* Extract ASCII Char from msg word and write to Dump File */
        /* ------------------------------------------------------- */
        if (fp_dump != NULL) {
            f_putc (smp_wd & 0x007f, fp_dump); /* Hdr char to output file */
        }

    } while ((smp_wd & S2G_ASCII_TEXT_END_FLAG) == 0); /* Not "end" yet */
                                      
    /* --------------------------------------------------------------------- */
    /* If the Header Text contained an odd number of characters, output an   */
    /* additional "space" character so that an even number of characters are */
    /* written to the file. This is done to maintain "word alignment" in the */
    /* output file for convenience in the "PC" (Intel) world.                */
    /* --------------------------------------------------------------------- */
    if (cnt & 0x0001) { /* If odd number of chars in Hdr Text */
        if (fp_dump != NULL) {
            f_putc (' ', fp_dump); /* Space char to output file */
        }
    }

    /* -------------------------- */
    /* Return with "Success" code */
    /* -------------------------- */
    return (TRUE);
}


/* ------------------------------------------------------------------------ */
/* execute_dump  -  Execute CVR or FDR Dump Operation Until Done or Timeout */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/* This routine implements the "FDR or CVR Dump" operation. Upon entry to   */
/* this routine, the "Dump" has already been initiated by the User.         */
/*                                                                          */
/* This routine sends the "Set Dump Packet Size" command (if required) and  */
/* the "FDR or CVR Dump" command to the SMP via the TDM bus, then waits for */
/* the appropriate response sequence. Once the required responses have been */
/* received, the routine sends the first "Next Packet Go" command to the    */
/* SMP, then starts polling the High-Speed Serial port in order to monitor  */
/* the Dump Data coming from Flash memory. The incoming data is stored and  */
/* the block CRC is checked. A "Block CRC" message is received from the SMP */
/* after the Block has been transferred. If this message agrees with the    */
/* computed CRC, then the block can be stored and a new "Next Packet Go"    */
/* command can be sent to the SMP for the next block. Whenever a block CRC  */
/* validation fails, a "Previous Packet Go" message is sent to the SMP.     */
/* This causes the SMP to repeat the previous block.                        */
/*                                                                          */
/* When the SMP is sending the last packet of a "Data Dump", the packet is  */
/* marked with a "Done" flag. If the CRC validation is successful on the    */
/* last packet, the GSE sends a "Terminate Dump" command to the SMP.        */
/*                                                                          */
/* This routine will terminate and return to the caller only after the Dump */
/* operation has been terminated, or if a timeout occurs while waiting for  */
/* a TDM or High-Speed Serial transmission from the SMP. This can occur if  */
/* the FA2100 has a problem of some sort.                                   */
/*                                                                          */
/* NOTE: There is an underlying assumption that the "packet size" can be    */
/* evenly divided into a Flash erase block.                                 */
/*                                                                          */
/* Inputs  : Mode, Number of Mins (0 if Not Used), Marker (0 if Not Used),  */
/*         : Ptr to word location for Retries count                         */
/*                                                                          */
/* Outputs : Messages to SMP, Data to Store, etc., Retry counter            */
/*                                                                          */
/* Returns : TRUE if Success, FALSE otherwise                               */
/*                                                                          */
/* ------------------------------------------------------------------------ */
boolt execute_dump (void) {
    word  msg, blk_crc, smp_crc,  pkt_cnt, pkt_tries;
    word qual, blk_pairs, pkt_wds;
    int done, retry;
    int retries = 0;
    unsigned int WriteCount;
    word mode = FDR_FULL_DUMP;
    word mins = 0;
    word mkr=0; 
    char PacifierString[20];

    
    #if PKT_DISPLAY /* If Packet Display enabled */
    char pktdisp[5*8+1];
    #endif
        
    // clear fifos             
    reset_TDM_Rx_FIFO ();
    reset_HSS_Rx_FIFO ();

    /* --------------------------------------------------------------------- */
    /* If not a "CVR Dump" Send Packet Size Cmd to the SMP and get Response. */
    /* For a CVR Dump, the Packet Size is currently fixed, based on the      */
    /* constant, "CVR_DUMP_FRMS_PER_PKT".                                    */
    /* --------------------------------------------------------------------- */
   
    if (mode != CVR_DUMP) { /* If any form of FDR dump */
        /* --------------------------------------------------------------- */
        /* Set Packet Size in words for calls to the "Read Packet" routine */
        /* --------------------------------------------------------------- */
        pkt_wds = FDR_DUMP_PKT_WDS; /* Set packet size */

        /* ------------------------ */
        /* Send Pkt Size Msg to SMP */
        /* ------------------------ */
        send_SMP_word (G2S_DUMP_PKT16X_WDS | ((FDR_DUMP_PKT_WDS >> 4) & 0x00ff));

        /* -------------------------------------------- */
        /* Wait for the "Packet Size Echo" from the SMP */
        /* -------------------------------------------- */
        if (! wait_SMP_msg_or_tmout (&msg, SMP_TMOUT_MSECS)) { /* If not rcvd */
            reportf ("Timeout on Pkt Sz Echo from SMP");
            return (FALSE); /* Exit with "Fail" status */
        }

        if (msg != (G2S_DUMP_PKT16X_WDS | ((FDR_DUMP_PKT_WDS >> 4) & 0x00ff))) {
            reportf ("Expctd Pkt Sz Echo from SMP, Rcvd 0x%04x", msg);
            return (FALSE); /* Exit with "Fail" status */
        }
        
        reportf("Got pkt size: 0x%04X", msg);
    } else { /* This is a CVR Dump */
        /* --------------------------------------------------------------- */
        /* Set Packet Size in words for calls to the "Read Packet" routine */
        /* --------------------------------------------------------------- */
        pkt_wds = CVR_DUMP_PKT_WDS; /* Set packet size */
    }

    /* ------------------------------- */
    /* Send CVR or FDR Dump cmd to SMP */
    /* ------------------------------- */
    if (mode == CVR_DUMP) { /* If CVR Dump */
        send_SMP_word (G2S_CVR_DUMP | (mins & G2S_CVR_DUMP_QUAL_MASK));
    } else if ((mode == FDR_FULL_DUMP) || (mins == 0)) { /* This indicates Full dump */
        send_SMP_word (G2S_FDR_DUMP | G2S_FDR_FULL_DUMP);
    } else if (mode == FDR_TIME_DUMP) { /* Doing "time" dump */
        send_SMP_word (G2S_FDR_DUMP | G2S_FDR_TIME_DUMP);
        /* ---------------------------------------------------------------- */
        /* Dump time is specified in ten-minute units, so the minutes value */
        /* passed to this routine must be divided by ten.                   */
        /* ---------------------------------------------------------------- */
        send_SMP_word (G2S_FDR_DUMP_TIME | ((mins / 10) & 0x00ff));
    } else { /* Must be "Marker" or "Last Dump" mode */
        if (mode == FDR_FROM_LAST_DUMP) {
            qual = 0x000f;
        } else { /* "Dump from Marker" mode */
            qual = (mkr << 4) & 0x00f0;
        }
        send_SMP_word (G2S_FDR_DUMP | qual);
    }

    /* ------------------------------------------------------------------ */
    /* Wait for the "Number of Block Pairs to be Dumped" Msg from the SMP */
    /* ------------------------------------------------------------------ */
    if (! wait_SMP_msg_or_tmout (&msg, SMP_TMOUT_MSECS)) { /* If not rcvd */
        reportf ("Timeout on Blk-Pairs Msg from SMP");
        return (FALSE); /* Exit with "Fail" status */
    }

    if ((msg & S2G_DUMP_BLK_PAIRS_MASK) != S2G_DUMP_BLK_PAIRS) {
        reportf ("Expctd Blk Pairs from SMP, Rcvd 0x%04x", msg);
        return (FALSE); /* Exit with "Fail" status */
    }

    /* --------------------------------------------------------------------- */
    /* Extract "Block Pairs to be Dumped" from Message and Report it. If the */
    /* "Block Pairs" value is "zero", then an invalid dump request has been  */
    /* made, and the SMP will not be proceeding with the Dump operation.     */
    /* --------------------------------------------------------------------- */
    blk_pairs = msg & S2G_DUMP_BLK_PAIRS_QMASK; /* Get Blk Pairs from Msg */
    if (blk_pairs == 0) { /* If "zero" pairs */
        reportf ("SMP Zero Blks Response Indicates Invalid Dump Request");
        return (FALSE); /* Exit with "Fail" status */
    }
    reportf ("SMP Will Dump %u Block Pairs", blk_pairs);

    /* ------------------------------------------------------------------- */
    /* Get Dump-File Hdr Text messages from the SMP and write to Dump File */
    /* ------------------------------------------------------------------- */
    if (! get_hdr_or_tmout (SMP_TMOUT_MSECS)) { /* If not rcvd */
        reportf ("Error on File Header Text from SMP");
        return (FALSE); /* Exit with "Fail" status */
    }

    /* ------------------------------------------- */
    /* Wait for the "Ready" indicator from the SMP */
    /* ------------------------------------------- */
    if (! wait_SMP_msg_or_tmout (&msg, SMP_TMOUT_MSECS)) { /* If not rcvd */
        reportf ("Timeout on RDY response from SMP");
        return (FALSE); /* Exit with "Fail" status */
    }

    if ((msg & S2G_RDY_TO_DUMP_MASK) != S2G_RDY_TO_DUMP) {
        reportf ("Expctd Dump Rdy from SMP, Rcvd 0x%04x", msg);
        return (FALSE); /* Exit with "Fail" status */
    }

    /* ------------------ */
    /* Top of "Dump" loop */
    /* ------------------ */
    done    = FALSE;
    retry   = FALSE;
    pkt_cnt = 0;
    
    /* -------------------------------------------------------- */
    /* Flush HSS FIFO before sending the "Go" command to FA2100 */
    /* -------------------------------------------------------- */
    reset_HSS_Rx_FIFO (); /* Reset and flush HSS Rx FIFO */

    while (TRUE) { /* Loop terminated internally */
//        if (!EmptyQ(&RxHSSQ)) {
//            reset_HSS_Rx_FIFO ();
//            reportf ("Extra data in HSS queue");
//        }

        /* -----------------------------------------------r------- */
        /* Check for a "User Abort" command, and Exit if detected */
        /* ------------------------------------------------------ */
//        if (USBUART_DataIsReady()) {
    
        if (getchar_timeout_us(0) !=  PICO_ERROR_TIMEOUT) {
//        if (tud_cdc_n_available(0)) {
            reportf ("User Abort Detected, %u-Word Pkt Count = 0x%04x ",
                  pkt_wds, pkt_cnt);
            break;          // Exit with "Fail" status 
        }

        /* ---------------------------------------------------------------- */
        /* If "retry" flag is set, check for Max tries and Exit if reached. */
        /* If no "retry" needed, check "done" flag and exit if done. If not */
        /* done, issue the "Next Packet Go" command to the FA2100.          */
        /* ---------------------------------------------------------------- */
        if (retry) { /* Current packet not sent successfully */
            ++retries; /* Bump the accumulated retries counter */

            if (++pkt_tries == 5) { /* If reached max tries for a packet */
                reportf ("Max Pkt Tries (%u) Attempted,"
                        " %u-Word Pkt Count = 0x%04x",
                            pkt_tries, pkt_wds, pkt_cnt);
                break;          /* Exit with "Fail" status */
            }

            /* -------------------------------------------------------- */
            /* OK to try again - Begin by sending the "Prev Pkt Go" cmd */
            /* -------------------------------------------------------- */
            retry = FALSE; /* Reset the "retry" flag */
            send_SMP_word (G2S_PREV_PKT_GO);
        } else if (done) { /* No retry, Done rcvd, Issue "Terminate Dump" cmd */
            send_SMP_word (G2S_TERMINATE_DUMP);
            reportf ("Successful Termination, %u Retries", retries);
            return (TRUE); /* Normal "success" exit */
        } else { /* No retry and not done - Issue "Next Block Go" cmd */
            pkt_tries = 0; /* Initialize "packet-tries" counter */
            send_SMP_word (G2S_NEXT_PKT_GO);
        }
  
        /* ------------------------------------------------------------- */
        /* Wait for the two msgs from SMP containing CRC Hi and Lo bytes */
        /* ------------------------------------------------------------- */
        if (! wait_SMP_msg_or_tmout (&msg, SMP_TMOUT_MSECS)) { /* If not rcvd */
            // reportf ("Timeout on CRC Hi Msg from SMP");
            // retry = TRUE; /* Set flag for retry */
            // continue; /* To top of "while" loop */
            ssd1306_clear(&disp);
            ssd1306_draw_string(&disp, 0, 0, 2, "Error");
            ssd1306_draw_string(&disp, 0, 20, 2, "Timeout");
            ssd1306_show(&disp);
            return (FALSE); // Exit with "Fail" status
        }

        if ((msg & S2G_BLOCK_CRC_MASK) != S2G_BLOCK_CRC) {
            reportf ("Expctd CRC Msg from SMP, Rcvd 0x%04x", msg);
            retry = TRUE; /* Set flag for retry */
            continue; /* To top of "while" loop */
        }

        smp_crc = (msg & 0xff) << 8; /* Assume Hi byte */
        
        if (! wait_SMP_msg_or_tmout (&msg, SMP_TMOUT_MSECS)) { /* If not rcvd */
            reportf ("Timeout on CRC Lo Msg from SMP");
            retry = TRUE; /* Set flag for retry */
            continue; /* To top of "while" loop */
        }

        if ((msg & S2G_BLOCK_CRC_MASK) != S2G_BLOCK_CRC) {
            reportf ("Expctd CRC Msg from SMP, Rcvd 0x%04x", msg);
            retry = TRUE; /* Set flag for retry */
            continue; /* To top of "while" loop */
        }

        smp_crc += msg & 0x00ff; /* Assume Lo byte */

        /* ----------------------------------------- */
        /* Read dump data via High-Speed serial port */
        /* ----------------------------------------- */
        if (! rd_pkt_or_tmout (pkt_wds, &blk_crc, RD_PKT_TMOUT_MSECS)) {
            reportf ("Error or Timeout on Pkt Data from SMP");
            retry = TRUE; /* Set flag for retry */
            continue; /* To top of "while" loop */
        }

        /* ----------------------------------------------------------- */
        /* Set "done" flag if flag bit set in the CRC message from SMP */
        /* ----------------------------------------------------------- */
        done = done  | ((msg & S2G_CRC_DONE_FLAG) != 0);

        #if PKT_DISPLAY /* If Packet Display enabled */
        /* -------------------------------------------------------- */
        /* If Packet Display is enabled, write packet to the screen */
        /* -------------------------------------------------------- */
        pktdisp[0] = '\0';
        for (i = 0; i < pkt_wds; i++) {
            sprintf (pktdisp+strlen(pktdisp), " 0x%04x", pkt_bfr[i]);

            if ((i % 8) == 7 || i == pkt_wds-1) {
                reportf (pktdisp);
                pktdisp[0] = '\0';
            }
        }
        #endif

        /* ----------------------------------------------------------------- */
        /* Set "retry" flag if CRC from SMP does not match local calculation */
        /* ----------------------------------------------------------------- */
        if ((smp_crc != blk_crc) && (done == 0)) {
            reportf ("Blk %04x CRC Error: SMP CRC = 0x%04x, PI CRC = 0x%04x",
                        pkt_cnt,smp_crc, blk_crc);
            retry = TRUE; /* Set flag for retry */

            if (!EmptyQ(&RxHSSQ)) {
                reset_HSS_Rx_FIFO ();
                reportf ("Extra data in HSS queue");
        }
            continue; /* To top of "while" loop */
        }

        /* ------------------------------------------------------------- */
        /* Fall-thru to here indicates packet was received without error */
        /* ------------------------------------------------------------- */
        pkt_cnt++;    // count good packet (RNS)

        #if PKT_COUNT_REPORT
        /* ------------------------------------------------------- */
        /* Report progress if packet count report interval reached */
        /* ------------------------------------------------------- */
        if ((pkt_cnt % 32) == 0) {
            reportf ("%u-Word Pkt Count = 0x%04x ", pkt_wds, pkt_cnt);
        }
        #endif

        /* -------------------------------------------------------------- */
        /* If file is open, write the packet to the output file           */
        /* -------------------------------------------------------------- */
        if (fp_dump != NULL) {
            f_write(fp_dump,pkt_bfr,2*pkt_wds,&WriteCount);
        }

        /* -------------------------------------------------------- */
        /* User Pacifier                                            */
        /* -------------------------------------------------------- */
        if ((pkt_cnt % 128) == 0) {
            sprintf (PacifierString,"%i",pkt_cnt/128);
            ssd1306_clear(&disp);
            ssd1306_draw_string(&disp, 0, 0, 2, "Downloading");
            ssd1306_draw_string(&disp, 0, 20, 2, "FDR Data..");
            ssd1306_draw_string(&disp,0,40,2,PacifierString);
            ssd1306_draw_string(&disp, 40,40, 2," Blocks");
            ssd1306_show(&disp); 
        }


    }
    
    reportf ("Dump Incomplete, %u Retries", retries);
    return (FALSE);
}
/* [] END OF FILE */

/* ------------------------------------------------------- */
/* List of files in the SD card */
/* ------------------------------------------------------- */

#include <stdio.h>
#include "ff.h" // Include the FatFs header file

FATFS fatFs;
FRESULT fr;

void list_files(void) {
    DIR dir;
    FILINFO fno;
    FRESULT fr;
    unsigned int fileCount = 0; // Initialize file counter
    char displayBuffer[64]; // Buffer to hold the display text

    // Mount the internal storage
    fr = f_mount(&fatFs, "", 0);
    if (fr != FR_OK) {
        printf("Failed to mount filesystem\n");
        return;
    }

    // Open the directory
    fr = f_opendir(&dir, "/"); // Open the root directory
    if (fr == FR_OK) {
        for (;;) {
            fr = f_readdir(&dir, &fno); // Read a directory item
            if (fr != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir
            
            // Check if it is a file, does not start with ._, and ends with .FDR
            if (!(fno.fattrib & AM_DIR) && strncmp(fno.fname, "._", 2) != 0) {
                // Check if the file name ends with .FDR
                char* ext = strstr(fno.fname, ".FDR");
                if (ext != NULL && strcmp(ext, ".FDR") == 0) { // Make sure it ends with .FDR
                    fileCount++; // Increment file counter

                    // Open the file to get its size
                    FIL file;
                    fr = f_open(&file, fno.fname, FA_READ);
                    if (fr == FR_OK) {
                        // Get the file size
                        UINT fileSize = f_size(&file);
                        // Convert file size to megabytes
                        double fileSizeMB = (double)fileSize / (1024 * 1024);

                        // Prepare the display text with file name and size
                        snprintf(displayBuffer, sizeof(displayBuffer), "File: %s (%.2f MB)\n", fno.fname, fileSizeMB);

                        // Print the file info to the terminal
                        printf("%s", displayBuffer);

                        // Close the file
                        f_close(&file);
                    }
                }
            }
        }
        f_closedir(&dir);
    } else {
        printf("Failed to open directory\n");
    }

    // Display the file count on the SSD1306
    snprintf(displayBuffer, sizeof(displayBuffer), "%u file%s", fileCount, fileCount == 1 ? "" : "s");
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, displayBuffer);
    ssd1306_show(&disp);

    // Unmount the filesystem
    f_mount(NULL, "", 0);

    // Optional: Return to a default screen or message after displaying the file count
    sleep_ms(3000); // Wait for a bit before clearing the display
    displayReset();
}




/* ------------------------------------------------------- */
/* Create Custom Tail */
/* ------------------------------------------------------- */

void DisplayWordUpdate() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Tails:");
    ssd1306_draw_string(&disp, 0, 20, 2, selectedWord);
    ssd1306_show(&disp);
}

void updateSelectedWord(int direction) {
    char currentChar = selectedWord[selectedLetterIndex];

    if (direction > 0) { // Increment character
        if (currentChar >= '0' && currentChar < '9') {
            // If current character is between 0 and 8, simply increment
            currentChar++;
        } else if (currentChar == '9') {
            // If current character is 9, move to A
            currentChar = 'A';
        } else if (currentChar >= 'A' && currentChar < 'Z') {
            // If current character is between A and Y, simply increment
            currentChar++;
        } else if (currentChar == 'Z') {
            // If current character is Z, wrap around to 0
            currentChar = '0';
        } else {
            // Default case, if somehow outside expected range
            currentChar = '0';
        }
    } else { // Decrement character
        if (currentChar > '0' && currentChar <= '9') {
            // If current character is between 1 and 9, simply decrement
            currentChar--;
        } else if (currentChar == '0') {
            // If current character is 0, move to Z
            currentChar = 'Z';
        } else if (currentChar > 'A' && currentChar <= 'Z') {
            // If current character is between B and Z, simply decrement
            currentChar--;
        } else if (currentChar == 'A') {
            // If current character is A, wrap around to 9
            currentChar = '9';
        } else {
            // Default case, if somehow outside expected range
            currentChar = '9';
        }
    }

    selectedWord[selectedLetterIndex] = currentChar;
    DisplayWordUpdate();
}
// Define an enum for different tail pages
typedef enum {
    TAIL_PAGE_ATMO,
    TAIL_PAGE_CLASSIC,
    TAIL_PAGE_NEXTGEN
} TailPage;

// Define a variable to keep track of the current tail page
TailPage currentTailPage = TAIL_PAGE_ATMO; // Default value, adjust as needed

void handleInputs() {
    bool currentStateUp, currentStateDown, currentStateNext, currentStatePrev;
    bool currentStateAccept;
    selectedLetterIndex = 0; // Start from the first character

    // Initial Debounce Delay
    sleep_ms(300); // Wait for 300ms to ensure no residual input from previous actions

    while (true) {
        currentStateDown = !gpio_get(5); // Assuming GPIO 5 is "down"
        currentStateUp = !gpio_get(3); // Assuming GPIO 3 is "up"
        currentStateNext = !gpio_get(4); // Assuming GPIO 4 is "next"
        // currentStateAccept = !gpio_get(2); // Assuming GPIO 2 is "accept", replace with correct GPIO

        if (currentStateUp) {
            updateSelectedWord(1);
            sleep_ms(200); // Adjust timing as needed for debounce
        } else if (currentStateDown) {
            updateSelectedWord(-1);
            sleep_ms(200); // Adjust timing as needed for debounce
        } else if (currentStateNext) {
            if (selectedLetterIndex < 6) { // Last character is at index 6
            selectedLetterIndex++;
            DisplayWordUpdate();
        } else {
            // If on the last character and next is pressed, assume completion
            strncpy(selectedTail, selectedWord, sizeof(selectedTail) - 1);
            DisplayWordUpdate();
            // Append the new tail to the specified file
            if (strlen(filename) > 0) {
                appendNewTail(selectedTail, "N/A", filename);
            }

            ReadDFDR(); // Start FDR download with the current selectedWord
            break; // Exit input loop after starting download
        }
        sleep_ms(200); // Debounce delay
    }
}
}


void appendNewTail(const char* tail, const char* serial, const char* filename) {
    FIL fil; // File object
    FRESULT fr;

    // Open the file for appending, create it if it does not exist
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        char buffer[256]; // Assuming a tail and serial won't exceed this length

        // Construct the line to append
        sprintf(buffer, "%s\t%s\n", tail, serial);

        UINT bytesWritten;
        f_write(&fil, buffer, strlen(buffer), &bytesWritten);

        // Check if the write operation was successful
        if(bytesWritten < strlen(buffer)) {
            printf("Failed to write the full tail and serial to the file.\n");
        }

        f_close(&fil);
    } else {
        printf("Failed to open or create %s for appending.\n", filename);
    }
}


/* ------------------------------------------------------- */
/* Counting the amount of tails in each fleet */
/* ------------------------------------------------------- */

int countTotalEntries(const char* filename) {
    FIL fil;
    FRESULT fr;
    char line[256];
    int count = 0;

    fr = f_mount(&fatFs, "", 0);
    if (fr != FR_OK) {
        printf("Failed to mount filesystem, error: %d\n", fr);
        return -1;
    }

    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("Failed to open file, error: %d\n", fr);
        return -1;
    }

    // Simple count of non-empty, non-header lines
    bool pastHeader = false;
    while (f_gets(line, sizeof(line), &fil)) {
        // Optionally, skip header or initial non-entry lines
        if (!pastHeader) {
            // For example, you might check for a specific line to indicate the end of a header
            if (strstr(line, "A/C\tS/N") || strstr(line, "Tail\tSerial")) { // This could be more robust
                pastHeader = true;
            }
            continue;
        }

        // Count if line is not empty (you might want to trim whitespace before checking)
        if (line[0] != '\n' && line[0] != '\r' && line[0] != '\0') {
            count++;
        }
    }

    f_close(&fil);
    f_mount(NULL, "", 0);

    return count;
}

/* ------------------------------------------------------- */
/* Atmoshpere Tails Display */
/* ------------------------------------------------------- */

extern ssd1306_t disp; // Assuming the display is initialized elsewhere
extern FATFS fatFs;    // Assuming the FATFS object is available globally

void displayAtmo(int entryIndex) {
    char* tails[] = {
        "N601EN", "N602NN", "N603NN", "N604NN", "N605NN",
        "N606NN", "N607NN", "N608NN", "N609NN", "N610NN",
        "N611NN", "N612NN", "N613NN", "N614NN", "N615NN"
    };

    char* serials[] = {
        "15462", "15463", "15464", "15465", "15467",
        "15468", "15471", "15473", "15475", "15476",
        "15460", "15477", "15481", "15483", "15484"
    };

    int totalEntries = sizeof(tails) / sizeof(tails[0]);

    if (entryIndex == totalEntries + 1) {
        // Display "Enter Tail" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Enter");
        ssd1306_draw_string(&disp, 0, 20, 2, "Tail");
        ssd1306_show(&disp);
        atmoSelectedSerial = true;
        atmoBack = false;
        atmoConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

    if (entryIndex == totalEntries + 2) {
        // Display "Go Back" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Go");
        ssd1306_draw_string(&disp, 0, 20, 2, "Back");
        ssd1306_show(&disp);
        atmoSelectedSerial = false;
        atmoBack = true;
        atmoConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

if (entryIndex >= 1 && entryIndex <= totalEntries) {
    // Display the selected tail and serial number
    atmoSelectedSerial = false;
    atmoBack = false;
    atmoConfirmationNeeded = true;
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Atmo:"); // Display "Atmo:" label
    ssd1306_draw_string(&disp, 0, 20, 2, tails[entryIndex - 1]); // Display tail number
    ssd1306_draw_string(&disp, 0, 40, 2, serials[entryIndex - 1]); // Display serial number
    ssd1306_show(&disp);
    
    // Update selectedTail with the selected tail number
    strncpy(selectedTail, tails[entryIndex - 1], sizeof(selectedTail) - 1);
}
}

/* ------------------------------------------------------- */
/* Classic Tail Display */
/* ------------------------------------------------------- */

void displayClassic(int entryIndex) {
    char* tails[] = {
        "N500AE", "N501BG", "N502AE", "N503AE", "N504AE",
        "N505AE", "N506AE", "N507AE", "N508AE", "N509AE",
        "N510AE", "N511AE", "N512AE", "N513AE", "N514AE",
        "N515AE", "N516AE", "N517AE", "N518AE", "N519AE",
        "N520DC", "N521AE", "N522AE", "N523AE", "N524AE",
        "N702PS", "N703PS", "N705PS", "N706PS", "N708PS",
        "N709PS", "N710PS", "N712PS", "N716PS", "N718PS",
        "N719PS", "N720PS", "N723PS", "N725PS", "N525AE",
        "N526EA", "N527EA", "N528EG", "N529EA", "N530EA",
        "N531EG", "N532EA", "N533AE", "N534AE", "N535EA",
        "N536EA", "N537EA", "N538EG", "N539EA", "N540EG",
        "N541EA", "N542EA", "N543EA", "N544EA", "N545PB",
        "N546FF"
    };

    char* serials[] = {
        "10025", "10017", "10018", "10021", "10044",
        "10053", "10056", "10059", "10072", "10078",
        "10105", "10107", "10110", "10114", "10119",
        "10121", "10123", "10124", "10126", "10131",
        "10140", "10142", "10147", "10152", "10154",
        "10135", "10137", "10144", "10150", "10160",
        "10165", "10167", "10168", "10171", "10175",
        "10177", "10178", "10181", "10186", "10302",
        "10304", "10305", "10306", "10307", "10308",
        "10309", "10310", "10311", "10312", "10313",
        "10315", "10316", "10317", "10318", "10319",
        "10320", "10321", "10323", "10324", "10325",
        "10326"
    };

    int totalEntries = sizeof(tails) / sizeof(tails[0]);

    if (entryIndex == totalEntries + 1) {
        // Display "Enter Tail" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Enter");
        ssd1306_draw_string(&disp, 0, 20, 2, "Tail");
        ssd1306_show(&disp);
        classicSelectedSerial = true;
        classicBack = false;
        classicConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

    if (entryIndex == totalEntries + 2) {
        // Display "Go Back" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Go");
        ssd1306_draw_string(&disp, 0, 20, 2, "Back");
        ssd1306_show(&disp);
        classicSelectedSerial = false;
        classicBack = true;
        classicConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

    if (entryIndex >= 1 && entryIndex <= totalEntries) {
        // Display the selected tail and serial number
        classicSelectedSerial = false;
        classicBack = false;
        classicConfirmationNeeded = true;
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Classic:"); // Display "Classic:" label
        ssd1306_draw_string(&disp, 0, 20, 2, tails[entryIndex - 1]); // Display tail number
        ssd1306_draw_string(&disp, 0, 40, 2, serials[entryIndex - 1]); // Display serial number
        ssd1306_show(&disp);
        
        // Update selectedTail with the selected tail number
        strncpy(selectedTail, tails[entryIndex - 1], sizeof(selectedTail) - 1);
    }
}


/* ------------------------------------------------------- */
/* Next Gen Tails Display */
/* ------------------------------------------------------- */

void displayNextGen(int entryIndex) {
    char* tails[] = {
        "N547NN", "N548NN", "N549NN", "N550NN", "N551NN",
        "N552NN", "N553NN", "N554NN", "N555NN", "N556NN",
        "N557NN", "N558NN", "N559NN", "N560NN", "N561NN",
        "N562NN", "N563NN", "N564NN", "N565NN", "N566NN",
        "N567NN", "N568NN", "N569NN", "N570NN", "N571NN",
        "N572NN", "N573NN", "N574NN", "N575NN", "N576NN",
        "N577NN", "N578NN", "N579NN", "N580NN", "N581NN",
        "N582NN", "N583NN", "N584NN", "N585NN", "N586NN",
        "N587NN", "N588NN", "N589NN", "N590NN", "N591NN",
        "N592NN", "N593NN", "N594NN", "N595NN", "N596NN",
        "N597NN", "N598NN", "N599NN", "N600NN", "N616NN",
        "N617NN", "N618NN", "N619NN", "N629NN", "N630NN",
        "N631NN", "N632NN", "N633NN", "N634NN", "N635NN"
    };

    char* serials[] = {
        "15317", "15318", "15322", "15323", "15327",
        "15328", "15333", "15334", "15338", "15339",
        "15340", "15342", "15343", "15345", "15346",
        "15347", "15350", "15351", "15352", "15353",
        "15354", "15355", "15356", "15357", "15360",
        "15361", "15362", "15365", "15366", "15367",
        "15380", "15381", "15382", "15383", "15384",
        "15385", "15386", "15387", "15388", "15389",
        "15390", "15391", "15392", "15393", "15394",
        "15404", "15405", "15407", "15408", "15411",
        "15412", "15413", "15415", "15416", "15426",
        "15428", "15429", "15434", "15373", "15374",
        "15375", "15376", "15377", "15378", "15379"
    };

    int totalEntries = sizeof(tails) / sizeof(tails[0]);

    if (entryIndex == totalEntries + 1) {
        // Display "Enter Tail" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Enter");
        ssd1306_draw_string(&disp, 0, 20, 2, "Tail");
        ssd1306_show(&disp);
        nextGenSelectedSerial = true;
        nextGenBack = false;
        nextGenConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

    if (entryIndex == totalEntries + 2) {
        // Display "Go Back" option
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Go");
        ssd1306_draw_string(&disp, 0, 20, 2, "Back");
        ssd1306_show(&disp);
        nextGenSelectedSerial = false;
        nextGenBack = true;
        nextGenConfirmationNeeded = false;
        sleep_ms(200);
        return;
    }

    if (entryIndex >= 1 && entryIndex <= totalEntries) {
        // Display the selected tail and serial number
        nextGenSelectedSerial = false;
        nextGenBack = false;
        nextGenConfirmationNeeded = true;
        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 0, 2, "Next Gen:"); // Display "Next Gen:" label
        ssd1306_draw_string(&disp, 0, 20, 2, tails[entryIndex - 1]); // Display tail number
        ssd1306_draw_string(&disp, 0, 40, 2, serials[entryIndex - 1]); // Display serial number
        ssd1306_show(&disp);
        
        // Update selectedTail with the selected tail number
        strncpy(selectedTail, tails[entryIndex - 1], sizeof(selectedTail) - 1);
    }
}

/* ------------------------------------------------------- */
/* Menu Display */
/* ------------------------------------------------------- */

char* mainMenuItems[] = {"Send", "Download", "List"};
int numberOfItems = sizeof(mainMenuItems) / sizeof(char*);


void displayMainMenu() {
    ssd1306_clear(&disp);
    for (int i = 0; i < numberOfItems; i++) {
        char lineBuffer[32];  // Ensure buffer is large enough for your string
        // Indicate the current selection with a dot
        if (i == mainMenuSelection) {
            snprintf(lineBuffer, sizeof(lineBuffer), "* %s", mainMenuItems[i]);
        } else {
            snprintf(lineBuffer, sizeof(lineBuffer), "  %s", mainMenuItems[i]);
        }
        ssd1306_draw_string(&disp, 0, i * 20, 2, lineBuffer);
    }
    ssd1306_show(&disp);
}

/* ------------------------------------------------------- */
/* Main Menu Naviagation */
/* ------------------------------------------------------- */

void handleMainMenuNavigation() {
    if (!isOnMainMenu) return;

    static uint8_t last_joystick_state = 0xFF; // Store the last state
    static int consecutive_press_count = 0; // Counter for consecutive presses

    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output


    if (!gpio_get(5)) { // Scroll up
        mainMenuSelection = (mainMenuSelection - 1 + 3) % 3;
        displayMainMenu(); // Immediate visual feedback
        sleep_ms(200); // Debounce
    } else if (!gpio_get(3)) { // Scroll down
        mainMenuSelection = (mainMenuSelection + 1) % 3;
        displayMainMenu(); // Immediate visual feedback
        sleep_ms(200); // Debounce
    } else if (!gpio_get(4)) { // Select option
        switch (mainMenuSelection) {
            case 0:
                // Handle Send action
                // Make sure to update the display or state immediately if needed
                SendZmodem();
                break;
            case 1:
                // Transition to a different menu or function/state
                isOnMainMenu = false; // Exiting main menu
                isOnTailPageMenu = true; // Entering tail page menu
                displayTailPageMenu(); // Immediate visual feedback without additional delay
                break;
            case 2:
                // Handle List action
                // Make sure to update the display or state immediately if needed
                list_files();
                break;
        }
                sleep_ms(200);
        // Remove or minimize sleep here to ensure immediate transition
    }
    // Reset the press count if the state changes
    if (joystick_state != last_joystick_state) {
        consecutive_press_count = (joystick_state == 0xEA) ? 1 : 0;
    } else if (joystick_state == 0xEA) {
        // Increment the count if the same press continues
        consecutive_press_count++;
    }

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6: // Up
                mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
                displayMainMenu();
                printf("Moved Up - Selection: %d\n", mainMenuSelection);
                break;
            case 0xE9: // Down
                mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
                displayMainMenu();
                printf("Moved Down - Selection: %d\n", mainMenuSelection);
                break;
            case 0xEE: // Diagonal top left
                mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
                displayMainMenu();
                printf("Moved Diagonally Bottom Left\n");
                break;
            case 0xEB: // Diagonal bottom left
                mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
                displayMainMenu();
                printf("Moved Diagonally Top Left\n");
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
                mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
                displayMainMenu();
                printf("Moved Diagonally down\n");
                // Executing function based on the current menu selection
                break;
            case 0xE7:  // Slight Diagonal up move
                mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
                displayMainMenu();
                printf("Moved Diagonally Bottom Right or Slight Down\n");
                // Executing function based on the current menu selection
                printf("Right - Executing function for current selection: %d\n", mainMenuSelection);
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    } else if (consecutive_press_count >= 10) { // Check if the EF state occurred twice consecutively
        printf("Double Press Detected - Executing Special Function\n");
        switch (mainMenuSelection) {
            case 0:
                SendZmodem();
                printf("SendZmodem() executed\n");
                break;
            case 1:
                isOnMainMenu = false;
                isOnTailPageMenu = true;
                displayTailPageMenu();
                printf("Transition to Tail Page Menu\n");
                break;
            case 2:
                list_files();
                printf("list_files() executed\n");
                break;
        }
        consecutive_press_count = 0; // Reset the counter after executing
    }

    last_joystick_state = joystick_state; // Update the last known state
    sleep_ms(200); // Debounce delay
}


// void handleMainMenuNavigation() {
//     if (!isOnMainMenu) return;

//     uint8_t joystick_state = read_joystick_state(i2c0);

//     // Directly match the known joystick states to actions
//     switch (joystick_state) {
//         case 0xF6:  // Move up
//             mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Up - Selection: %d\n", mainMenuSelection);
//             break;
//         case 0xE9:  // Move down
//             mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Down - Selection: %d\n", mainMenuSelection);
//             break;
//         case 0xEE:  // Diagonal top left
//                     mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Diagonally Top Left\n");
//             // Additional actions if needed
//             break;
//         case 0xEB:  // Diagonal bottom left
//             mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Diagonally Bottom Left\n");
//             // Additional actions if needed
//             break;
//         case 0xED:  // Diagonal bottom right (shared with slight down move)
//             printf("Moved Diagonally Bottom Right or Slight Down\n");
//             // Executing function based on the current menu selection
//             switch (mainMenuSelection) {
//                 case 0:
//                     SendZmodem();
//                     printf("SendZmodem() executed\n");
//                     break;
//                 case 1:
//                     isOnMainMenu = false;
//                     isOnTailPageMenu = true;
//                     displayTailPageMenu();
//                     printf("Transition to Tail Page Menu\n");
//                     break;
//                 case 2:
//                     list_files();
//                     printf("list_files() executed\n");
//                     break;
//             }
//             break;
//         case 0xE7:  // Slight Diagonal up move
//             printf("Moved Diagonally Up\n");
//             // Executing function based on the current menu selection
//             printf("Right - Executing function for current selection: %d\n", mainMenuSelection);
//             switch (mainMenuSelection) {
//                 case 0:
//                     SendZmodem();
//                     printf("SendZmodem() executed\n");
//                     break;
//                 case 1:
//                     isOnMainMenu = false;
//                     isOnTailPageMenu = true;
//                     displayTailPageMenu();
//                     printf("Transition to Tail Page Menu\n");
//                     break;
//                 case 2:
//                     list_files();
//                     printf("list_files() executed\n");
//                     break;
//             }
//             break;
//         default:
//             if (joystick_state != 0xEF && joystick_state != 0xFF) {
//                 printf("Unknown or Neutral State\n");
//             }
//             break;
//     }

//     sleep_ms(200);  // Debounce delay
// }




// void handleMainMenuNavigation() {
//     if (!isOnMainMenu) return;

//     uint8_t joystick_state = read_joystick_state(i2c0);
   



//     // Check joystick outputs to adjust bit masks if needed
//     if (joystick_state != 0xFF && joystick_state != 0xEF) {  
//         if (joystick_state & 0x02) {  // Check for 'up'
//             mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Up - Selection: %d\n", mainMenuSelection);
//         } else if (joystick_state & 0x01) {  // Check for 'down'
//             mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Down - Selection: %d\n", mainMenuSelection);
//         } else if (joystick_state & 0x04) {  // Temporarily using 0x04 for 'right' as a placeholder
//             printf("Right - Executing function for current selection: %d\n", mainMenuSelection);
//             switch (mainMenuSelection) {
//                 case 0:
//                     SendZmodem();
//                     printf("SendZmodem() executed\n");
//                     break;
//                 case 1:
//                     isOnMainMenu = false;
//                     isOnTailPageMenu = true;
//                     displayTailPageMenu();
//                     printf("Transition to Tail Page Menu\n");
//                     break;
//                 case 2:
//                     list_files();
//                     printf("list_files() executed\n");
//                     break;
//             }
//         }
//     }

//     sleep_ms(200);  // Debounce delay
// }



// void handleMainMenuNavigation() {
//     if (!isOnMainMenu) return;

//     uint8_t joystick_state = read_joystick_state(i2c0);
//     printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

//     // Check for a failure to read joystick state
//     if (joystick_state == 0xFF) {
//         printf("Failed to read joystick\n");
//         return;  // Exit if there's a read error
//     }

//     // Check for 'up' movement only if the state is not 0xFF or other known error/state codes
//     if (joystick_state != 0xFF && joystick_state != 0xEF) {  
//         if (joystick_state & 0x01) {  // Assuming '0x01' is the bit for 'up'
//             mainMenuSelection = (mainMenuSelection - 1 + numberOfItems) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Up - Selection: %d\n", mainMenuSelection);
//         } else if (joystick_state & 0x02) {  // Assuming '0x02' is the bit for 'down'
//             mainMenuSelection = (mainMenuSelection + 1) % numberOfItems;
//             displayMainMenu();
//             printf("Moved Down - Selection: %d\n", mainMenuSelection);
//         }
//     }

//     sleep_ms(200);  // Debounce delay
// }



/* ------------------------------------------------------- */
/*  Tails Navigation */
/* ------------------------------------------------------- */
// void handleMenuNavigation() {
//     if (!isOnTailPageMenu) return;

//     if (!gpio_get(5)) { // "Scroll up" button pressed
//         tailPageMenuSelection = (tailPageMenuSelection - 1 + 4) % 4;
//         displayTailPageMenu();
//         sleep_ms(200); // Debounce delay
//     } else if (!gpio_get(3)) { // "Scroll down" button pressed
//         tailPageMenuSelection = (tailPageMenuSelection + 1) % 4;
//         displayTailPageMenu();
//         sleep_ms(200); // Debounce delay
//     } else if (!gpio_get(4)) { // "Select" button pressed
//         switch (tailPageMenuSelection) {
//             case 0: // Classic selected
//                 isOnTailPageMenu = false;
//                 isOnClassPage = true;
//                 currentIndexClassic = 1;
//                 displayClassicPage(); // Assume this function sets up the display for the "Classic" page
//                 break;
//             case 1: // Next Gen selected
//                 isOnTailPageMenu = false;
//                 isOnNextGenPage = true;
//                 currentIndexNG = 1; 
//                 displayNextGenPage(); // Assume this function sets up the display for the "Next Gen" page
//                 break;
//             case 2: // Atmo selected
//                 isOnTailPageMenu = false;
//                 isOnAtmoPage = true;
//                 currentIndexAtmosphere = 1;
//                 displayAtmospherePage(); // Assume this function sets up the display for the "Atmo" page
                
//                 break;
//             case 3: // Back to Homepage selected
//                 isOnTailPageMenu = false;
//                 isOnMainMenu = true;
//                 displayMainMenu(); // Return to the main menu
//                 sleep_ms(500);
//                 break;
//         }
//         sleep_ms(200); // Debounce delay
//     }
// }

void handleMenuNavigation() {
    static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state

    if (!isOnTailPageMenu) return;

        if (!gpio_get(5)) { // "Scroll up" button pressed
        tailPageMenuSelection = (tailPageMenuSelection - 1 + 4) % 4;
        displayTailPageMenu();
        sleep_ms(200); // Debounce delay
    } else if (!gpio_get(3)) { // "Scroll down" button pressed
        tailPageMenuSelection = (tailPageMenuSelection + 1) % 4;
        displayTailPageMenu();
        sleep_ms(200); // Debounce delay
    } else if (!gpio_get(4)) { // "Select" button pressed
        switch (tailPageMenuSelection) {
            case 0: // Classic selected
                isOnTailPageMenu = false;
                isOnClassPage = true;
                currentIndexClassic = 1;
                displayClassicPage(); // Assume this function sets up the display for the "Classic" page
                break;
            case 1: // Next Gen selected
                isOnTailPageMenu = false;
                isOnNextGenPage = true;
                currentIndexNG = 1; 
                displayNextGenPage(); // Assume this function sets up the display for the "Next Gen" page
                break;
            case 2: // Atmo selected
                isOnTailPageMenu = false;
                isOnAtmoPage = true;
                currentIndexAtmosphere = 1;
                displayAtmospherePage(); // Assume this function sets up the display for the "Atmo" page
                
                break;
            case 3: // Back to Homepage selected
                isOnTailPageMenu = false;
                isOnMainMenu = true;
                displayMainMenu(); // Return to the main menu
                sleep_ms(500);
                break;
        }
        sleep_ms(200); // Debounce delay
    }

    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    // Check for change in joystick state
    if (joystick_state != last_joystick_state) {
        if (joystick_state == 0xEA) {
            consecutive_press_count = 1;  // Start a new press count
        } else {
            consecutive_press_count = 0;  // Reset the counter if not pressed
        }
    } else if (joystick_state == 0xEA) {
        consecutive_press_count++;  // Increment if the press continues
    }

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
                tailPageMenuSelection = (tailPageMenuSelection - 1 + 4) % 4;
                displayTailPageMenu();
                printf("Moved Up - Selection: %d\n", tailPageMenuSelection);
                break;
            case 0xE9:  // Move down
                tailPageMenuSelection = (tailPageMenuSelection + 1) % 4;
                displayTailPageMenu();
                printf("Moved Down - Selection: %d\n", tailPageMenuSelection);
                break;
            case 0xEE:  // Diagonal top left
                tailPageMenuSelection = (tailPageMenuSelection - 1 + 4) % 4;
                displayTailPageMenu();
                printf("Moved Diagonally Top Left\n");
                // Additional actions if needed
                break;
            case 0xEB:  // Diagonal bottom left
                tailPageMenuSelection = (tailPageMenuSelection + 1) % 4;
                displayTailPageMenu();
                printf("Moved Diagonally Bottom Left\n");
                // Additional actions if needed
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
                tailPageMenuSelection = (tailPageMenuSelection + 1) % 4;
                displayTailPageMenu();
                printf("Moved Diagonally Bottom Right or Slight Down\n");
                // Executing function based on the current menu selection
                break;
            case 0xE7:  // Slight Diagonal up move
                tailPageMenuSelection = (tailPageMenuSelection - 1 + 4) % 4;
                displayTailPageMenu();
                printf("Moved Diagonally Up\n");
                // Executing function based on the current menu selection
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }

    // Check for double press action
    if (consecutive_press_count >= 2) {  // Double press detected as select
        printf("Double Press Detected - Executing function for current selection: %d\n", tailPageMenuSelection);
        switch (tailPageMenuSelection) {
            case 0: // Classic selected
                isOnTailPageMenu = false;
                isOnClassPage = true;
                currentIndexClassic = 1;
                displayClassicPage(); // Setup the display for the "Classic" page
                break;
            case 1: // Next Gen selected
                isOnTailPageMenu = false;
                isOnNextGenPage = true;
                currentIndexNG = 1; 
                displayNextGenPage(); // Setup the display for the "Next Gen" page
                break;
            case 2: // Atmo selected
                isOnTailPageMenu = false;
                isOnAtmoPage = true;
                currentIndexAtmosphere = 1;
                displayAtmospherePage(); // Setup the display for the "Atmo" page
                break;
            case 3: // Back to Homepage selected
                isOnTailPageMenu = false;
                isOnMainMenu = true;
                displayMainMenu(); // Return to the main menu
                break;
        }
        consecutive_press_count = 0;  // Reset the counter after handling the double press
    }

    sleep_ms(200);  // Debounce delay to prevent rapid repeated inputs
}

/* ------------------------------------------------------- */
/* Tail Menu Display */
/* ------------------------------------------------------- */

void displayTailPageMenu() {
    char* menuItems[] = {"Classic", "Next Gen", "Atmo", "Back"};
    int numberOfItems = sizeof(menuItems) / sizeof(char*);

    // Determine the start index for displaying menu items based on the current selection
    int startIndex = tailPageMenuSelection - 1;
    if (startIndex < 0) startIndex = 0; // Ensure the start index is not negative
    if (startIndex + 3 > numberOfItems) startIndex = numberOfItems - 3; // Adjust start index to show last 3 items

    ssd1306_clear(&disp); // Clear the display
    // Only iterate through the next three items or until the end of the list
    for (int i = startIndex; i < startIndex + 3 && i < numberOfItems; i++) {
        char lineBuffer[32];
        // Check if the current item is the selected one
        if (i == tailPageMenuSelection) {
            snprintf(lineBuffer, sizeof(lineBuffer), "* %s", menuItems[i]); // Mark current selection
        } else {
            snprintf(lineBuffer, sizeof(lineBuffer), "  %s", menuItems[i]); // Unmarked item
        }
        // Draw the string, adjusting position based on the loop iteration
        ssd1306_draw_string(&disp, 0, (i - startIndex) * 20, 2, lineBuffer);
    }
    ssd1306_show(&disp); // Update the display with the new content

    isOnTailPageMenu = true; // Indicate that we're now on the tail page menu
}



/* ------------------------------------------------------- */
/* Classic Tail page Display and Navigation */
/* ------------------------------------------------------- */

void displayClassicPage() {
    // Assuming 'currentIndexClassic' tracks the selected item on the Classic page
    displayClassic(currentIndexClassic);; // Display the currently selected "Classic" tail
}





void handleClassicPageNavigation() {
    if (!isOnClassPage) return;

    // Detect scroll up
    if (!gpio_get(5)) { // Assuming GPIO 5 is "scroll up"
        if (currentIndexClassic > 0) {
            currentIndexClassic--;
            displayClassic(currentIndexClassic);
        }
       
    }

    // Detect scroll down
    else if (!gpio_get(3)) { // Assuming GPIO 3 is "scroll down"
        if(!classicBack){
            currentIndexClassic++;
            displayClassic(currentIndexClassic);
        }
       
    }

    // Detect selection (this is where you transition to the confirmation page)
    else if (!gpio_get(4)) { // Assuming GPIO 4 is "select"
        // Check if a tail serial has been selected (assuming selectedSerial is a string)
        if (classicConfirmationNeeded) { // Check if the selectedSerial is not empty
            displayClassicConfirmationPage();
            classicConfirmationNeeded = false;
            // Transition out of the Atmosphere page
            // Further actions can be added here, such as setting flags for the confirmation page
        } else if(classicSelectedSerial){
                currentTailPage = TAIL_PAGE_CLASSIC;
                ssd1306_clear(&disp); // Clear the display
                ssd1306_draw_string(&disp, 0, 0, 2, "Tails:");
                //temp fix
                ssd1306_draw_string(&disp, 0, 20, 2, selectedWord);

                ssd1306_show(&disp);
            // If no serial selected, potentially handle manual input or other actions
            handleInputs(); // Assuming this is a manual input method for serials
            // Note: You may need to adjust the logic within handleInputs() to ensure it works in this context
        }else{
            backToTail();
        }
    }
    sleep_ms(200);
    static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
                if (currentIndexClassic > 0) {
            currentIndexClassic--;
            displayClassic(currentIndexClassic);
                }
                break;
            case 0xE9:  // Move down
                if(!classicBack){
            currentIndexClassic++;
            displayClassic(currentIndexClassic);
            }
                break;
            case 0xEE:  // Diagonal top left
            if (currentIndexClassic > 0) {
            currentIndexClassic--;
            displayClassic(currentIndexClassic);
            }
                break;
            case 0xEB:  // Diagonal bottom left
                if(!classicBack){
            currentIndexClassic++;
            displayClassic(currentIndexClassic);
                }
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
            if(!classicBack){
            currentIndexClassic++;
            displayClassic(currentIndexClassic);
        }
                break;
            case 0xE7:  // Slight Diagonal up move
                if (currentIndexClassic > 0) {
                    currentIndexClassic--;
                    displayClassic(currentIndexClassic);
                }
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }

}

/* ------------------------------------------------------- */
/* Next Gen page Display and Navigation */
/* ------------------------------------------------------- */

void displayNextGenPage() {
    // Assuming 'currentIndexNG' tracks the selected item on the Next Gen page
    displayNextGen(currentIndexNG); // Display the currently selected "Next Gen" tail
}

void handleNextGenPageNavigation() {
    if (!isOnNextGenPage) return;

    if (!gpio_get(5)) { // "Scroll up"
        if (currentIndexNG > 0) {
            currentIndexNG--;
            displayNextGenPage();
        }
    } else if (!gpio_get(3)) { // "Scroll down"
        if(!nextGenBack){
            currentIndexNG++;
            displayNextGenPage();
        }
    } else if (!gpio_get(4)) { // "Select" button pressed
        // Check if a tail serial has been selected (assuming selectedSerial is a string)
        if (nextGenConfirmationNeeded) { // Check if the selectedSerial is not empty
            displayNextGenConfirmationPage();
            nextGenConfirmationNeeded = false;
        } else if(nextGenSelectedSerial) {
                currentTailPage = TAIL_PAGE_NEXTGEN; 
                ssd1306_clear(&disp); // Clear the display
                ssd1306_draw_string(&disp, 0, 0, 2, "Tails:");
                //temp fix
                ssd1306_draw_string(&disp, 0, 20, 2, selectedWord);

                ssd1306_show(&disp);
            handleInputs(); // Assuming this is a manual input method for serials
            // Note: You may need to adjust the logic within handleInputs() to ensure it works in this context
        } else {
            backToTail();
        }
    }
    sleep_ms(200); // Debounce delay
        static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
                if (currentIndexNG > 0) {
            currentIndexNG--;
           displayNextGen(currentIndexNG);
                }
                break;
            case 0xE9:  // Move down
                if(!nextGenBack){
            currentIndexNG++;
            displayNextGen(currentIndexNG);
            }
                break;
            case 0xEE:  // Diagonal top left
            if (currentIndexNG > 0) {
            currentIndexNG--;
           displayNextGen(currentIndexNG);
            }
                break;
            case 0xEB:  // Diagonal bottom left
                if(!nextGenBack){
            currentIndexNG++;
           displayNextGen(currentIndexNG);
                }
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
            if(!nextGenBack){
            currentIndexNG++;
           displayNextGen(currentIndexNG);
        }
                break;
            case 0xE7:  // Slight Diagonal up move
                if (currentIndexNG > 0) {
                    currentIndexNG--;
                   displayNextGen(currentIndexNG);
                }
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }

}


/* ------------------------------------------------------- */
/* Atmoshpere page display and navigation */
/* ------------------------------------------------------- */

void displayAtmospherePage() {
    // Assuming 'currentIndexAtmosphere' tracks the selected item on the Atmosphere page
    displayAtmo(currentIndexAtmosphere); // Display the currently selected "Atmosphere" tail
}


void handleAtmospherePageNavigation() {
    if (!isOnAtmoPage) return;

    if (!gpio_get(5)) { // "Scroll up"
        if (currentIndexAtmosphere > 0) {
            currentIndexAtmosphere--;
            displayAtmospherePage();
        }
    } else if (!gpio_get(3)) { // "Scroll down"
        if (!atmoBack) {
            currentIndexAtmosphere++;
            displayAtmospherePage();
        }
    } else if (!gpio_get(4)) { // "Select" button pressed
        // Check if a tail serial has been selected (assuming selectedSerial is a string)
        if (atmoConfirmationNeeded) { // Check if the selectedSerial is not empty
            // Display the confirmation page for the selected tail
            printf("Selected tail for confirmation: %s\n", selectedTail); // Log the selected tail to the console
            displayAtmoConfirmationPage();
            atmoConfirmationNeeded = false;
            // Transition out of the Atmosphere page
             // No longer on Atmosphere page
        } else if (atmoSelectedSerial) {
            currentTailPage = TAIL_PAGE_ATMO;
            ssd1306_clear(&disp); // Clear the display
            ssd1306_draw_string(&disp, 0, 0, 2, "Tails:");
            //temp fix
            ssd1306_draw_string(&disp, 0, 20, 2, selectedWord);
            ssd1306_show(&disp);

            handleInputs(); // Assuming this is a manual input method for serials
            // Note: You may need to adjust the logic within handleInputs() to ensure it works in this context
        } else {
            backToTail();
        }
    }

    sleep_ms(200); // Debounce delay
            static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
        if (currentIndexAtmosphere > 0) {
            currentIndexAtmosphere--;
            displayAtmospherePage();
        }
                break;
            case 0xE9:  // Move down
        if (!atmoBack) {
            currentIndexAtmosphere++;
            displayAtmospherePage();
        }
                break;
            case 0xEE:  // Diagonal top left
        if (currentIndexAtmosphere > 0) {
            currentIndexAtmosphere--;
            displayAtmospherePage();
        }
                break;
            case 0xEB:  // Diagonal bottom left
        if (!atmoBack) {
            currentIndexAtmosphere++;
            displayAtmospherePage();
        }
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
        if (!atmoBack) {
            currentIndexAtmosphere++;
            displayAtmospherePage();
        }
                break;
            case 0xE7:  // Slight Diagonal up move
         if (currentIndexAtmosphere > 0) {
            currentIndexAtmosphere--;
            displayAtmospherePage();
        }
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }
}

void handleNextGenConfirmationInput() {
    if (ConfirmNGPage) {
        if (!gpio_get(5)) { // "Scroll up"
            confirmSelection = (confirmSelection + 1) % 2;
            displayNextGenConfirmationPage();
        } else if (!gpio_get(3)) { // "Scroll down"
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayNextGenConfirmationPage();
        } else if (!gpio_get(4)) { // "Select"
            if (confirmSelection == 0) {
                // Confirm action: FDR Download
                ReadDFDR(); // Perform download
                displayReset(); // Reset to the main menu after download
            } else {
                // Cancel action: Return to Next Gen Page
                resetAllPageFlags();
                isOnNextGenPage = true; // Return to Next Gen page
                ConfirmNGPage = false; // Exit confirmation page
                displayNextGenPage(); // Display Next Gen page again
            }
        }
        sleep_ms(200);
            static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
            confirmSelection = (confirmSelection + 1) % 2;
            displayNextGenConfirmationPage();
                break;
            case 0xE9:  // Move down
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayNextGenConfirmationPage();
                break;
            case 0xEE:  // Diagonal top left
            confirmSelection = (confirmSelection + 1) % 2;
            displayNextGenConfirmationPage();
                break;
            case 0xEB:  // Diagonal bottom left
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayNextGenConfirmationPage();
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayNextGenConfirmationPage();
                break;
            case 0xE7:  // Slight Diagonal up move
            confirmSelection = (confirmSelection + 1) % 2;
            displayNextGenConfirmationPage();
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }
    
    }
}

void handleAtmoConfirmationInput() {
    if (ConfirmAtmoPage) {
        if (!gpio_get(5)) { // "Scroll up"
            confirmSelection = (confirmSelection + 1) % 2;
            displayAtmoConfirmationPage();
        } else if (!gpio_get(3)) { // "Scroll down"
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayAtmoConfirmationPage();
        } else if (!gpio_get(4)) { // "Select"
            if (confirmSelection == 0) {
                // Confirm action: FDR Download
                ReadDFDR(); // Perform download
                displayReset(); // Reset to the main menu after download
            } else {
                // Cancel action: Ensure no other page flags are inadvertently set
                resetAllPageFlags(); // Reset all page flags before setting the correct one
                isOnAtmoPage = true; // Correctly return to Atmosphere page
                ConfirmAtmoPage = false;
                displayAtmospherePage();
            }

        }
        sleep_ms(200);
                    static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
            confirmSelection = (confirmSelection + 1) % 2;
            displayAtmoConfirmationPage();
                break;
            case 0xE9:  // Move down
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayAtmoConfirmationPage();
                break;
            case 0xEE:  // Diagonal top left
            confirmSelection = (confirmSelection + 1) % 2;
            displayAtmoConfirmationPage();
                break;
            case 0xEB:  // Diagonal bottom left
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayAtmoConfirmationPage();
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayAtmoConfirmationPage();
                break;
            case 0xE7:  // Slight Diagonal up move
            confirmSelection = (confirmSelection + 1) % 2;
            displayAtmoConfirmationPage();
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }
    
    }
}

void handleClassicConfirmationInput() {
    if (ConfirmClassicPage) {
        if (!gpio_get(5)) { // "Scroll up"
            confirmSelection = (confirmSelection + 1) % 2;
            displayClassicConfirmationPage();
        } else if (!gpio_get(3)) { // "Scroll down"
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayClassicConfirmationPage();
        } else if (!gpio_get(4)) { // "Select"
            if (confirmSelection == 0) {
                // Confirm action: FDR Download
                ReadDFDR(); // Perform download
                displayReset(); // Reset to the main menu after download
            } else {
                // Cancel action: Return to Next Gen Page
                resetAllPageFlags();
                isOnClassPage = true; // Return to Next Gen page
                ConfirmClassicPage = false; // Exit confirmation page
                displayClassicPage(); // Display Next Gen page again
            }
        }
        sleep_ms(200);
                            static int consecutive_press_count = 0;  // Static variable to count consecutive presses
    static uint8_t last_joystick_state = 0xFF;  // Assume 0xFF as the initial idle state
    uint8_t joystick_state = read_joystick_state(i2c0);
    printf("Joystick state: 0x%02X\n", joystick_state); // Debugging output

    last_joystick_state = joystick_state;  // Update the last known state

    // Execute actions based on joystick state
    if (joystick_state != 0xFF && joystick_state != 0xEF) {
        switch (joystick_state) {
            case 0xE6:  // Move up
            confirmSelection = (confirmSelection + 1) % 2;
            displayClassicConfirmationPage();
                break;
            case 0xE9:  // Move down
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayClassicConfirmationPage();
                break;
            case 0xEE:  // Diagonal top left
            confirmSelection = (confirmSelection + 1) % 2;
            displayClassicConfirmationPage();
                break;
            case 0xEB:  // Diagonal bottom left
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayClassicConfirmationPage();
                break;
            case 0xED:  // Diagonal bottom right (shared with slight down move)
            confirmSelection = (confirmSelection - 1 + 2) % 2;
            displayClassicConfirmationPage();
                break;
            case 0xE7:  // Slight Diagonal up move
            confirmSelection = (confirmSelection + 1) % 2;
            displayClassicConfirmationPage();
                break;
            default:
                printf("Other Joystick Movement\n");
                break;
        }
    }
    }
}

void displayReset() {
    // Reset flags and state to default values
    ConfirmAtmoPage = false;
    ConfirmNGPage = false;
    ConfirmClassicPage = false;
    isOnMainMenu = true; // Make sure to display the main menu
    isOnClassPage = false;
    isOnNextGenPage = false;
    isOnAtmoPage = false;
    isOnTailPage = false;
    atmoBack = false;
    classicBack = false;
    nextGenBack = false;
    // Reset additional flags as necessary

    displayMainMenu(); // Show the main menu
}



void resetAllPageFlags() {
    isOnMainMenu = false;
    isOnTailPageMenu = false;
    isOnClassPage = false;
    isOnNextGenPage = false;
    isOnAtmoPage = false;
    ConfirmAtmoPage = false;
    ConfirmClassicPage = false;
    ConfirmNGPage = false;
    // Add other page flags as necessary
}



void displayClassicConfirmationPage() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Choose");

    // Dynamically show the dot based on the current selection
    char confirmText[32] = "  Confirm";
    char cancelText[32] = "  Cancel";
    if (confirmSelection == 0) {
        snprintf(confirmText, sizeof(confirmText), "* Confirm");
    } else if (confirmSelection == 1) {
        snprintf(cancelText, sizeof(cancelText), "* Cancel");
    }

    ssd1306_draw_string(&disp, 0, 20, 2, confirmText);
    ssd1306_draw_string(&disp, 0, 40, 2, cancelText);
    ssd1306_show(&disp);

    // Set the state to be in the confirmation page
    ConfirmClassicPage = true;
    ConfirmAtmoPage = false;
    ConfirmNGPage = false;
    isNewClassic = false; // Reset isNewClassic as we're now moving to confirmation logic
    isOnClassPage = false;
    isOnAtmoPage= false;
}

void displayAtmoConfirmationPage() {
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Choose");

    // Dynamically show the dot based on the current selection
    char confirmText[32] = "  Confirm";
    char cancelText[32] = "  Cancel";
    if (confirmSelection == 0) {
        snprintf(confirmText, sizeof(confirmText), "* Confirm");
    } else if (confirmSelection == 1) {
        snprintf(cancelText, sizeof(cancelText), "* Cancel");
    }

    ssd1306_draw_string(&disp, 0, 20, 2, confirmText);
    ssd1306_draw_string(&disp, 0, 40, 2, cancelText);
    ssd1306_show(&disp);

    // Set the state to be in the confirmation page
    ConfirmClassicPage = false;
    ConfirmAtmoPage = true;
    ConfirmNGPage = false;
    isNewClassic = false; // Reset isNewClassic as we're now moving to confirmation logic
    isOnClassPage = false;
    isOnAtmoPage= false;
}

void displayNextGenConfirmationPage() {
    isOnNextGenPage = false;
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "Choose");

    // Dynamically show the dot based on the current selection
    char confirmText[32] = "  Confirm";
    char cancelText[32] = "  Cancel";
    if (confirmSelection == 0) {
        snprintf(confirmText, sizeof(confirmText), "* Confirm");
    } else if (confirmSelection == 1) {
        snprintf(cancelText, sizeof(cancelText), "* Cancel");
    }

    ssd1306_draw_string(&disp, 0, 20, 2, confirmText);
    ssd1306_draw_string(&disp, 0, 40, 2, cancelText);
    ssd1306_show(&disp);

    // Set the state to be in the confirmation page
    ConfirmClassicPage = false;
    ConfirmAtmoPage = false;
    ConfirmNGPage = true;
    isNewNextGen = false; // Reset isNewClassic as we're now moving to confirmation logic
    isOnClassPage = false;
    isOnAtmoPage= false;
}

void backToTail(){
    ConfirmAtmoPage = false;
    ConfirmNGPage = false;
    ConfirmClassicPage = false;
    isOnMainMenu = false; 
    isOnClassPage = false;
    isOnNextGenPage = false;
    isOnAtmoPage = false;
    isOnTailPageMenu = true;

    displayTailPageMenu();
}
//
//