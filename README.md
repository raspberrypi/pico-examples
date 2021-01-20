# PICO SDK Examples

## Getting started

See [Getting Started with the Raspberry Pi Pico](https://rptl.io/pico-get-started) and the README in the [pico-sdk](https://github.com/raspberrypi/pico-sdk) for information
on getting up and running.

### First  Examples

App | Description | Link to prebuilt UF2
---|---|---
[hello_serial](hello_world/serial) | The obligatory Hello World program for Pico (Output over serial version) | https://rptl.io/pico-hello-serial
[hello_usb](hello_world/usb) | The obligatory Hello World program for Pico (Output over USB version) | https://rptl.io/pico-hello-usb
[blink](blink) | Blink an LED on and off. | https://rptl.io/pico-blink

### ADC

App|Description
---|---
[hello_adc](adc/hello_adc)|Display the voltage from an ADC input.
[joystick_display](adc/joystick_display)|Display a Joystick X/Y input based on two ADC inputs.
[adc_console](adc/adc_console)|An interactive shell for playing with the ADC. Includes example of free-running capture mode.

### Clocks

App|Description
---|---
[hello_48MHz](clocks/hello_48MHz)| Change the system clock frequency to 48 MHz while running.
[hello_gpout](clocks/hello_gpout)| Use the general purpose clock outputs (GPOUT) to drive divisions of internal clocks onto GPIO outputs.
[hello_resus](clocks/hello_resus)| Enable the clock resuscitate feature, "accidentally" stop the system clock, and show how we recover.

### CMake

App|Description
---|---
[build_variants](cmake/build_variants)| Builds two version of the same app with different configurations

### DMA

App|Description
---|---
[hello_dma](dma/hello_dma)| Use the DMA to copy data in memory.
[control_blocks](dma/control_blocks)| Build a control block list, to program a longer sequence of DMA transfers to the UART.
[channel_irq](dma/channel_irq)| Use an IRQ handler to reconfigure a DMA channel, in order to continuously drive data through a PIO state machine.


### Flash

App|Description
---|---
[cache_perfctr](flash/cache_perfctr)| Read and clear the cache performance counters. Show how they are affected by different types of flash reads.
[nuke](flash/nuke)| Obliterate the contents of flash. An example of a NO_FLASH binary (UF2 loaded directly into SRAM and runs in-place there). A useful utility to drag and drop onto your Pico if the need arises.
[program](flash/program)| Erase a flash sector, program one flash page, and read back the data.
[xip_stream](flash/xip_stream)| Stream data using the XIP stream hardware, which allows data to be DMA'd in the background whilst executing code from flash.
[ssi_dma](flash/ssi_dma)| DMA directly from the flash interface (continuous SCK clocking) for maximum bulk read performance.

### GPIO

App|Description
---|---
[hello_7segment](gpio/hello_7segment) | Use the GPIOs to drive a seven segment LED display.
[hello_gpio_irq](gpio/hello_gpio_irq) | Register an interrupt handler to run when a GPIO is toggled.
[dht_sensor](gpio/dht_sensor) | Use GPIO to bitbang the serial protocol for a DHT temperature/humidity sensor.

See also: [blink](blink), blinking an LED attached to a GPIO.

### HW divider

App|Description
---|---
[hello_divider](divider) | Show how to directly access the hardware integer dividers, in case AEABI injection is disabled.


### I2C

App|Description
---|---
[bus_scan](i2c/bus_scan) | Scan the I2C bus for devices and display results.
[lcd_1602_i2c](i2c/lcd_1602_i2c) | Display some text on a generic 16x2 character LCD display, via I2C.
[mpu6050_i2c](i2c/mpu6050_i2c) | Read acceleration and angular rate values from a MPU6050 accelerometer/gyro, attached to an I2C bus.

### Interpolator

App|Description
---|---
[hello_interp](interp/hello_interp) | A bundle of small examples, showing how to access the core-local interpolator hardware, and use most of its features.

### Multicore

App|Description
---|---
[hello_multicore](multicore/hello_multicore) | Launch a function on the second core, printf some messages on each core, and pass data back and forth through the mailbox FIFOs.
[multicore_fifo_irqs](multicore/multicore_fifo_irqs) | On each core, register and interrupt handler for the mailbox FIFOs. Show how the interrupt fires when that core receives a message. 
[multicore_runner](multicore/multicore_runner) | Set up the second core to accept, and run, any function pointer pushed into its mailbox FIFO. Push in a few pieces of code and get answers back.

### Pico Board

App|Description
---|---
[blinky](picoboard/blinky)| Blink "hello, world" in Morse code on Pico's LED
[button](picoboard/button)| Use Pico's BOOTSEL button as a regular button input, by temporarily suspending flash access.

### PIO

App|Description
---|---
[hello_pio](pio/hello_pio)| Absolutely minimal example showing how to control an LED by pushing values into a PIO FIFO.
[apa102](pio/apa102)| Rainbow pattern on on a string of APA102 addressable RGB LEDs.
[differential_manchester](pio/differential_manchester)| Send and receive differential Manchester-encoded serial (BMC).
[hub75](pio/hub75)| Display an image on a 128x64 HUB75 RGB LED matrix.
[i2c](pio/i2c)| Scan an I2C bus.
[logic_analyser](pio/logic_analyser)| Use PIO and DMA to capture a logic trace of some GPIOs, whilst a PWM unit is driving them.
[manchester_encoding](pio/manchester_encoding)| Send and receive Manchester-encoded serial.
[pio_blink](pio/pio_blink)| Set up some PIO state machines to blink LEDs at different frequencies, according to delay counts pushed into their FIFOs.
[pwm](pio/pwm)| Pulse width modulation on PIO. Use it to gradually fade the brightness of an LED.
[spi](pio/spi)| Use PIO to erase, program and read an external SPI flash chip. A second example runs a loopback test with all four CPHA/CPOL combinations.
[squarewave](pio/squarewave)| Drive a fast square wave onto a GPIO. This example accesses low-level PIO registers directly, instead of using the SDK functions.
[st7789_lcd](pio/st7789_lcd)| Set up PIO for 62.5 Mbps serial output, and use this to display a spinning image on a ST7789 serial LCD.
[uart_rx](pio/uart_rx)| Implement the receive component of a UART serial port. Attach it to the spare Arm UART to see it receive characters.
[uart_tx](pio/uart_tx)| Implement the transmit component of a UART serial port, and print hello world.
[ws2812](pio/ws2812)| Examples of driving WS2812 addressable RGB LEDs.
[addition](pio/addition)| Add two integers together using PIO. Only around 8 billion times slower than Cortex-M0+.

### PWM

App|Description
---|---
[hello_pwm](pwm/hello_pwm) | Minimal example of driving PWM output on GPIOs.
[led_fade](pwm/led_fade) | Fade an LED between low and high brightness. An interrupt handler updates the PWM slice's output level each time the counter wraps.
[measure_duty_cycle](pwm/measure_duty_cycle) | Drives a PWM output at a range of duty cycles, and uses another PWM slice in input mode to measure the duty cycle.

### Reset

App|Description
---|---
[hello_reset](reset/hello_reset) | Perform a hard reset on some peripherals, then bring them back up.

### RTC

App|Description
---|---
[hello_rtc](rtc/hello_rtc) | Set a date/time on the RTC, then repeatedly print the current time, 10 times per second, to show it updating.
[rtc_alarm](rtc/rtc_alarm) | Set an alarm on the RTC to trigger an interrupt at a date/time 5 seconds into the future.
[rtc_alarm_repeat](rtc/rtc_alarm_repeat) | Trigger an RTC interrupt once per minute.

### SPI

App|Description
---|---
[bme280_spi](spi/bme280_spi) | Attach a BME280 temperature/humidity/pressure sensor via SPI.
[mpu9250_spi](spi/mpu9250_spi) | Attach a MPU9250 accelerometer/gyoscope via SPI.
[spi_dma](spi/spi_dma) | Use DMA to transfer data both to and from the SPI simultaneously. The SPI is configured for loopback.
[spi_flash](spi/spi_flash) | Erase, program and read a serial flash device attached to one of the SPI controllers.

### System

App|Description
---|---
[hello_double_tap](system/hello_double_tap) | On dev boards with a reset button (but no BOOTSEL), a magic number in RAM can be used to enter the USB bootloader, when the reset button is pressed twice quickly.
[narrow_io_write](system/narrow_io_write) | Demonstrate the effects of 8-bit and 16-bit writes on a 32-bit IO register.

### Timer

App|Description
---|---
[hello_timer](timer/hello_timer) | Set callbacks on the system timer, which repeat at regular intervals. Cancel the timer when we're done.
[periodic_sampler](timer/periodic_sampler) | Sample GPIOs in a timer callback, and push the samples into a concurrency-safe queue. Pop data from the queue in code running in the foreground.
[timer_lowlevel](timer/timer_lowlevel) | Example of direct access to the timer hardware. Not generally recommended, as the SDK may use the timer for IO timeouts.

### UART

App|Description
---|---
[hello_uart](uart/hello_uart) | Print some text from one of the UART serial ports, without going through `stdio`.
[uart_advanced](uart/uart_advanced) | Use some other UART features like RX interrupts, hardware control flow, and data formats other than 8n1.

### USB Device

App|Description
---|---
[dev_audio_headset](usb/device/dev_audio_headset) | Audio headset example from TinyUSB
[dev_hid_composite](usb/device/dev_hid_composite) | Composite HID (mouse + keyboard) example from TinyUSB
[dev_hid_generic_inout](usb/device/dev_hid_generic_inout) | Generic HID device example from TinyUSB
[dev_lowlevel](usb/device/dev_lowlevel) | A USB Bulk loopback implemented with direct access to the USB hardware (no TinyUSB)

### USB Host

App|Description
---|---
[host_hid](usb/host/host_hid) | Use USB in host mode to poll an attached HID keyboard (TinyUSB example)

### Watchdog

App|Description
---|---
[hello_watchdog](watchdog/hello_watchdog) | Set the watchdog timer, and let it expire. Detect the reboot, and halt.
