#!/usr/bin/env python3
"""Send Wi-Fi credentials to a Pico W running the ble_wifi_provisioner example.

This script connects to the Pico over BLE, bonds with it, and writes the SSID
and password to the provisioning characteristics over an encrypted link.

Requires the 'bleak' BLE library:

    pip install bleak

(On some systems use 'pip3', or 'python3 -m pip install bleak'. A virtual
environment is recommended to avoid touching the system Python.)

Usage:

    ./set_credentials.py <ssid> <password> <address>

For example:

    ./set_credentials.py MyNetwork hunter2 2C:CF:67:BE:08:05
"""
import asyncio
import sys
import argparse

from bleak import BleakClient
from bleak.exc import BleakError

SSID_CHARACTERISTIC = "b1829813-e8ec-4621-b9b5-6c1be43fe223"
PASSWORD_CHARACTERISTIC = "410f5077-9e81-4f3b-b888-bf435174fa58"

# Add arguments from terminal with: python3 set_credentials.py ssid password address
parser = argparse.ArgumentParser(description="ssid, password and address parser")
parser.add_argument("ssid")
parser.add_argument("password")
parser.add_argument("address")
args = parser.parse_args()

ssid = args.ssid
password = args.password
address = args.address

print("submitted ssid: ", ssid)
print("submitted password: ", password)
print("submitted address: ", address)


def on_disconnect(client):
    print("Disconnected from device.")


async def ensure_paired(client):
    """Establish an encrypted/bonded link before writing.

    The SSID/password characteristics require an encrypted link
    (ENCRYPTION_KEY_SIZE_16), and backends satisfy that differently:

      - WinRT (native Windows) does NOT auto-pair when an encryption-required
        write arrives; the write is rejected and the link may even drop. So on
        Windows we must pair() up front, before writing.
      - BlueZ (Linux / Raspberry Pi) usually pairs transparently on the write,
        and a direct pair() call can be unreliable on some versions.

    Pairing up front works on Windows and is harmless on Linux, so we attempt
    it and tolerate it raising or being a no-op (e.g. already bonded). If the
    backend really needs on-demand pairing instead, the subsequent write still
    triggers it.
    """
    try:
        print("Pairing / bonding...")
        await client.pair()
    except BleakError as e:
        # Already bonded, or this backend pairs on demand at write time.
        print(f"pair() reported: {e!r} (continuing)")


async def main(ssid, password, address):
    # A disconnected callback helps surface link drops during pairing.
    client = BleakClient(address, disconnected_callback=on_disconnect)

    await client.connect()
    print(f"Connected: {client.is_connected}")

    try:
        await ensure_paired(client)

        print("Writing SSID...")
        await client.write_gatt_char(SSID_CHARACTERISTIC, ssid.encode("utf-8"), response=True)
        await asyncio.sleep(1.0)

        print("Writing password...")
        await client.write_gatt_char(PASSWORD_CHARACTERISTIC, password.encode("utf-8"), response=True)
        await asyncio.sleep(1.0)

        print("Credentials written successfully.")

    finally:
        if client.is_connected:
            await client.disconnect()



if __name__ == "__main__":
    try:
        asyncio.run(main(ssid, password, address))
    except BleakError as e:
        print(f"BLE error: {e}", file=sys.stderr)
        sys.exit(1)