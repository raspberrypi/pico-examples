/*
*	AM2302-Sensor.cpp
*
*	Author:  Frank Häfele
*	Date:    25.04.2024
*
*	Objective: AM2302-Sensor class
*
*  SPDX-License-Identifier: BSD-3-Clause
*/

#include "AM2302-Sensor.hpp"

/**
 * @brief Construct a new am2302::am2302 sensor::am2302 sensor object
 * 
 * @param pin Pin for AM2302 sensor
 */
AM2302::AM2302_Sensor::AM2302_Sensor(uint8_t pin) : _us_last_read{0}, _pin{pin}
{}

/**
 * @brief begin function setup pin and run sensor check.
 * 
 * @return true if sensor check is successful.
 * @return false if sensor check failed.
 */
bool AM2302::AM2302_Sensor::begin() {
   gpio_init(_pin);
   gpio_pull_up(_pin);
   // required delay() for a secure sensor check,
   // if you reset the mcu very fast one after another
   auto tic{time_us_64()};
   while ( time_us_64() - tic < READ_FREQUENCY * 1000U ) {
      sleep_ms(1U);
   }
   auto status{read()};
   _us_last_read = time_us_64();
   if (status == AM2302_READ_OK) {
      return true;
   }
   else {
      return false;
   }
}

/**
 * @brief read functionality
 * 
 * @return sensor status
*/
int8_t AM2302::AM2302_Sensor::read() {
   auto status{read_sensor()};
   
   if (status == AM2302_READ_OK) {
      // return status immediately
      return status;
   }
   else if (status == AM2302_ERROR_READ_FREQ) {
      return status;
   }
   else if (status == AM2302_ERROR_TIMEOUT) {
      resetData();
      return status;
   }
   else if (status == AM2302_ERROR_CHECKSUM) {
      // nothing to do
      return status;
   }
   return status;
}

/**
 * @brief initiate start sequence and read sensor data
 * 
 * @return sensor status
*/
int8_t AM2302::AM2302_Sensor::read_sensor() {
   // check read frequency
   if ( time_us_64() - _us_last_read < READ_FREQUENCY * 1000U) {
      return AM2302_ERROR_READ_FREQ;
   }
   _us_last_read = time_us_64();
   // *****************************
   //  === send start sequence ===
   // ****************************
   // start from HIGH ==> switch to LOW for min. of 1 ms
   // Set pin to Output
   gpio_set_dir(_pin, GPIO_OUT);
   // set Pin to LOW 
   gpio_put(_pin, 0);
   // wait min. 1,0 ms
   sleep_us(1200U);
   // Set port to HIGH ==> INPUT_PULLUP with PullUp
   gpio_put(_pin, 1);
   gpio_set_dir(_pin, GPIO_IN);
   // delay_us(30.0); not needed

   // ******************************
   //  === wait for Acknowledge ===
   // ******************************
   // Acknowledge Sequence 80us LOW 80 us HIGH
   // wait for LOW (80 µs)
   await_state(0);
   // wait for HIGH (80 µs)
   await_state(1);

   // *****************************
   //  ==== Read Sensor Data ====
   // *****************************
   // ==> START of time critical code <==
   // read 40 bits from sensor into the buffer:
   // ==> HIGH state is 70 µs
   // ==> LOW state is 28 µs
   uint8_t _data[5U] = {0};
   if (read_sensor_data(_data, 5U) == AM2302_ERROR_TIMEOUT) {
      return AM2302_ERROR_TIMEOUT;
   }
   // ==> END of time critical code <==
   
   // check checksum
   _checksum_ok = (_data[4] == ( (_data[0] + _data[1] + _data[2] + _data[3]) & 0xFF) );

   /*
   // Code part to check the checksum
   // Due to older sensors have an bug an deliver wrong data
   auto d4 = _data[4];
   auto cs = ( (_data[0] + _data[1] + _data[2] + _data[3]) & 0xFF) ;
   Serial.print("delivered Checksum:  ");
   AM2302_Tools::print_byte_as_bit(d4);
   Serial.print("calculated Checksum: ");
   AM2302_Tools::print_byte_as_bit(cs);
   */

   if (_checksum_ok) {
      _hum  = static_cast<uint16_t>((_data[0] << 8) | _data[1]);
      if (_data[2] & 0x80) {
         // negative temperature detected
         _data[2] &= 0x7f;
         _temp = -static_cast<int16_t>((_data[2] << 8) | _data[3]);
      }
      else {
         _temp = static_cast<int16_t>((_data[2] << 8) | _data[3]);
      }
      return AM2302_READ_OK;
   }
   else {
      return AM2302_ERROR_CHECKSUM;
   }
}

