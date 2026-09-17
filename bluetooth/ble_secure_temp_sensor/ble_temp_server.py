#!/usr/bin/env python3
"""
BLE test server for the secure_picow_temp example.

Advertises an Environmental Sensing GATT service with a simulated temperature
characteristic (slowly varying sine wave), allowing the Pico W client to
connect and receive notifications.

Uses Bumble (https://github.com/google/bumble) which talks directly over HCI,
bypassing BlueZ entirely.  BlueZ must be stopped before running this script:

    sudo systemctl stop bluetooth
    python ble_temp_server.py [--transport usb:0]
    sudo systemctl start bluetooth   # when done

To find the right transport index for your adapter:
    python -m bumble.apps.usb_probe

Requirements:
    pip install bumble

Security settings (must match the SECURITY_SETTING the Pico client was built with):
    0 - Just Works         (NO_INPUT_NO_OUTPUT, no MITM)
    1 - Numeric Comparison (DISPLAY_YES_NO, MITM)
    2 - Keyboard + Display (KEYBOARD_DISPLAY, MITM)
    3 - Display Only       (DISPLAY_ONLY, MITM)

Usage:
    python ble_temp_server.py [--security <0-3>] [--temp <degC>]
                              [--interval <seconds>] [--transport <transport>]
"""

import argparse
import asyncio
import logging
import math
import struct
import sys

try:
    from bumble.device import Device, Connection
    from bumble.hci import Address
    from bumble.transport import open_transport
    from bumble.gatt import (
        Service,
        Characteristic,
        CharacteristicValue,
        GATT_ENVIRONMENTAL_SENSING_SERVICE,
    )
    from bumble.att import UUID
    from bumble.core import AdvertisingData
    from bumble.pairing import PairingConfig, PairingDelegate
except ImportError as e:
    print(f"ERROR: failed to import bumble: {e}")
    print("If bumble is installed, check you are using the right Python/venv.")
    sys.exit(1)

# Temperature characteristic UUID (0x2A6E) - not a named constant in bumble
GATT_TEMPERATURE_CHARACTERISTIC = UUID.from_16_bits(0x2A6E, 'Temperature')

# ---------------------------------------------------------------------------
# Constants matching the Pico example
# ---------------------------------------------------------------------------

SERVER_DEVICE_NAME = "secure_picow_temp"

# Temperature is a signed 16-bit integer in units of 0.01 degC (Bluetooth SIG spec).
# The Pico client expects current_temp = deg_c * 100 as uint16_t, which matches.
TEMP_SCALE = 100.0

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger(__name__)

# Suppress noisy bumble internal warnings that fire harmlessly during shutdown
logging.getLogger("bumble.device").setLevel(logging.ERROR)
logging.getLogger("bumble.host").setLevel(logging.ERROR)

# ---------------------------------------------------------------------------
# Security configuration
# ---------------------------------------------------------------------------

class DisplayDelegate(PairingDelegate):
    """PairingDelegate that displays a passkey to the user."""

    def __init__(self):
        super().__init__(io_capability=PairingDelegate.IoCapability.DISPLAY_OUTPUT_ONLY)

    async def display_number(self, number: int, digits: int) -> None:
        log.info("Passkey: %0*d  (enter this on the client)", digits, number)


class KeyboardDelegate(PairingDelegate):
    """PairingDelegate that prompts the user to enter a passkey from stdin."""

    def __init__(self, io_capability=PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY):
        super().__init__(io_capability=io_capability)

    async def get_number(self) -> int | None:
        # Run the blocking input() call in a thread so we don't block the event loop.
        loop = asyncio.get_event_loop()
        try:
            passkey_str = await loop.run_in_executor(
                None, lambda: input("Enter passkey shown by the client: ")
            )
            return int(passkey_str.strip())
        except (ValueError, EOFError):
            log.warning("Invalid passkey input - declining pairing.")
            return None


def make_pairing_config(security: int) -> PairingConfig:
    """Return a Bumble PairingConfig matching the given security setting."""
    # All modes use Secure Connections (sc=True) and bonding, matching the
    # sm_set_secure_connections_only_mode(true) call in the Pico client.
    common = dict(sc=True, bonding=True)
    if security == 0:
        # Just Works: no I/O capability, no MITM
        return PairingConfig(mitm=False, delegate=PairingDelegate(
            io_capability=PairingDelegate.IoCapability.NO_OUTPUT_NO_INPUT,
        ), **common)
    elif security == 1:
        # Numeric Comparison: server displays, client confirms yes/no
        return PairingConfig(mitm=True, delegate=PairingDelegate(
            io_capability=PairingDelegate.IoCapability.DISPLAY_OUTPUT_AND_YES_NO_INPUT,
        ), **common)
    elif security == 2:
        # Keyboard + Display: matches IO_CAPABILITY_KEYBOARD_DISPLAY on the Pico.
        # Pairing method depends on the other side's capabilities.
        return PairingConfig(mitm=True, delegate=KeyboardDelegate(
            io_capability=PairingDelegate.IoCapability.DISPLAY_OUTPUT_AND_KEYBOARD_INPUT,
        ), **common)
    else:
        # security == 3: Display Only - matches IO_CAPABILITY_DISPLAY_ONLY on the Pico.
        # Requires the Pico client to be built with SECURITY_SETTING=2 (KEYBOARD_DISPLAY)
        # so it can enter the passkey we display here.
        return PairingConfig(mitm=True, delegate=DisplayDelegate(), **common)

