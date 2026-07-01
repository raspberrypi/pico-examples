# BLE Wi-Fi provisioning

This example demonstrates provisioning Wi-Fi credentials over Bluetooth Low Energy.

When the Pico W powers on it attempts to connect using credentials saved in flash.
If that fails (or none are saved), it starts a BLE GATT server that you can connect
to with a mobile BLE scanner app or with the included `set_credentials.py` script,
and write the SSID and password to it. The most recent set of credentials that
results in a successful connection is saved to flash for next time.

Once connected to the network the example runs an iperf server. Press `D` on the
Pico's console to disconnect and reboot; on the next boot it reconnects using the
saved credentials.

## Security

The credential characteristics require an encrypted link (they are marked with
`ENCRYPTION_KEY_SIZE_16`), so the SSID and password are never sent in the clear.
The client must pair with the Pico before it can write them. Pairing uses LE Secure
Connections with Just Works (no passkey), since the Pico has no input or output for
a passkey exchange. The Pico bonds with the client, so a previously-provisioned
client can reconnect without pairing again.

## The GATT server

The server exposes a custom service with two write-only characteristics: one for the
SSID and one for the password.

## Using set_credentials.py

`set_credentials.py` connects to the Pico, pairs, and writes the credentials. It
requires the `bleak` BLE library. Installing it into a virtual environment keeps it
isolated from the system Python:

```
python3 -m venv venv
. venv/bin/activate
pip install bleak
```

After the first time, you only need to activate the virtual environment:

```
. venv/bin/activate
```

The script takes three arguments: the SSID, the password, and the Bluetooth address
of the Pico running this example:

```
python set_credentials.py "my ssid" "my password" 2C:CF:67:BE:08:05
submitted ssid:  my ssid
submitted password:  my password
submitted address:  2C:CF:67:BE:08:05
Connected and paired: True
Writing SSID...
Writing password...
Credentials written successfully.
```

The script runs anywhere `bleak` runs, including Linux/Raspberry Pi, native Windows,
and macOS.

## Testing

When the example starts it waits 3 seconds for you to press `W`, which wipes any
stored SSID, password.
Press `W` again in another 3 seconds to wipe bonding information.
This is handy for repeated testing.

While provisioning, the Pico's console shows something like:

```
Waiting to receive ssid and password via BLE
Identity resolving failed
Connection complete
Pairing started
Just Works requested
Pairing complete, success
Setting SSID
Current saved SSID: "my ssid"
Current saved password length: 0
Setting password
Current saved SSID: "my ssid"
Current saved password length: 7
connect status: joining
connect status: no ip
connect status: link up
Succesfully provisioned credentials using wifi_prov_lib!
finished provisioning result=0

Ready, running iperf server at 10.3.194.230
```

On a later boot, when valid credentials are already stored in flash, it reconnects
without needing BLE:

```
Read credentials
Current saved SSID: "my ssid"
BTstack up and running on 2C:CF:67:BE:08:05.
Current saved password length: 7

connect status: joining
connect status: no ip
connect status: link up
Connected.
finished provisioning result=0

Ready, running iperf server at 10.3.194.230
```

## Troubleshooting

**Pairing or connection fails, often after re-flashing the Pico.** The two devices
may be holding mismatched bonding keys. Bonds must be cleared on *both* sides: press
`W` on the Pico twice at startup to wipe its bonds, and remove the device on the host
running `set_credentials.py`:

```
bluetoothctl remove 2C:CF:67:BE:08:05
```

(On Windows, remove the device from Bluetooth settings)

**Running under WSL.** LE Secure Connections pairing does **not** work reliably when
`set_credentials.py` is run inside WSL2 using a USB Bluetooth dongle forwarded via
`usbipd`: the pairing handshake fails during the SMP key exchange over the proxied
adapter. This has been seen with more than one dongle and is a limitation of the
WSL2/usbip USB transport.
Run the script under native Windows Python, or on Linux / Raspberry Pi with a
directly-attached Bluetooth controller.