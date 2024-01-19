/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/tone.h"

/**  Example code to generate tones with Buzzer using pico-sdk.
 *
 *   The melody is the Christmas song "Noche de Paz" (Silent Night).
 *
 *   .:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.
 *   .                                                     .
 *   .        *        Song: Noche de Paz (Silent Night)   .
 *   .       /.\       Instrument: Buzzer                  .
 *   .      /..'\      Interpreter: Raspberry Pi Pico      .
 *   .      /'.'\                                 _        .
 *   .     /.''.'\                              _[ ]_      .
 *   .     /.'.'.\         Merry Christmas!      (")       .
 *   .    /'.''.'.\          Feliz Navidad!  `__( : )--'   .
 *   .    ^^^[_]^^^                            (  :  )     .
 *   .                                       ""`-...-'""   .  
 *   .                                                     .
 *   .:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.
 *       
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

#define NOTE_DURATION  400U  // Speed of each note in ms

uint32_t get_note_duration_in_ms(float note_duration) {
    return (uint32_t)(note_duration * NOTE_DURATION);
}

int main() {
    // Configure tone generation on BUZZER_PIN
    tone_init(BUZZER_PIN);
    while (1) {
        // 1
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.f));

        // 2
        tone(BUZZER_PIN, NOTE_E4, get_note_duration_in_ms(3.f));

        // 3
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.f));

        // 4
        tone(BUZZER_PIN, NOTE_E4, get_note_duration_in_ms(3.f));

        // 5
        tone(BUZZER_PIN, NOTE_D5, get_note_duration_in_ms(2.f));
        tone(BUZZER_PIN, NOTE_D5, get_note_duration_in_ms(1.f));

        // 6
        tone(BUZZER_PIN, NOTE_B4, get_note_duration_in_ms(3.f));

        // 7
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(2.f));
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(1.f));

        // 8
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(3.f));

        // 9
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(2.f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(1.f));

        // 10 
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_B4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(1.f));

        // 11 
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.f));

        // 12 
        tone(BUZZER_PIN, NOTE_E4, get_note_duration_in_ms(3.f));

        // 13
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(2.f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(1.f));

        // 14
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_B4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(1.f));

        // 15
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_A4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.f));

        // 16
        tone(BUZZER_PIN, NOTE_E4, get_note_duration_in_ms(3.f));

        // 17
        tone(BUZZER_PIN, NOTE_D5, get_note_duration_in_ms(2.f));
        tone(BUZZER_PIN, NOTE_D5, get_note_duration_in_ms(1.f));

        // 18
        tone(BUZZER_PIN, NOTE_F5, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_D5, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_B4, get_note_duration_in_ms(1.f));

        // 19
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(3.f));

        // 20
        tone(BUZZER_PIN, NOTE_E5, get_note_duration_in_ms(3.f));

        // 21
        tone(BUZZER_PIN, NOTE_C5, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_E4, get_note_duration_in_ms(1.f));

        // 22
        tone(BUZZER_PIN, NOTE_G4, get_note_duration_in_ms(1.5f));
        tone(BUZZER_PIN, NOTE_F4, get_note_duration_in_ms(.5f));
        tone(BUZZER_PIN, NOTE_D4, get_note_duration_in_ms(1.f));

        // 23 & 24
        tone(BUZZER_PIN, NOTE_C4, get_note_duration_in_ms(5.f));
        // tone(BUZZER_PIN, <Silencio>, 2.f);
        // Fin
  }
}
