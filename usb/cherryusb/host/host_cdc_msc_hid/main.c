#include "pico/stdlib.h"
#include "pico/time.h"
#include "usbh_core.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

/* include class test demo */
#include "demo/usb_host.c"

int main(void)
{
    stdio_init_all();
    printf("CherryUSB host cdc msc hid example\n");

    usbh_initialize(0, 0); // regbase is not used

    vTaskStartScheduler();
    // Everything is interrupt driven so just loop here
    while (1) {
    }
    return 0;
}