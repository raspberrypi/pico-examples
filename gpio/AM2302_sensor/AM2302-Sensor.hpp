/*
*	AM2302-Sensor.hpp
*
*	Author:  Frank Häfele
*	Date:    25.04.2024
*
*	Objective: AM2302-Sensor class
*
*  SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef __AM2302_SENSOR_H__
#define __AM2302_SENSOR_H__
#include <cstdint>
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

namespace AM2302 {

   constexpr const char * AM2302_STATE_OK{"OK"};
   constexpr const char * AM2302_STATE_ERR_CKSUM{"Error: Checksum"};
   constexpr const char * AM2302_STATE_ERR_TIMEOUT{"Error: Timeout"};
   constexpr const char * AM2302_STATE_ERR_READ_FREQ{"Error: Read Frequency"};

   constexpr int8_t AM2302_READ_OK          {0};
   constexpr int8_t AM2302_ERROR_CHECKSUM   {-1};
   constexpr int8_t AM2302_ERROR_TIMEOUT    {-2};
   constexpr int8_t AM2302_ERROR_READ_FREQ  {-3};

   // define timeout in 100 µs
   constexpr uint8_t READ_TIMEOUT         {100U};
   // read cycle max every 2 s
   constexpr uint16_t READ_FREQUENCY      {2000U};

   class AM2302_Sensor {

      public:
         explicit AM2302_Sensor(uint8_t pin);
         bool begin();
         int8_t read();
         float get_Temperature() const {return _temp * 0.1F;}
         float get_Humidity() const {return _hum * 0.1F;}
         const char * get_sensorState(int8_t state) const;

      private:
         unsigned long _us_last_read;
         uint16_t _hum {0};
         int16_t _temp {0};
         uint8_t _pin;
         bool _checksum_ok {false};
         int8_t await_state(uint8_t state);
         int8_t read_sensor();
         int8_t read_sensor_data(uint8_t *buffer, uint8_t const size);
         void resetData();
   };
}

namespace AM2302_Tools {
    void print_byte_as_bit(char value);
}

#endif
