# Pico Bluetooth Examples

The source for most of the Bluetooth examples is stored in `pico-sdk/lib/btstack/example`.
There's a standalone example in `pico-examples/pico_w/bt/standalone`.

## Debugging

To debug Bluetooth issues you can enable [btstack](https://github.com/bluekitchen/btstack) debug output which also enables packet logging.
Define `WANT_HCI_DUMP=1` in your CMakeLists.txt file. Uncomment this line to enable debug in the btstack examples.

	target_compile_definitions(picow_bt_example_common INTERFACE
	    #WANT_HCI_DUMP=1 # This enables btstack debug
	    )

## Packet logging

To view packet logs, save the output from the debug port (e.g. the uart) to a file and afterwards run `pico-sdk/lib/btstack/tool/create_packet_log.py <filename>`.
This will generate a file with the same name except for a `pklg` extension. This can be opened in the [Wireshark](https://www.wireshark.org) application to analyze communications activity.

## Link keys

By default, the last two sectors of flash are used to store Bluetooth link keys and this relies on the Bluetooth address. Old pre-release versions of the SDK had issues with some devices using a default Bluetooth address. If you experience issues with pairing try Resetting flash memory, see https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#resetting-flash-memory
