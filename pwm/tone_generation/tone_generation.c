/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pwm.h"

/**  Example code to generate tones with Buzzer using PWM with pico-sdk.
 *
 *   Every sound humans encounter consists of one or more frequencies, and the
 *   way the ear interprets those sounds  is called pitch.
 *
 *   In order to produce a variety of pitches, a digital signal needs to convey
 *   the frequency of sound it is trying to reproduce. The simplest approach is
 *   to generate a 50% duty cycle pulse stream and set the frequency to the
 *   desired pitch.
 *
 *   References: 
 *       - https://www.hackster.io/106958/pwm-sound-synthesis-9596f0#overview
 */
#define BUZZER_PIN 3

// Notes' frequencies used for the melody in Hz.
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698

#define SPEED  400U  // Speed of each note in ms
#define SILENCE 10U  // 10 ms of silence after each note

uint slice_num;  // Variable to save the PWM slice number

void play_tone(uint gpio, uint16_t freq, float duration);

int main() {
    // Configure BUZZER_PIN as PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    // Update slice_num with the slice number of BUZZER_PIN
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    // Get default configuration for PWM slice and initialize pwm
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    while (1) {
        /**
         *  .:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.
         *  .                                                     .
         *  .        *        Song: Noche de Paz (Silent Night)   .
         *  .       /.\       Instrument: Buzzer                  .
         *  .      /..'\      Interpreter: Raspberry Pi Pico      .
         *  .      /'.'\                                 _        .
         *  .     /.''.'\                              _[ ]_      .
         *  .     /.'.'.\         Merry Christmas!      (")       .
         *  .    /'.''.'.\          Feliz Navidad!  `__( : )--'   .
         *  .    ^^^[_]^^^                            (  :  )     .
         *  .                                       ""`-...-'""   .  
         *  .                                                     .
         *  .:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.
         */
        // 1
        play_tone(BUZZER_PIN, NOTE_G4, 1.5f);
        play_tone(BUZZER_PIN, NOTE_A4, .5f);
        play_tone(BUZZER_PIN, NOTE_G4, 1.f);

        // 2
        play_tone(BUZZER_PIN, NOTE_E4, 3.f);

        // 3
        play_tone(BUZZER_PIN, NOTE_G4, 1.5f);
        play_tone(BUZZER_PIN, NOTE_A4, .5f);
        play_tone(BUZZER_PIN, NOTE_G4, 1.f);

        // 4
        play_tone(BUZZER_PIN, NOTE_E4, 3.f);

        // 5
        play_tone(BUZZER_PIN, NOTE_D5, 2.f);      
        play_tone(BUZZER_PIN, NOTE_D5, 1.f);      

        // 6
        play_tone(BUZZER_PIN, NOTE_B4, 3.f);      

        // 7
        play_tone(BUZZER_PIN, NOTE_C5, 2.f);
        play_tone(BUZZER_PIN, NOTE_C5, 1.f);

        // 8
        play_tone(BUZZER_PIN, NOTE_G4, 3.f);

        // 9
        play_tone(BUZZER_PIN, NOTE_A4, 2.f);
        play_tone(BUZZER_PIN, NOTE_A4, 1.f);

        // 10 
        play_tone(BUZZER_PIN, NOTE_C5, 1.5f);
        play_tone(BUZZER_PIN, NOTE_B4, .5f);
        play_tone(BUZZER_PIN, NOTE_A4, 1.f);

        // 11 
        play_tone(BUZZER_PIN, NOTE_G4, 1.5f);
        play_tone(BUZZER_PIN, NOTE_A4, .5f);
        play_tone(BUZZER_PIN, NOTE_G4, 1.f);

        // 12 
        play_tone(BUZZER_PIN, NOTE_E4, 3.f);

        // 13
        play_tone(BUZZER_PIN, NOTE_A4, 2.f);
        play_tone(BUZZER_PIN, NOTE_A4, 1.f);

        // 14
        play_tone(BUZZER_PIN, NOTE_C5, 1.5f);
        play_tone(BUZZER_PIN, NOTE_B4, .5f);
        play_tone(BUZZER_PIN, NOTE_A4, 1.f);

        // 15
        play_tone(BUZZER_PIN, NOTE_G4, 1.5f);
        play_tone(BUZZER_PIN, NOTE_A4, .5f);
        play_tone(BUZZER_PIN, NOTE_G4, 1.f);

        // 16
        play_tone(BUZZER_PIN, NOTE_E4, 3.f);

        // 17
        play_tone(BUZZER_PIN, NOTE_D5, 2.f);
        play_tone(BUZZER_PIN, NOTE_D5, 1.f);

        // 18
        play_tone(BUZZER_PIN, NOTE_F5, 1.5f);
        play_tone(BUZZER_PIN, NOTE_D5, .5f);
        play_tone(BUZZER_PIN, NOTE_B4, 1.f);

        // 19
        play_tone(BUZZER_PIN, NOTE_C5, 3.f);

        // 20
        play_tone(BUZZER_PIN, NOTE_E5, 3.f);

        // 21
        play_tone(BUZZER_PIN, NOTE_C5, 1.5f);
        play_tone(BUZZER_PIN, NOTE_G4, .5f);
        play_tone(BUZZER_PIN, NOTE_E4, 1.f);

        // 22
        play_tone(BUZZER_PIN, NOTE_G4, 1.5f);
        play_tone(BUZZER_PIN, NOTE_F4, .5f);
        play_tone(BUZZER_PIN, NOTE_D4, 1.f);

        // 23 & 24
        play_tone(BUZZER_PIN, NOTE_C4, 5.f);
        // play_tone(BUZZER_PIN, <Silencio>, 2.f);
        // Fin
  }
}

void play_tone(uint gpio, uint16_t freq, float duration) {
    // Calculate and configure new clock divider according to the frequency
    float clkdiv = (1.f / freq) * 2000.f;
    pwm_set_clkdiv(slice_num, clkdiv);
    // Configure duty to 50% ((2**16-1)/2)
    pwm_set_gpio_level(BUZZER_PIN, 32768U);
    sleep_ms((uint32_t) SPEED * duration);
    // Make silence after each note to distinguish them better.
    pwm_set_gpio_level(BUZZER_PIN, 0);
    sleep_ms(SILENCE);
}
