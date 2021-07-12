#include "pico/stdlib.h"
#include "servo.h"
#include <stdbool.h>

int main()
{
    for(;;)
    {
        servo_init(1, 50);
        servo_put(1, 0, true);

        sleep_ms(1000);

        servo_init(1, 50);
        servo_put(1, 90, true);

        sleep_ms(1000);
    }
}