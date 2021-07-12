#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "servo.h"
#include <stdbool.h>

#define ANGLE 90


int main()
{
    servo_init(1, 50);
    servo_put(1, ANGLE, true);

    reset_usb_boot(0,0); // reset to BOOTSEL for another flash
}