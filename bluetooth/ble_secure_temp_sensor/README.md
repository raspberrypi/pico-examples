# Secure temp sensor

This example uses BLE to communicate temperature between a pair of pico Ws. This example is a variant of temp sensor, using LE secure to provide a secure connection. 

secure_temp_server is a peripheral or server that transmits its temperature to another device
secure_temp_client is a client that reads a temperature from another device

In server.c and client.c there is a define SECURITY_SETTING which you can change to explore different security options:

security setting 0: Just works (pairing), no MITM (Man In The Middle) protection
 client and server have no input or output support

security setting 1: Numeric comparison with MITM protection
 client can query yes or no from the user, server has a display only
 server displays passkey
 client displays passkey and user can select Yes or No if they agree the passkey is from the server

security setting 2:
 client has a keyboard and display, server has a display only
 server displays passkey
 client user enters the passkey displayed by the server

security setting 3:
 client has a display only, server has a display and keyboard
 Client displays passkey
 server user enters the passkey displayed by the server

You will need to use the console with both devices to see the passkeys and answer security prompts. Both stdio over UART and USB are enabled so you can use either.

## Support scripts

Some scripts are provided to make running these examples easier with just one Pico device.

Client (ble_temp_client.py):

Requires pip install bleak
Works with BlueZ running normally
If connection fails with a disconnect during discovery, run bluetoothctl remove <addr> to clear stale bonding info

Server (ble_temp_server.py):

Requires pip install bumble

When running on Raspberry Pi OS, it needs cap_net_admin on the Python binary: sudo setcap cap_net_admin+eip $(readlink -f venv/bin/python3). You must stop and mask BlueZ first: sudo systemctl stop bluetooth && sudo systemctl mask bluetooth
Run with python ble_temp_server.py --builtin
When done, restore BlueZ: sudo systemctl unmask bluetooth && sudo systemctl start bluetooth

To avoid clashing with BlueZ you can use a BT USB dongle.

python3 -m venv venv
. venv/bin/activate
pip install bumble
python3 -m bumble.apps.usb_probe
./ble_temp_server.py --usb 0
