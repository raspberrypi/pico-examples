#!/bin/bash
# Write a P-256 ECDSA private key to RP2350 OTP page 3 (row 0xc0).
#
# Usage: write_identity_key_otp.sh [--write] [--row <row>] <private-key.pem>
#
# By default runs in dry-run mode. Pass --write to program OTP.
# --row sets the starting OTP row (default 0xc0 = page 3).
#
# The 32-byte raw scalar is extracted from the PEM file with openssl,
# packed into a raw OTP binary (4 bytes per row, lower 3 used), and
# written to OTP starting at row 0xc0 using picotool.

set -euo pipefail

WRITE=0
ROW=0xc0
while [ $# -gt 0 ]; do
    case "$1" in
        --write) WRITE=1; shift ;;
        --row) ROW="$2"; shift 2 ;;
        -*) echo "Unknown option: $1" >&2; exit 1 ;;
        *) break ;;
    esac
done

if [ $# -ne 1 ]; then
    echo "Usage: $0 [--write] [--row <row>] <private-key.pem>" >&2
    exit 1
fi

PEM_FILE="$1"

if [ ! -f "$PEM_FILE" ]; then
    echo "Error: file not found: $PEM_FILE" >&2
    exit 1
fi

# Extract the 32-byte raw private key scalar
PRIV_HEX=$(openssl ec -in "$PEM_FILE" -text -noout 2>/dev/null \
    | sed -n '/priv:/,/pub:/p' \
    | grep -oE '[0-9a-f]{2}(:[0-9a-f]{2})*' \
    | tr -d ':\n ')

if [ ${#PRIV_HEX} -ne 64 ]; then
    echo "Error: expected 32-byte (64 hex char) P-256 scalar, got ${#PRIV_HEX} hex chars" >&2
    exit 1
fi

echo "Private key scalar: $PRIV_HEX"

# Build raw OTP binary: 4 bytes per row, 3 bytes of data per row (little-endian).
# 32 bytes = 11 rows (last row has 2 bytes of key + 1 zero-pad byte).
TMPBIN=$(mktemp /tmp/otp_key.XXXXXX.bin)
trap 'rm -f "$TMPBIN"' EXIT

python3 -c "
import sys, struct
key = bytes.fromhex('$PRIV_HEX')
with open('$TMPBIN', 'wb') as f:
    for i in range(0, len(key), 3):
        chunk = key[i:i+3]
        # Pad last chunk to 3 bytes
        chunk = chunk.ljust(3, b'\x00')
        # Raw OTP row: 4 bytes little-endian, upper byte ignored by picotool
        f.write(struct.pack('<I', chunk[0] | (chunk[1] << 8) | (chunk[2] << 16)))
"

ROW_COUNT=$(( ($(wc -c < "$TMPBIN") ) / 4 ))

if [ "$WRITE" -eq 1 ]; then
    echo "Writing $ROW_COUNT OTP rows starting at row $ROW"
    picotool otp load -r -s "$ROW" "$TMPBIN"
    echo "Done."
else
    echo "DRY RUN: would write $ROW_COUNT OTP rows starting at row $ROW"
    echo "Re-run with --write to program OTP."
fi
