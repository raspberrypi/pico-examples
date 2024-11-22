These programs demonstrate use of `bi_ptr` variables, which can be configured in a binary post-compilation using the `picotool config` command.

You can view the configurable variables with
```
$ picotool blink_any.uf2 
File blink_any.uf2:

LED Configuration:
 LED_PIN = 25
 LED_TYPE = 0

$ picotool config hello_anything.uf2 
File hello_anything.uf2:

text = "Hello, world!"
Enabled Interfaces:
 use_usb = 1
 use_uart = 1
UART Configuration:
 uart_baud = 115200
 uart_rx = 1
 uart_tx = 0
 uart_num = 0
```

For example, to blink the LED on pin 7 instead of 25 use
```
$ picotool config blink_any.uf2 -s LED_PIN 7
File blink_any.uf2:

LED_PIN = 25
setting LED_PIN -> 7
```

Or to change the printed string use
```
$ picotool config hello_anything.uf2 -s text "Goodbye, world!"
File hello_anything.uf2:

text = "Hello, world!"
setting text -> "Goodbye, world!"
```

The binaries can also be configured after being loaded onto the device with
```
$ picotool config
text = "Hello, world!"
Enabled Interfaces:
 use_usb = 1
 use_uart = 1
UART Configuration:
 uart_baud = 115200
 uart_rx = 1
 uart_tx = 0
 uart_num = 0

$ picotool config -s use_uart 0
use_uart = 1
setting use_uart -> 0
```
