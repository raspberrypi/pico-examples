# Raspberry Pi Pico SDK Examples

## Getting started

See [Getting Started with the Raspberry Pi Pico](https://rptl.io/pico-get-started) and the README in the [pico-sdk](https://github.com/raspberrypi/pico-sdk) for information
on getting up and running.

### First  Examples

App|Description | Link to prebuilt UF2
---|---|---
[hello_serial](hello_world/serial) | The obligatory Hello World program for Pico (Output over serial version) |
[hello_usb](hello_world/usb) | The obligatory Hello World program for Pico (Output over USB version) | https://rptl.io/pico-hello-usb
[blink](blink) | Blink an LED on and off. | https://rptl.io/pico-blink

### ADC

App|Description
---|---
[hello_adc](adc/hello_adc)|Display the voltage from an ADC input.
[joystick_display](adc/joystick_display)|Display a Joystick X/Y input based on two ADC inputs.
[adc_console](adc/adc_console)|An interactive shell for playing with the ADC. Includes example of free-running capture mode.
[onboard_temperature](adc/onboard_temperature)|Display the value of the onboard temperature sensor.
[microphone_adc](adc/microphone_adc)|Read analog values from a microphone and plot the measured sound amplitude.

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
[bmp280_i2c](i2c/bmp280_i2c) | Read and convert temperature and pressure data from a BMP280 sensor, attached to an I2C bus.
[lcd_1602_i2c](i2c/lcd_1602_i2c) | Display some text on a generic 16x2 character LCD display, via I2C.
[lis3dh_i2c](i2c/lis3dh_i2c) | Read acceleration and temperature value from a LIS3DH sensor via I2C
[mcp9808_i2c](i2c/mcp9808_i2c) | Read temperature, set limits and raise alerts when limits are surpassed.
[mma8451_i2c](i2c/mma8451_i2c) | Read acceleration from a MMA8451 accelerometer and set range and precision for the data.
[mpl3115a2_i2c](i2c/mpl3115a2_i2c) | Interface with an MPL3115A2 altimeter, exploring interrupts and advanced board features, via I2C.
[mpu6050_i2c](i2c/mpu6050_i2c) | Read acceleration and angular rate values from a MPU6050 accelerometer/gyro, attached to an I2C bus.
[oled_i2c](i2c/oled_i2c) | Convert and display a bitmap on a 128x32 SSD1306-driven OLED display
[pa1010d_i2c](i2c/pa1010d_i2c) | Read GPS location data, parse and display data via I2C.
[pcf8523_i2c](i2c/pcf8523_i2c) | Read time and date values from a real time clock. Set current time and alarms on it.

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

### Pico W Networking

These eaxmples are for the Pico W, and are only available for `PICO_BOARD=pico_w`

App|Description
---|---
[picow_access_point](pico_w/access_point)| Starts a WiFi access point, and fields DHCP requests.
[picow_blink](pico_w/blink)| Blinks the on-board LED (which is connected via the WiFi chip).
[picow_iperf_server](pico_w/iperf)| Runs an "iperf" server for WiFi speed testing.
[picow_ntp_client](pico_w/ntp_client)| Connects to an NTP server to fetch and display the current time.
[picow_tcp_client](pico_w/tcp_client)| A simple TCP client. You can run [python_test_tcp_server.py](pico_w/python_test_tcp/python_test_tcp_server.py) for it to connect to.
[picow_tcp_server](pico_w/tcp_server)| A simple TCP server. You can use [python_test_tcp_client.py](pico_w/python_test_tcp/python_test_tcp_client.py) to connect to it.
[picow_wifi_scan](pico_w/wifi_scan)| Scans for WiFi networks and prints the results.

#### FreeRTOS examples

These are examples of integrating Pico W networking under FreeRTOS, and require you to set the `FREERTOS_KERNEL_PATH`
to point to the FreeRTOS Kernel.

App|Description
---|---
[picow_freertos_iperf_server_nosys](pico_w/freertos/iperf)| Runs an "iperf" server for WiFi speed testing under FreeRTOS in NO_SYS=1 mode. The LED is blinked in another task
[picow_freertos_iperf_server_sys](pico_w/freertos/iperf)| Runs an "iperf" server for WiFi speed testing under FreeRTOS in NO_SYS=0 (i.e. full FreeRTOS integration) mode. The LED is blinked in another task
[picow_freertos_ping_nosys](pico_w/freertos/ping)| Runs the lwip-contrib/apps/ping test app under FreeRTOS in NO_SYS=1 mode.
[picow_freertos_iperf_server_sys](pico_w/freertos/iperf)| Runs the lwip-contrib/apps/ping test app under FreeRTOS in NO_SYS=0 (i.e. full FreeRTOS integration) mode. The test app uses the lwIP \em socket API in this case. 

### PIO

App|Description
---|---
[hello_pio](pio/hello_pio)| Absolutely minimal example showing how to control an LED by pushing values into a PIO FIFO.
[apa102](pio/apa102)| Rainbow pattern on on a string of APA102 addressable RGB LEDs.
[differential_manchester](pio/differential_manchester)| Send and receive differential Manchester-encoded serial (BMC).
[hub75](pio/hub75)| Display an image on a 128x64 HUB75 RGB LED matrix.
[i2c](pio/i2c)| Scan an I2C bus.
[ir_nec](pio/ir_nec)| Sending and receiving IR (infra-red) codes using the PIO.
[logic_analyser](pio/logic_analyser)| Use PIO and DMA to capture a logic trace of some GPIOs, whilst a PWM unit is driving them.
[manchester_encoding](pio/manchester_encoding)| Send and receive Manchester-encoded serial.
[pio_blink](pio/pio_blink)| Set up some PIO state machines to blink LEDs at different frequencies, according to delay counts pushed into their FIFOs.
[pwm](pio/pwm)| Pulse width modulation on PIO. Use it to gradually fade the brightness of an LED.
[spi](pio/spi)| Use PIO to erase, program and read an external SPI flash chip. A second example runs a loopback test with all four CPHA/CPOL combinations.
[squarewave](pio/squarewave)| Drive a fast square wave onto a GPIO. This example accesses low-level PIO registers directly, instead of using the SDK functions.
[st7789_lcd](pio/st7789_lcd)| Set up PIO for 62.5 Mbps serial output, and use this to display a spinning image on a ST7789 serial LCD.
[quadrature_encoder](pio/quadrature_encoder)| A quadrature encoder using PIO to maintain counts independent of the CPU. 
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
[spi_master_slave](spi/spi_master_slave) | Demonstrate SPI communication as master and slave.
[max7219_8x7seg_spi](spi/max7219_8x7seg_spi) | Attaching a Max7219 driving an 8 digit 7 segment display via SPI
[max7219_32x8_spi](spi/max7219_32x8_spi) | Attaching a Max7219 driving an 32x8 LED display via SPI

### System

App|Description
---|---
[hello_double_tap](system/hello_double_tap) | An LED blink with the `pico_bootsel_via_double_reset` library linked. This enters the USB bootloader when it detects the system being reset twice in quick succession, which is useful for boards with a reset button but no BOOTSEL button.
[narrow_io_write](system/narrow_io_write) | Demonstrate the effects of 8-bit and 16-bit writes on a 32-bit IO register.
[unique_board_id](system/unique_board_id) | Read the 64 bit unique ID from external flash, which serves as a unique identifier for the board.

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
[lcd_uart](uart/lcd_uart) | Display text and symbols on a 16x02 RGB LCD display via UART
[uart_advanced](uart/uart_advanced) | Use some other UART features like RX interrupts, hardware control flow, and data formats other than 8n1.

### USB Device

#### TinyUSB Examples 

Most of the USB device examples come directly from the TinyUSB device examples directory [here](https://github.com/hathach/tinyusb/tree/master/examples/device).
Those that are supported on RP2040 devices are automatically included as part of the pico-examples
build as targets named `tinyusb_dev_<example_name>`, e.g. https://github.com/hathach/tinyusb/tree/master/examples/device/hid_composite
is built as `tinyusb_dev_hid_composite`.

At the time of writing, these examples are available:

- tinyusb_dev_audio_4_channel_mic
- tinyusb_dev_audio_test
- tinyusb_dev_board_test
- tinyusb_dev_cdc_dual_ports
- tinyusb_dev_cdc_msc
- tinyusb_dev_dfu
- tinyusb_dev_dfu_runtime
- tinyusb_dev_dynamic_configuration
- tinyusb_dev_hid_composite
- tinyusb_dev_hid_generic_inout
- tinyusb_dev_hid_multiple_interface
- tinyusb_dev_midi_test
- tinyusb_dev_msc_dual_lun
- tinyusb_dev_net_lwip_webserver
- tinyusb_dev_uac2_headset
- tinyusb_dev_usbtmc
- tinyusb_dev_video_capture
- tinyusb_dev_webusb_serial

Whilst these examples ably demonstrate how to use TinyUSB in device mode, their `CMakeLists.txt` is set up in a way
tailored to how TinyUSB builds their examples within their source tree.

For a better example of how to configure `CMakeLists.txt` for using TinyUSB in device mode with the Raspberry Pi SDK
see below:

#### SDK build example 
App|Description
---|---
[dev_hid_composite](usb/device/dev_hid_composite) | A copy of the TinyUSB device example with the same name, but with a CMakeLists.txt which demonstrates how to add a dependency on the TinyUSB device libraries with the Raspberry Pi Pico SDK

#### Low Level example
App|Description
---|---
[dev_lowlevel](usb/device/dev_lowlevel) | A USB Bulk loopback implemented with direct access to the USB hardware (no TinyUSB)

### USB Host

All the USB host examples come directly from the TinyUSB host examples directory [here](https://github.com/hathach/tinyusb/tree/master/examples/host).
Those that are supported on RP2040 devices are automatically included as part of the pico-examples
build as targets named `tinyusb_host_<example_name>`, e.g. https://github.com/hathach/tinyusb/tree/master/examples/host/cdc_msc_hid
is built as `tinyusb_host_cdc_msc_hid`.

At the time of writing, there is only one host example available:

- tinyusb_host_cdc_msc_hid

### Watchdog

App|Description
---|---
[hello_watchdog](watchdog/hello_watchdog) | Set the watchdog timer, and let it expire. Detect the reboot, and halt.
