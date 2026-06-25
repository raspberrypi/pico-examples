# Secure temp sensor

This example uses BLE to communicate temperature between a pair of Pico Ws. It is a variant of
the temp sensor example, using LE Secure Connections to provide a secure connection.

`secure_temp_server` is a peripheral/server that transmits its temperature to another device.
`secure_temp_client` is a central/client that reads temperature from another device.

## Security settings

In `server.c` and `client.c` there is a `SECURITY_SETTING` define which you can change to explore
different BLE security options. Both ends must be built with the same setting unless you are
deliberately testing an asymmetric combination (see the table below).

The settings map to Bluetooth IO capabilities as follows:

| Setting | IO capability | Description |
|---------|--------------|-------------|
| 0 | `NO_INPUT_NO_OUTPUT` | Just Works - no MITM protection |
| 1 | `DISPLAY_YES_NO` | Numeric Comparison - MITM protection |
| 2 | `KEYBOARD_DISPLAY` | Passkey Entry - MITM protection |
| 3 | `DISPLAY_ONLY` | Display Only - MITM protection |

The actual pairing method used depends on the IO capabilities of *both* devices, not just one.
The Bluetooth SIG defines a matrix of initiator × responder capabilities that determines the
method. Setting `SM_AUTHREQ_MITM_PROTECTION` requests MITM protection but does not guarantee it —
if the negotiated method cannot provide it, pairing will fail.

### Working combinations

| Client setting | Server setting | Pairing method | MITM? |
|---------------|---------------|----------------|-------|
| 0 | 0 | Just Works | No |
| 1 | 1 | Numeric Comparison | Yes |
| 2 | 2 | Passkey Entry | Yes |
| 2 | 3 | Passkey Display (server displays, client types) | Yes |
| 3 | 2 | Passkey Display (client displays, server types) | Yes |
| 3 | 3 | Fails (both display only, nobody can type) | — |
| 0 | >0 | Fails (MITM required but not achievable) | — |

Settings 0, 1 and 2 work symmetrically with the same setting on both ends. Setting 3 is
`DISPLAY_ONLY` so it can only achieve MITM protection when paired with setting 2 on the other end.

You will need a console on each device to see passkeys and answer prompts. Both stdio over UART
and USB are enabled so you can use either.

## Support scripts

Python scripts are provided to make it easier to test with just one Pico W.

> **Note:** Run these scripts on a native Linux host or a Raspberry Pi. WSL2 with a
> usbip-attached Bluetooth adapter can connect and perform GATT discovery, but pairing
> fails during the LE Secure Connections exchange (the DHKey check fails with
> authentication failure / reason 12). This is a limitation of the WSL2 + usbip + BlueZ
> path, not the example code.

### Client (`ble_temp_client.py`)

Acts as a BLE central, connecting to a Pico W running `secure_temp_server`.

```
pip install bleak
python3 ble_temp_client.py [--security <0-3>]
```

- Works with BlueZ running normally — no special setup required.
- If connection fails with a disconnect during service discovery, clear stale bonding info:
  ```
  bluetoothctl remove <addr>
  ```
  The Pico clears its own bond automatically on key mismatch, but BlueZ needs to be told manually.

### Server (`ble_temp_server.py`)

Acts as a BLE peripheral, advertising a simulated temperature for a Pico W running
`secure_temp_client` to connect to.

Uses [Bumble](https://github.com/google/bumble) which talks directly over HCI, bypassing BlueZ.

```
pip install bumble
```

#### Using a USB Bluetooth dongle (recommended)

The easiest option — the dongle is claimed by Bumble leaving the built-in adapter free for BlueZ
and the client script. Find the transport ID with:

```
python3 -m bumble.apps.usb_probe
```

Then run:

```
./ble_temp_server.py --usb <id> [--security <0-3>]
```

#### Using the built-in adapter

BlueZ must be stopped first to release the adapter:

```
sudo systemctl stop bluetooth
sudo systemctl mask bluetooth
```

On Raspberry Pi OS, grant `cap_net_admin` to the Python binary so it can open the HCI socket
without running as root:

```
sudo setcap cap_net_admin+eip $(readlink -f venv/bin/python3)
```

Then run:

```
./ble_temp_server.py --builtin [--security <0-3>]
```

When done, restore BlueZ:

```
sudo systemctl unmask bluetooth
sudo systemctl start bluetooth
```

### Security mode prompts

For settings 1-3 the scripts will prompt for interaction during pairing:

- **Setting 1 (Numeric Comparison)**: The Pico displays a number; confirm it matches when prompted.
- **Setting 2 (Passkey Entry)**: The server displays a passkey; type it into the Pico console.
- **Setting 3 (Display Only)**: The Pico displays a passkey; type it when the script prompts.

For asymmetric combinations (client=2, server=3 or client=3, server=2) the above still applies —
one side displays and the other types, determined by who has `KEYBOARD_DISPLAY` capability.