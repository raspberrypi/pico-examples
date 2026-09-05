#include <stdbool.h>
/**
 * @brief initialize PWM pins and frequency clock
 * 
 * @param gpio PWM Pin
 * @param base_frequency set Servo Frequency - 50Hz is common
 */
void servo_init(int gpio, int base_frequency);


/**
 * @brief Map a range of values to another range
 * @return long 
 */
long map(long x, long in_min, long in_max, long out_min, long out_max);

/**
 * @brief Write an angle to the Servo
 * 
 * @param gpio PWM pin attached to the servo
 * @param angle Angle from 1-180
 * @param wait_write Wait for write to finish
 */
void servo_put(int gpio, int angle, bool wait_write);