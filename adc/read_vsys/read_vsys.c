/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <power_status.h>
#include "hardware/adc.h"
#include "pico/float.h"

#if CYW43_USES_VSYS_PIN
#include "pico/cyw43_arch.h"
#endif

int main() {
    stdio_init_all();

    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Pico W uses a CYW43 pin to get VBUS so we need to initialise it
    #if CYW43_USES_VSYS_PIN
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    #endif

    bool old_battery_status;
    float old_voltage;
    bool battery_status = true;
    char *power_str = "UNKNOWN";

    while(true) {
        // Get battery status
        if (power_source(&battery_status) == PICO_OK) {
            power_str = battery_status ? "BATTERY" : "POWERED";
        }

        // Get voltage
        float voltage = 0;
        int voltage_return = power_voltage(&voltage);
        voltage = floorf(voltage * 100) / 100;

        // Display power if it's changed
        if (old_battery_status != battery_status || old_voltage != voltage) {
            char percent_buf[10] = {0};
            if (battery_status && voltage_return == PICO_OK) {
                const float min_battery_volts = 3.0f;
                const float max_battery_volts = 4.2f;
                int percent_val = ((voltage - min_battery_volts) / (max_battery_volts - min_battery_volts)) * 100;
                snprintf(percent_buf, sizeof(percent_buf), " (%d%%)", percent_val);
            }

            // Also get the temperature
            adc_select_input(4);
            const float conversionFactor = 3.3f / (1 << 12);
            float adc = (float)adc_read() * conversionFactor;
            float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

            // Display power and remember old vales
            printf("Power %s, %.2fV%s, temp %.1f DegC\n", power_str, voltage, percent_buf, tempC);
            old_battery_status = battery_status;
            old_voltage = voltage;
        }
        sleep_ms(1000);
    }

#if CYW43_USES_VSYS_PIN
    cyw43_arch_deinit();
#endif
    return 0;
}
