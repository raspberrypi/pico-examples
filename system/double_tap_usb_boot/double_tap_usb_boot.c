#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"

// Allow user override of the LED mask
#ifndef USB_BOOT_LED_ACTIVITY_MASK
#define USB_BOOT_LED_ACTIVITY_MASK 0
#endif

// Doesn't make any sense for a RAM only binary
#if !PICO_NO_FLASH
static const uint32_t magic_token[] = {
        0xf01681de, 0xbd729b29, 0xd359be7a,
};

static uint32_t __uninitialized_ram(magic_location)[count_of(magic_token)];

// run at initialization time
static void __attribute__((constructor)) boot_double_tap_check() {
    for (uint i = 0; i < count_of(magic_token); i++) {
        if (magic_location[i] != magic_token[i]) {
            // Arm for 100 ms then disarm and continue booting
            for (i = 0; i < count_of(magic_token); i++) {
                magic_location[i] = magic_token[i];
            }
            busy_wait_us(100000);
            magic_location[0] = 0;
            return;
        }
    }

    magic_location[0] = 0;
    reset_usb_boot(USB_BOOT_LED_ACTIVITY_MASK, 0);
}

#endif