/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "stdbool.h"
#include "hardware/adc.h"
#include "power_status.h"

#if CYW43_USES_VSYS_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef PICO_POWER_SAMPLE_COUNT
#define PICO_POWER_SAMPLE_COUNT 3
#endif

// Pin used for ADC 0
#define PICO_FIRST_ADC_PIN 26

int power_source(bool *battery_powered) {
#if defined CYW43_WL_GPIO_VBUS_PIN
    *battery_powered = !cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    return PICO_OK;
#elif defined PICO_VBUS_PIN
    gpio_set_function(PICO_VBUS_PIN, GPIO_FUNC_SIO);
    *battery_powered = !gpio_get(PICO_VBUS_PIN);
    return PICO_OK;
#else
    return PICO_ERROR_NO_DATA;
#endif
}

int power_voltage(float *voltage_result) {
#ifndef PICO_VSYS_PIN
    return PICO_ERROR_NO_DATA;
#else
#if CYW43_USES_VSYS_PIN
    cyw43_thread_enter();
    // Make sure cyw43 is awake
    cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
#endif

    // setup adc
    adc_gpio_init(PICO_VSYS_PIN);
    adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
 
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);

    // We seem to read low values initially - this seems to fix it
    int ignore_count = PICO_POWER_SAMPLE_COUNT;
    while (!adc_fifo_is_empty() || ignore_count-- > 0) {
        (void)adc_fifo_get_blocking();
    }

    // read vsys
    uint32_t vsys = 0;
    for(int i = 0; i < PICO_POWER_SAMPLE_COUNT; i++) {
        uint16_t val = adc_fifo_get_blocking();
        vsys += val;
    }

    adc_run(false);
    adc_fifo_drain();

    vsys /= PICO_POWER_SAMPLE_COUNT;
#if CYW43_USES_VSYS_PIN
    cyw43_thread_exit();
#endif
    // Generate voltage
    const float conversion_factor = 3.3f / (1 << 12);
    *voltage_result = vsys * 3 * conversion_factor;
    return PICO_OK;
#endif
}