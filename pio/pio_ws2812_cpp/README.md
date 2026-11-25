# Raspberry Pi Pico WS2812 Parallel Pio C++ Example

This is an example using C++ based on the ws2812_parallel example

This example is split into separate files for easier reuse in projects.

## CRGB.hpp

Structure for handling RGB data inspired by FastLED:
- Provides constructors for colours defined as three 8-bit and one 24-bit values.
- Can be accessed using r,g,b members, by byte index or as a 24-bit value.
- Provide overloads for common maths operations
- toGRB() member function returns 24-bit colour in GRB format used for WS2812 LEDS

## PLED.hpp

PLED namespace contains a collection of the low level parallel pio code

To override defaults add #define before including this file:

- NUM_STRIPS - up to 32 parallel strips

- LEDS_PER_STRIP

- PIN_BASE - first pin in the contiguous set

- INITIAL_BRIGHT - Initialise value for PLED::brightness which sets the global brightness

In the main programme:

- PLED::init() - sets up the DMA channels and pio program
- PLED::show() - transmits the color data to the LED strips

The same set of colour data can be access through arrays of CRGB structs:

- PLED::strip[NUM_STRIPS][LEDS_PER_STRIP]
- PLED::led[NUM_STRIPS*LEDS_PER_STRIP]