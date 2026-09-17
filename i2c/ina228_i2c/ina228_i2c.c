#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>

// --- Shunt resistor selection ----------------------------------------------
// The shunt converts current into the small voltage the INA228 measures.
// A bigger shunt gives better resolution at low current but drops more voltage
// (and clips sooner) at high current. Pick one to match what you're measuring.
//   SHUNT_BOARD_15M : 15 mOhm, the breakout's stock part. Good for ~100s of mA.
//   SHUNT_SLEEP_1R  : 1 Ohm precision part. Good for sleep currents (uA..few mA).
#define SHUNT_BOARD_15M  0
#define SHUNT_SLEEP_1R   1

#define SHUNT_SELECT  SHUNT_BOARD_15M

#if SHUNT_SELECT == SHUNT_SLEEP_1R
  #define R_SHUNT 1.0
  // Optimised for sleep measurement (<5 mA). The DUT's ~150 mA active draw will
  // clip with this calibration -- intended; this build targets sleep only.
  #define MAX_EXPECTED_CURRENT 0.005
#else // SHUNT_BOARD_15M
  #define R_SHUNT 0.015
  #define MAX_EXPECTED_CURRENT 0.2
#endif

#define I2C_ADDR 0x40

// Shunt-voltage ADC input range:
//   0 = +/-163.84 mV full-scale
//   1 = +/-40.96 mV full-scale  (~4x finer resolution, for small shunt voltages)
// Defaults track the shunt: the stock 15 mOhm part may carry 100s of mA, so the
// wider range avoids clipping; the 1 Ohm sleep part sees only mV, so the narrow
// range gives much better resolution. Changing this updates VSHUNT_FACTOR and
// SHUNT_CAL below, which MUST stay consistent with each other.
#if SHUNT_SELECT == SHUNT_SLEEP_1R
  #define ADCRANGE 1
#else // SHUNT_BOARD_15M
  #define ADCRANGE 0
#endif

// --- Averaging --------------------------------------------------------------
// The INA228 can average multiple conversions in hardware before updating the
// registers (ADC_CONFIG AVG field, bits 2:0). More averaging = quieter readings
// but slower updates.
//
// Default tracks the shunt/use case:
//   Sleep build  -> heavy averaging: steady low currents benefit from a quiet
//                   reading, and updates can afford to be slow.
//   Stock build  -> light averaging: a long averaging window smears abrupt
//                   load changes (e.g. a powman sleep/wake step) across the
//                   window and produces garbage on the transition, so keep it
//                   short to track changes and stay responsive.
// Allowed values: 1, 4, 16, 64, 128, 256, 512, 1024 (samples averaged).
#ifndef AVG_SAMPLES
#if SHUNT_SELECT == SHUNT_SLEEP_1R
  #define AVG_SAMPLES 128
#else // SHUNT_BOARD_15M
  #define AVG_SAMPLES 16
#endif
#endif

#if   AVG_SAMPLES == 1
  #define AVG_FIELD 0
#elif AVG_SAMPLES == 4
  #define AVG_FIELD 1
#elif AVG_SAMPLES == 16
  #define AVG_FIELD 2
#elif AVG_SAMPLES == 64
  #define AVG_FIELD 3
#elif AVG_SAMPLES == 128
  #define AVG_FIELD 4
#elif AVG_SAMPLES == 256
  #define AVG_FIELD 5
#elif AVG_SAMPLES == 512
  #define AVG_FIELD 6
#elif AVG_SAMPLES == 1024
  #define AVG_FIELD 7
#else
  #error "AVG_SAMPLES must be one of: 1, 4, 16, 64, 128, 256, 512, 1024"
#endif

// --- Conversion time --------------------------------------------------------
// Time the ADC spends on each individual conversion (before averaging), set
// for bus, shunt and temperature alike here. Longer = quieter samples but
// slower. Total time per reading is roughly CONV_TIME x AVG_SAMPLES, so raising
// both together can make updates very slow -- watch the combined figure.
// Allowed values in microseconds: 50, 84, 150, 280, 540, 1052, 2074, 4120.
#define CONV_TIME_US 1052

#if   CONV_TIME_US == 50
  #define CT_FIELD 0
#elif CONV_TIME_US == 84
  #define CT_FIELD 1
#elif CONV_TIME_US == 150
  #define CT_FIELD 2
#elif CONV_TIME_US == 280
  #define CT_FIELD 3
#elif CONV_TIME_US == 540
  #define CT_FIELD 4
#elif CONV_TIME_US == 1052
  #define CT_FIELD 5
#elif CONV_TIME_US == 2074
  #define CT_FIELD 6
#elif CONV_TIME_US == 4120
  #define CT_FIELD 7
#else
  #error "CONV_TIME_US must be one of: 50, 84, 150, 280, 540, 1052, 2074, 4120"