/**
 * @brief wait for a specific pin state
 * 
 * @param state state to wait for
 * @return int8_t status
 */
int8_t AM2302::AM2302_Sensor::await_state(uint8_t state) {
   uint8_t wait_counter{0}, state_counter{0};
   // count wait for state time
   while ( (gpio_get(_pin) != state) ) {
      ++wait_counter;
      sleep_us(1U);
      if (wait_counter >= READ_TIMEOUT) {
         return AM2302_ERROR_TIMEOUT;
      }
   }
   // count state time
   while ( (gpio_get(_pin) == state) ) {
      ++state_counter;
      sleep_us(1U);
      if (state_counter >= READ_TIMEOUT) {
         return AM2302_ERROR_TIMEOUT;
      }
   }
   return (state_counter > wait_counter);
}

/**
 * @brief read sensor data
 * 
 * @param buffer data buffer of 40 bit
 * @param size of buffer => 40
 * @return int8_t 
 */
int8_t AM2302::AM2302_Sensor::read_sensor_data(uint8_t *buffer, uint8_t size) {
   for (uint8_t i = 0; i < size; ++i) {
      for (uint8_t bit = 0; bit < 8U; ++bit) {
         uint8_t wait_counter{0}, state_counter{0};
         // count wait for state time
         while ( !gpio_get(_pin) ) {
            ++wait_counter;
            sleep_us(1U);
            if (wait_counter >= READ_TIMEOUT) {
               return AM2302_ERROR_TIMEOUT;
            }
         }
         // count state time
         while ( gpio_get(_pin) ) {
            ++state_counter;
            sleep_us(1U);
            if (state_counter >= READ_TIMEOUT) {
               return AM2302_ERROR_TIMEOUT;
            }
         }
         buffer[i] <<= 1;
         buffer[i] |= (state_counter > wait_counter);
      }
   }
   return AM2302_READ_OK;
}

/**
 * @brief get Sensor State in human readable manner
 * 
 * @return sensor state
*/
const char * AM2302::AM2302_Sensor::get_sensorState(int8_t state) const {
   if(state == AM2302_READ_OK) {
      return AM2302_STATE_OK;
   }
   else if(state == AM2302_ERROR_CHECKSUM) {
      return AM2302_STATE_ERR_CKSUM;
   }
   else if(state == AM2302_ERROR_TIMEOUT) {
      return AM2302_STATE_ERR_TIMEOUT;
   }
   else if(state == AM2302_ERROR_READ_FREQ) {
      return AM2302_STATE_ERR_READ_FREQ;
   }
}

/**
 * @brief reset temperature and humidity data
 * 
 */
void AM2302::AM2302_Sensor::resetData() {
   // reset tem to -255 and hum to 0 as indication
   _temp = -2550;
   _hum = 0;
}

/**
 * @brief helper function to print byte as bit
 * 
 * @param value byte with 8 bits
 */
void AM2302_Tools::print_byte_as_bit(char value) {
   for (int i = 7; i >= 0; --i) {
      char c = (value & (1 << i)) ? '1' : '0';
      printf("%c", c);
   }
   printf("\n");
}