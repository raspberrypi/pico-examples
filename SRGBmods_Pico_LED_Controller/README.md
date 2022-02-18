![](https://srgbmods.net/picoledcontroller/img/PicoLEDController.png)
### SRGBmods Pico LED Controller for SignalRGB

This is an open-source DIY (do it yourself) project to create a small, affordable - yet still powerful - addressable RGB controller that can be managed by **SignalRGB** with sending packets via USB HID to the Pico.

In the **/src/** folder you will find:
- the **SRGBmods_Pico_LED_Controller.ino** sketch to compile with Arduino IDE
- the **SRGBmods_Pico_LED_Controller.js** plugin file for SignalRGB

**A full tutorial about installing and using everything you need is currently being worked on and will be made available within the next few days at [SRGBmods.net](https://srgbmods.net/), stay tuned!**

### Requirements
**Hardware**
- [an affordable - yet amazing - Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/)
- a USB-A to Micro-USB data cable
- WS2812(B) LED Strip(s)
- stranded wires (22awg should suffice)
- a 5V power supply to power the strip(s)

**Software**
- [Arduino IDE 1.8.19](https://www.arduino.cc/en/software) (or newer)
- [Raspberry Pi Pico Arduino core](https://github.com/earlephilhower/arduino-pico#installing-via-arduino-boards-manager) by Earle F. Philhower, III
- [Adafruit TinyUSB Library for Arduino](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)
- [FastLED Library for colored LED animation on Arduino](https://github.com/FastLED/FastLED)
- [SignalRGB](https://signalrgb.com) for controlling the Pico via USB packets

### Notes
As of now, FastLED does **not** natively support the RP2040, so you will have to **manually merge** [THESE](https://github.com/FastLED/FastLED/pull/1261/files#diff-fda1710ad90fcc4b2f07be21a834da7d24b00008867655232c84fb0369cfc74b) changes with the FastLED lib after installing it via Arduino IDE!