#endif

// ADC_CONFIG register value:
//   bits 15:12 MODE   = 0xF (continuous bus, shunt and temperature)
//   bits 11:9  VBUSCT = conversion time for bus voltage
//   bits 8:6   VSHCT  = conversion time for shunt voltage
//   bits 5:3   VTCT   = conversion time for temperature
//   bits 2:0   AVG    = samples averaged
#define ADC_CONFIG_VALUE ( (0xFu << 12)            \
                         | ((CT_FIELD & 0x7) << 9) \
                         | ((CT_FIELD & 0x7) << 6) \
                         | ((CT_FIELD & 0x7) << 3) \
                         | (AVG_FIELD & 0x7) )

// ina228 registers (see datasheet)
#define CONFIG_REG    0x00
#define ADC_CONFIG_REG 0x01
#define SHUNT_CAL_REG  0x02
#define VSHUNT_REG    0x04
#define VBUS_REG      0x05
#define DIETEMP_REG   0x06
#define CURRENT_REG   0x07
#define POWER_REG     0x08
#define ENERGY_REG    0x09
#define CHARGE_REG    0x0A

// Conversion factors (see datasheet)
// VSHUNT_FACTOR: volts per LSB. 312.5 nV at ADCRANGE=0, 4x smaller at ADCRANGE=1.
#if ADCRANGE == 1
const double VSHUNT_FACTOR  = 78.125 * 1e-9;      // V per LSB (ADCRANGE = 1)
#else
const double VSHUNT_FACTOR  = 312.5 * 1e-9;       // V per LSB (ADCRANGE = 0)
#endif
const double VBUS_FACTOR    = 195.3125 * 1e-6;    // V per LSB
const double DIETEMP_FACTOR = 7.8125 * 1e-3;      // degC per LSB
const double CURRENT_FACTOR = MAX_EXPECTED_CURRENT / (double)(1u << 19); // A per LSB
const double POWER_FACTOR   = 3.2 * (MAX_EXPECTED_CURRENT / (double)(1u << 19));        // W per LSB
const double ENERGY_FACTOR  = 16.0 * 3.2 * (MAX_EXPECTED_CURRENT / (double)(1u << 19)); // J per LSB
const double CHARGE_FACTOR  = MAX_EXPECTED_CURRENT / (double)(1u << 19);                // C per LSB

// SHUNT_CAL = 13107.2e6 * CURRENT_LSB * R_SHUNT, then multiplied by 4 if ADCRANGE = 1.
#if ADCRANGE == 1
const uint16_t SHUNT_CAL = (uint16_t)(4.0 * 13107.2e6 * (MAX_EXPECTED_CURRENT / (double)(1u << 19)) * R_SHUNT);
#else
const uint16_t SHUNT_CAL = (uint16_t)(13107.2e6 * (MAX_EXPECTED_CURRENT / (double)(1u << 19)) * R_SHUNT);
#endif

float vshunt, vbus, dietemp, current, power, energy, charge;

// --- Byte-combining helpers --------------------------------------------------
// Type-safe replacements for the old COALESCE macros. Each returns an unsigned
// value; signedness is handled separately by the sign-extension helpers below.

// 16-bit register (e.g. DIETEMP)
static inline uint16_t bytes_to_u16(const uint8_t *b) {
    return ((uint16_t)b[0] << 8) | b[1];
}

// 24-bit register holding a 20-bit value in the upper bits; lower 4 are reserved
// and read as zero (VSHUNT, VBUS, CURRENT).
static inline uint32_t bytes_to_u20(const uint8_t *b) {
    return ((uint32_t)b[0] << 12) | ((uint32_t)b[1] << 4) | ((uint32_t)b[2] >> 4);
}

// 24-bit register, all bits significant (POWER).
static inline uint32_t bytes_to_u24(const uint8_t *b) {
    return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
}

// 40-bit accumulation register (ENERGY, CHARGE).
static inline uint64_t bytes_to_u40(const uint8_t *b) {
    return ((uint64_t)b[0] << 32) | ((uint64_t)b[1] << 24) |
           ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 8)  | (uint64_t)b[4];
}

// --- Sign-extension helpers --------------------------------------------------
// VSHUNT, CURRENT (20-bit) and CHARGE (40-bit) are two's complement.

static inline int32_t sign_extend_20(uint32_t v) {
    return (v & 0x80000u) ? (int32_t)(v | 0xFFF00000u) : (int32_t)v;
}

static inline int16_t sign_extend_16(uint16_t v) {
    return (int16_t)v; // already the right width
}

static inline int64_t sign_extend_40(uint64_t v) {
    return (v & ((uint64_t)1 << 39)) ? (int64_t)(v | 0xFFFFFF0000000000ull) : (int64_t)v;
}

