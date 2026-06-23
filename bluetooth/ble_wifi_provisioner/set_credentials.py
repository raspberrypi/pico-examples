import asyncio
import sys
import argparse

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

SSID_CHARACTERISTIC = "b1829813-e8ec-4621-b9b5-6c1be43fe223"
PASSWORD_CHARACTERISTIC = "410f5077-9e81-4f3b-b888-bf435174fa58"

# Add arguments from terminal with: python3 set_credentials.py ssid password address
parser = argparse.ArgumentParser(description="ssid, password and address parser")
parser.add_argument("ssid")
parser.add_argument("password")
parser.add_argument("address")
parser.add_argument(
    "--no-bond",
    action="store_true",
    help="Skip explicit pairing/bonding (e.g. if already bonded at the OS level)",
)
args = parser.parse_args()

ssid = args.ssid
password = args.password
address = args.address

print("submitted ssid: ", ssid)
print("submitted password: ", password)
print("submitted address: ", address)


def on_disconnect(client):
    print("Disconnected from device.")


async def main(ssid, password, address, do_bond):
    # A disconnected callback helps surface link drops during pairing.
    client = BleakClient(address, disconnected_callback=on_disconnect)

    await client.connect()
    print(f"Connected: {client.is_connected}")

    try:
        if do_bond:
            # Request pairing with bonding. On supported backends bleak stores
            # the bond keys, so subsequent connections reuse the existing bond
            # instead of pairing again. If a bond already exists this is a no-op
            # on most backends (and raises on some), so we tolerate failure.
            try:
                print("Pairing / bonding...")
                paired = await client.pair()
                print(f"Paired: {paired}")
            except BleakError as e:
                # Already bonded, or backend reports pairing not needed.
                print(f"Pairing reported: {e!r} (continuing, may already be bonded)")

        # The credential characteristics require an encrypted link. Writing
        # before encryption is established will fail, so ensure the link is up.
        # write_gatt_char with response=True will block until the peer ACKs,
        # which on an encryption-required characteristic only succeeds once the
        # bonded/encrypted link is active.
        print("Writing SSID...")
        await client.write_gatt_char(
            SSID_CHARACTERISTIC, ssid.encode("utf-8"), response=True
        )
        await asyncio.sleep(1.0)

        print("Writing password...")
        await client.write_gatt_char(
            PASSWORD_CHARACTERISTIC, password.encode("utf-8"), response=True
        )
        await asyncio.sleep(1.0)

        print("Credentials written successfully.")

    finally:
        if client.is_connected:
            await client.disconnect()


if __name__ == "__main__":
    try:
        asyncio.run(main(ssid, password, address, do_bond=not args.no_bond))
    except BleakError as e:
        print(f"BLE error: {e}", file=sys.stderr)
        sys.exit(1)