/**
 * Copyright (c) 2021 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


// SDK types and declarations
//
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"    // for clock_get_hz()
#include "nec_transmit.h"

// import the assembled PIO state machine programs
#include "nec_carrier_burst.pio.h"
#include "nec_carrier_control.pio.h"

// Claim an unused state machine on the specified PIO and configure it
// to transmit NEC IR frames on the specificied GPIO pin.
//
// Returns: on success, the number of the carrier_control state machine
// otherwise -1
int nec_tx_init(PIO pio, uint pin_num) {

    // install the carrier_burst program in the PIO shared instruction space
    uint carrier_burst_offset;
    if (pio_can_add_program(pio, &nec_carrier_burst_program)) {
        carrier_burst_offset = pio_add_program(pio, &nec_carrier_burst_program);
    } else {
        return -1;
    }

    // claim an unused state machine on this PIO
    int carrier_burst_sm = pio_claim_unused_sm(pio, true);
    if (carrier_burst_sm == -1) {
        return -1;
    }

    // configure and enable the state machine
    nec_carrier_burst_program_init(pio,
                                   carrier_burst_sm,
                                   carrier_burst_offset,
                                   pin_num,
                                   38.222e3);                   // 38.222 kHz carrier

    // install the carrier_control program in the PIO shared instruction space
    uint carrier_control_offset;
    if (pio_can_add_program(pio, &nec_carrier_control_program)) {
        carrier_control_offset = pio_add_program(pio, &nec_carrier_control_program);
    } else {
        return -1;
    }

    // claim an unused state machine on this PIO
    int carrier_control_sm = pio_claim_unused_sm(pio, true);
    if (carrier_control_sm == -1) {
        return -1;
    }

    // configure and enable the state machine
    nec_carrier_control_program_init(pio,
                                     carrier_control_sm,
                                     carrier_control_offset,
                                     2 * (1 / 562.5e-6f),        // 2 ticks per 562.5us carrier burst
                                     32);                       // 32 bits per frame

    return carrier_control_sm;
}


// Create a frame in `NEC` format from the provided 8-bit address and data
//
// Returns: a 32-bit encoded frame
uint32_t nec_encode_frame(uint8_t address, uint8_t data) {
    // a normal 32-bit frame is encoded as address, inverted address, data, inverse data,
    return address | (address ^ 0xff) << 8 | data << 16 | (data ^ 0xff) << 24;
}