// --- Register access ---------------------------------------------------------

static void ina228_read_reg(uint8_t reg, uint8_t *buf, size_t len) {
    i2c_write_blocking(i2c_default, I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buf, len, false);
}

static void ina228_write_reg16(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    i2c_write_blocking(i2c_default, I2C_ADDR, buf, 3, false);
}

static void ina228_init(void) {
    // Set the shunt-voltage ADC range (CONFIG register bit 4 = ADCRANGE).
#if ADCRANGE == 1
    ina228_write_reg16(CONFIG_REG, 0x0010);
#else
    ina228_write_reg16(CONFIG_REG, 0x0000);
#endif

    // Program the shunt calibration so CURRENT/POWER read correctly.
    ina228_write_reg16(SHUNT_CAL_REG, SHUNT_CAL);

    // ADC config: continuous mode + averaging (see ADC_CONFIG_VALUE above).
    // Continuous mode is needed for the ENERGY/CHARGE accumulators.
    ina228_write_reg16(ADC_CONFIG_REG, ADC_CONFIG_VALUE);
}

static void ina228_read(float *vshunt, float *vbus, float *dietemp,
                        float *current, float *power, float *energy, float *charge) {
    uint8_t buf[5];

    ina228_read_reg(VSHUNT_REG, buf, 3);
    *vshunt = sign_extend_20(bytes_to_u20(buf)) * VSHUNT_FACTOR;

    ina228_read_reg(VBUS_REG, buf, 3);
    *vbus = bytes_to_u20(buf) * VBUS_FACTOR; // unsigned

    ina228_read_reg(DIETEMP_REG, buf, 2);
    *dietemp = sign_extend_16(bytes_to_u16(buf)) * DIETEMP_FACTOR;

    ina228_read_reg(CURRENT_REG, buf, 3);
    *current = sign_extend_20(bytes_to_u20(buf)) * CURRENT_FACTOR;

    ina228_read_reg(POWER_REG, buf, 3);
    *power = bytes_to_u24(buf) * POWER_FACTOR; // unsigned

    ina228_read_reg(ENERGY_REG, buf, 5);
    *energy = bytes_to_u40(buf) * ENERGY_FACTOR; // unsigned

    ina228_read_reg(CHARGE_REG, buf, 5);
    *charge = sign_extend_40(bytes_to_u40(buf)) * CHARGE_FACTOR; // signed
}

// Cross-check the reading for self-consistency. The INA228 derives POWER
// internally from its own current and bus-voltage measurement, so for a valid
// sample the separately-read CURRENT and VBUS should reconstruct POWER:
//     P ~= Vbus * I
// When the current register rails (e.g. the shunt voltage briefly leaves range
// during an abrupt powman sleep/wake step), CURRENT and POWER stop agreeing.
// That contradiction is a reliable "this sample is bogus" flag.
// Returns true if the sample looks trustworthy.
static bool ina228_reading_valid(float vbus, float current, float power) {
    float expected_power = vbus * current;        // Watts
    float diff = fabsf(expected_power - power);
    // Allow a fixed floor (covers near-zero noise) plus a generous 25% of the
    // larger magnitude (covers timing skew between the register reads).
    float tol = 1e-3f + 0.25f * fmaxf(fabsf(expected_power), fabsf(power));
    return diff <= tol;
}

int main() {
    stdio_init_all();

    // I2C initialisation
    i2c_init(i2c_default, 400 * 1000);

    // GPIO initialisation
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Initialise ina228
    ina228_init();

    while (true) {
        ina228_read(&vshunt, &vbus, &dietemp, &current, &power, &energy, &charge);
#if 0
        printf("INA228 Measurements:\nVSHUNT: %f V\nVBUS: %f V\nDIETEMP: %f degC\n"
               "CURRENT: %f A\nPOWER: %f W\nENERGY: %f J\nCHARGE: %f C\n-----------------\n",
               vshunt, vbus, dietemp, current, power, energy, charge);
#else
        if (!ina228_reading_valid(vbus, current, power)) {
            // Current and power disagree -- the sample railed (often on an
            // abrupt powman transition, or because the current is below what
            // this shunt can resolve). Don't print a misleading number.
            printf("current: invalid  voltage: %.3f V  power: invalid\n", vbus);
        } else
#if SHUNT_SELECT == SHUNT_SLEEP_1R
        // Sleep build: currents are small, print in microamps.
        printf("current: %.1f uA  voltage: %.3f V  power: %.3f mW\n",
               current * 1e6, vbus, power * 1000);
#else
        // Stock build: print in milliamps.
        printf("current: %.2f mA  voltage: %.3f V  power: %.2f mW\n",
               current * 1e3, vbus, power * 1000);
#endif
#endif
        sleep_ms(1000);
    }
}