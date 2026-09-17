#!/usr/bin/env python3
"""
BLE test client for the secure_picow_temp example.

Scans for a Pico W running the secure_picow_temp server, connects, pairs,
and prints temperature notifications until Ctrl-C.

Requirements:
    pip install bleak

Pairing (including any passkey entry or numeric comparison) is handled by the
host OS / BlueZ agent according to the security level the Pico requests, so no
security option is needed here - just follow any prompts from your OS.

Depending on the security setting the Pico server was built with, expect:
    0 - Just Works:         no passkey interaction needed.
    1 - Numeric Comparison: the Pico displays a number on its serial console;
                            your OS asks you to confirm it matches.
    2 - Passkey Entry:      the Pico displays a 6-digit passkey on its serial
                            console; enter it when prompted by your OS.
    3 - Passkey Display:    your OS displays a passkey; enter it on the Pico's
                            serial console.

Usage:
    python ble_temp_client.py [--scan-timeout <seconds>] [--debug]

Tips:
    - If you get a disconnect during service discovery, clear stale bonding info:
          bluetoothctl remove <addr>
      The Pico clears its own bond automatically on key mismatch.
    - The Pico advertises as "Pico <bdaddr>" not "secure_picow_temp"; the latter
      is the GATT device name only readable after connecting.
"""

import argparse
import asyncio
import logging
import struct
import sys

try:
    from bleak import BleakClient, BleakScanner
    from bleak.backends.characteristic import BleakGATTCharacteristic
except ImportError:
    print("ERROR: bleak is not installed.  Run:  pip install bleak")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Constants matching the Pico example
# ---------------------------------------------------------------------------

# Environmental Sensing Service (0x181A) and Temperature characteristic (0x2A6E)
ENVIRONMENTAL_SENSING_SERVICE_UUID = "0000181a-0000-1000-8000-00805f9b34fb"
TEMPERATURE_CHARACTERISTIC_UUID    = "00002a6e-0000-1000-8000-00805f9b34fb"

# Temperature is a signed 16-bit integer in units of 0.01 degC (Bluetooth SIG spec).
# The Pico stores current_temp = deg_c * 100 as uint16_t, which matches.
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

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def decode_temperature(data: bytes) -> float:
    """Decode a 2-byte little-endian temperature value (units: 0.01 degC)."""
    if len(data) != 2:
        raise ValueError(f"Expected 2 bytes for temperature, got {len(data)}")
    raw = struct.unpack_from("<h", data)[0]  # signed little-endian int16
    return raw / TEMP_SCALE


def notification_handler(characteristic: BleakGATTCharacteristic,
                          data: bytearray) -> None:
    """Called each time the server sends a temperature notification."""
    try:
        temp = decode_temperature(bytes(data))
        log.info("Temperature notification: %.2f degC  (raw: %s)", temp, data.hex())
    except ValueError as exc:
        log.error("Failed to decode notification: %s  raw=%s", exc, data.hex())


# ---------------------------------------------------------------------------
# Main client coroutine
# ---------------------------------------------------------------------------

async def run(timeout: float) -> None:
    # The Pico advertises the Environmental Sensing service UUID (0x181A).
    # The advertised name is "Pico <bdaddr>" so we match on service UUID,
    # with a name-prefix fallback in case BlueZ does not surface the UUID.
    log.info("Scanning for Environmental Sensing service (0x181A) or name prefix 'Pico '...")

    ESS_UUID_SHORT = "181a"

    def matches_pico(d, adv) -> bool:
        by_uuid = any(
            str(u).lower().replace("-", "").endswith(ESS_UUID_SHORT)
            for u in adv.service_uuids
        )
        by_name = (d.name or "").startswith("Pico ")
        if d.name or adv.service_uuids:
            log.debug("  Seen: %s  name=%r  uuids=%s  by_uuid=%s  by_name=%s",
                      d.address, d.name, [str(u) for u in adv.service_uuids],
                      by_uuid, by_name)
        return by_uuid or by_name

    device = await BleakScanner.find_device_by_filter(matches_pico, timeout=timeout)

    if device is None:
        log.error(
            "No device found within %.0f s.  Make sure the Pico W server is "
            "running and advertising.  Re-run with --debug to see all visible devices.",
            timeout,
        )
        return

    log.info("Found device: %s  [%s]", device.name, device.address)

    def disconnected_callback(client: BleakClient) -> None:
        log.warning("Device disconnected.")

    # pair_before_connect=True is essential: the BlueZ backend runs service
    # discovery automatically inside connect(), before we can call pair()
    # manually.  The Temperature characteristic requires ENCRYPTION_KEY_SIZE_16,
    # so the Pico rejects unencrypted ATT traffic and BlueZ drops the connection
    # mid-discovery unless pairing is completed first.
    log.info("Connecting and pairing...")
    async with BleakClient(device, timeout=10.0,
                           pair_before_connect=True,
                           disconnected_callback=disconnected_callback) as client:
        log.info("Connected and paired.")

        # Initial read to confirm the encrypted link is working before writing
        # the CCCD.  BTstack silently rejects ATT writes on insufficiently
        # secure links, so doing a read first ensures we are definitely encrypted.
        try:
            data = await client.read_gatt_char(TEMPERATURE_CHARACTERISTIC_UUID)
            log.info("Initial read: %.2f degC", decode_temperature(bytes(data)))
        except Exception as exc:
            log.warning("Initial read failed: %s", exc)

        log.info("Subscribing to temperature notifications...")
        await client.start_notify(TEMPERATURE_CHARACTERISTIC_UUID, notification_handler)
        log.info("Subscribed.  Listening for notifications - press Ctrl-C to stop.")

        try:
            while True:
                await asyncio.sleep(5)
                if not client.is_connected:
                    log.warning("Connection lost.")
                    break
        except asyncio.CancelledError:
            pass
        finally:
            if client.is_connected:
                await client.stop_notify(TEMPERATURE_CHARACTERISTIC_UUID)
            log.info("Unsubscribed and disconnecting.")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="BLE client for the secure_picow_temp example.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--scan-timeout", type=float, default=30.0,
        help="Seconds to scan before giving up.  Default: 30.",
    )
    parser.add_argument(
        "--debug", action="store_true",
        help="Print every BLE advertisement seen during the scan.",
    )
    args = parser.parse_args()

    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)

    try:
        asyncio.run(run(timeout=args.scan_timeout))
    except KeyboardInterrupt:
        log.info("Interrupted - goodbye.")


if __name__ == "__main__":
    main()