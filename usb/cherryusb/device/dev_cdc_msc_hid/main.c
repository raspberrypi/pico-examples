#include "pico/stdlib.h"
#include "pico/time.h"
#include "usbd_core.h"
#include <stdio.h>
#include <string.h>

/* include class test demo */
#include "demo/cdc_acm_hid_msc_template.c"

int main(void)
{
    stdio_init_all();
    printf("CherryUSB device cdc msc hid example\n");

    extern void cdc_acm_hid_msc_descriptor_init(uint8_t busid, uintptr_t reg_base);
    cdc_acm_hid_msc_descriptor_init(0, 0); // regbase is not used

    // Everything is interrupt driven so just loop here
    while (1) {
        extern void cdc_acm_data_send_with_dtr_test(uint8_t busid);
        cdc_acm_data_send_with_dtr_test(0);
        extern void hid_mouse_test(uint8_t busid);
        hid_mouse_test(0);
    }
    return 0;
}