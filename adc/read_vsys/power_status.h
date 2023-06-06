/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POWER_STATUS_H
#define POWER_STATUS_H

/*!
 * \brief Get power source
 *
 * Returns whether battery powered
 * \note On Pico W must have called cyw43_arch_init
 *
 * \param battery_powered True if powered by battery, False if powered by USB or another means
 * \return Zero if the battery status can be determined, an error code otherwise \see pico_error_codes
 */

int power_source(bool *battery_powered);

/*!
 * \brief Get system voltage
 *
 * Returns the system voltage
 * \note Must have called adc_init
 *
 * \param voltage Calculated voltage
 * \return Zero if the voltage can be determined, an error code otherwise \see pico_error_codes
 */
int power_voltage(float *voltage);

#endif