# ---------------------------------------------------------------------------
# Main server coroutine
# ---------------------------------------------------------------------------

async def run(temp_celsius: float, interval: float, security: int,
              transport: str) -> None:
    log.info("Base temperature: %.2f degC, notification interval: %.1f s, "
             "security setting: %d, transport: %s",
             temp_celsius, interval, security, transport)

    # Shared mutable temperature value, updated by the notification loop and
    # read by the GATT read handler.
    current_raw = [int(round(temp_celsius * TEMP_SCALE))]

    def read_temperature(_connection):
        return struct.pack("<h", current_raw[0])

    temp_char = Characteristic(
        GATT_TEMPERATURE_CHARACTERISTIC,
        Characteristic.Properties.READ | Characteristic.Properties.NOTIFY,
        Characteristic.READABLE,
        CharacteristicValue(read=read_temperature),
    )

    ess = Service(GATT_ENVIRONMENTAL_SENSING_SERVICE, [temp_char])

    adv_data = bytes(AdvertisingData([
        (AdvertisingData.COMPLETE_LOCAL_NAME,
         SERVER_DEVICE_NAME.encode()),
        (AdvertisingData.COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
         bytes(GATT_ENVIRONMENTAL_SENSING_SERVICE)),
    ]))

    shutting_down = False

    class Listener(Device.Listener, Connection.Listener):
        def on_connection(self, connection):
            log.info("Client connected: %s", connection)
            connection.listener = self

        def on_disconnection(self, reason):
            if shutting_down:
                return
            log.info("Client disconnected (reason=0x%02x) - waiting for auto-restart.", reason)

        def on_pairing(self, keys):
            log.info("Pairing complete.")

        def on_pairing_failure(self, reason):
            log.warning("Pairing failed (reason=0x%02x).", reason)

    async with await open_transport(transport) as t:
        # Use a temporary random address for construction; after power_on()
        # the real public address is read from the controller and we update it.
        device = Device.with_hci(
            name=SERVER_DEVICE_NAME,
            address=Address.generate_static_address(),
            hci_source=t.source,
            hci_sink=t.sink,
        )
        device.add_services([ess])
        device.pairing_config_factory = lambda _: make_pairing_config(security)
        device.listener = Listener()
        await device.power_on()
        # Use the real public address now that the controller has reported it.
        if device.public_address and device.public_address != Address.ANY:
            device.address = device.public_address
        log.info("Adapter address: %s", device.address)
        await device.start_advertising(auto_restart=True, advertising_data=adv_data)
        log.info("Advertising as '%s'.  Press Ctrl-C to stop.", SERVER_DEVICE_NAME)

        start = 0.0  # elapsed time for the sine wave, updated each tick
        try:
            while True:
                await asyncio.sleep(interval)
                start += interval
                current_temp = temp_celsius + 2.0 * math.sin(start / 30.0)
                current_raw[0] = max(-32768, min(32767,
                                                 int(round(current_temp * TEMP_SCALE))))
                data = struct.pack("<h", current_raw[0])
                log.info("Notifying temperature: %.2f degC  (raw=0x%04x)",
                         current_temp, current_raw[0] & 0xFFFF)
                await device.notify_subscribers(temp_char, data)
        except asyncio.CancelledError:
            pass
        finally:
            shutting_down = True
            log.info("Stopping server.")
            try:
                await device.stop_advertising()
                await device.power_off()
            except Exception:
                pass  # ignore errors during shutdown (socket/HCI already closed)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="BLE server for the secure_picow_temp example.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--temp", "-t", type=float, default=21.0,
        help="Base temperature in degC.  A +/-2 degC sine wave is added.  Default: 21.0.",
    )
    parser.add_argument(
        "--interval", "-i", type=float, default=10.0,
        help="Seconds between notifications.  Default: 10 (matches the Pico server).",
    )
    parser.add_argument(
        "--security", "-s", type=int, default=0, choices=[0, 1, 2, 3],
        help="Security setting (must match the Pico client build).  Default: 0.",
    )
    transport_group = parser.add_mutually_exclusive_group(required=True)
    transport_group.add_argument(
        "--usb", type=str, metavar="ID",
        help=(
            "Use a USB Bluetooth dongle as the adapter.  "
            "Find the ID with: python -m bumble.apps.usb_probe  "
            "(e.g. --usb 0 or --usb 2E8A:000C)"
        ),
    )
    transport_group.add_argument(
        "--builtin", action="store_true",
        help=(
            "Use the built-in Bluetooth adapter via the kernel HCI socket.  "
            "Requires BlueZ to be stopped first: "
            "sudo systemctl mask bluetooth && sudo systemctl stop bluetooth"
        ),
    )
    transport_group.add_argument(
        "--transport", "-T", type=str, metavar="TRANSPORT",
        help=(
            "Advanced: specify a Bumble transport string directly "
            "(e.g. hci-socket:0, usb:0, usb:2E8A:000C)."
        ),
    )
    args = parser.parse_args()

    if args.usb:
        transport = f"usb:{args.usb}"
    elif args.transport:
        transport = args.transport
    else:
        transport = "hci-socket:0"

    try:
        asyncio.run(run(
            temp_celsius=args.temp,
            interval=args.interval,
            security=args.security,
            transport=transport,
        ))
    except KeyboardInterrupt:
        log.info("Interrupted - goodbye.")


if __name__ == "__main__":
    main()