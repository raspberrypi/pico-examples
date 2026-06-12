#include "pico/stdio.h"

extern void run_ble_pointer(void);

int main(void) {
    stdio_init_all();
    run_ble_pointer();
    return 0;
}
