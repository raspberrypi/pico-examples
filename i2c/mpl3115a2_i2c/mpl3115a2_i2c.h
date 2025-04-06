// mpl3115a2.h
#ifndef _MPL3115A2_H
#define _MPL3115A2_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Existing structs
struct mpl3115a2_data_t {
    float temperature;
    float altitude;
};

void mpl3115a2_init(void);
void mpl3115a2_read_fifo(volatile uint8_t* fifo_buf);

// Add NEW prototypes
void mpl3115a2_set_sealevel_pressure(float hPa);
float mpl3115a2_get_sealevel_pressure(void);

#ifdef __cplusplus
}
#endif

#endif // _MPL3115A2_H