#!/bin/sh

set -e
set -u

# Partition an RP2350 (Pico 2) for ffs and program a firmware image.
#
# Optionally provisions WiFi credentials directly into the ffs partition. This
# is deliberately kept separate from the firmware UF2: the credentials are
# written into flash as ffs files and are NEVER baked into the firmware image,
# which is more secure than compiling credentials into the binary.
#
# The WiFi credentials are stored under fixed ffs file IDs that the application
# reads at runtime (see RPI_CONNECT_FFS_WIFI_SSID / RPI_CONNECT_FFS_WIFI_PASSWORD
# in main_pico.c). The application prefers FFS-provisioned credentials over any
# built into the firmware.
#
# Requires: picotool, a C compiler (cc), and the Pico SDK sources (for the
# ffsgen host tool). The SDK is located via PICO_SDK_PATH, or relative to this
# script when it is run from within the SDK tree.

# ffs file IDs - MUST match the application (main_pico.c) and the OTA library
# (rpi_connect_ota.h)
WIFI_SSID_FID=16        # 0x10  RPI_CONNECT_FFS_WIFI_SSID
WIFI_PASSWORD_FID=17    # 0x11  RPI_CONNECT_FFS_WIFI_PASSWORD
AUTH_TOKEN_FID=1        # 0x01  RPI_CONNECT_FFS_AUTH_TOKEN

# ffs chunk size. The ffs partition (see ffs_pt.json) is 8K = two 4K chunks;
# the generated blob is a single (active) chunk programmed at the partition
# start. Override with FFS_CHUNK_SIZE if the partition size changes.
FFS_CHUNK_SIZE="${FFS_CHUNK_SIZE:-4096}"

TMP_DIR=

cleanup() {
   if [ -d "${TMP_DIR}" ]; then
      rm -rf "${TMP_DIR}"
   fi
}

trap cleanup EXIT

script_dir=$(cd "$(dirname "$0")" && pwd)

usage() {
   cat <<EOF
Usage: $(basename "$0") [options] <firmware.uf2>

Partitions the device for ffs, loads the partition table, then programs the
given firmware image.

Options:
  --wifi-ssid <SSID>          Provision this WiFi SSID into the ffs partition
  --wifi-password <PASSWORD>  Provision this WiFi password into the ffs partition
  --connect-token <TOKEN>     Provision this Connect auth token into the ffs
                              partition (debug; normally the device derives a
                              token from its OTP identity key)
  -h, --help                  Show this help

WiFi credentials and the Connect token may also be supplied via the WIFI_SSID,
WIFI_PASSWORD and RPI_CONNECT_TOKEN environment variables (preferred, as they
then do not appear in the process arguments). Command-line options take
precedence over the environment.

Credentials are written to the ffs partition only and are NEVER included in
the firmware UF2.
EOF
}

# Locate the SDK root via PICO_SDK_PATH (needed to build the ffsgen host
# tool from its standalone CMake project in tools/ffsgen).
find_sdk_root() {
   for base in "${PICO_SDK_PATH:-}"; do
      [ -n "${base}" ] || continue
      if [ -f "${base}/tools/ffsgen/CMakeLists.txt" ]; then
         (cd "${base}" && pwd)
         return 0
      fi
   done
   return 1
}

# Credentials default to the environment (consistent with the firmware build),
# and can be overridden on the command line.
wifi_ssid="${WIFI_SSID:-}"
wifi_password="${WIFI_PASSWORD:-}"
connect_token="${RPI_CONNECT_TOKEN:-}"
firmware=
sleep_time=2

while [ $# -gt 0 ]; do
   case "$1" in
      --wifi-ssid)
         [ $# -ge 2 ] || { echo "error - --wifi-ssid requires an argument" >&2; exit 1; }
         wifi_ssid="$2"
         shift 2
         ;;
      --wifi-password)
         [ $# -ge 2 ] || { echo "error - --wifi-password requires an argument" >&2; exit 1; }
         wifi_password="$2"
         shift 2
         ;;
      --connect-token)
         [ $# -ge 2 ] || { echo "error - --connect-token requires an argument" >&2; exit 1; }
         connect_token="$2"
         shift 2
         ;;
      --sleep-time)
         [ $# -ge 2 ] || { echo "error - --sleep-time requires an argument" >&2; exit 1; }
         sleep_time="$2"
         shift 2
         ;;
      -h|--help)
         usage
         exit 0
         ;;
      --)
         shift
         break
         ;;
      -*)
         echo "error - unknown option: $1" >&2
         usage >&2
         exit 1
         ;;
      *)
         break
         ;;
   esac
done

if [ $# -lt 1 ]; then
   echo "error - no firmware image specified" >&2
   usage >&2
   exit 1
fi
firmware="$1"

if [ ! -f "${firmware}" ]; then
   echo "error - firmware image '${firmware}' not found" >&2
   exit 1
fi

PARTTION_FILES="ffs_pt"
TMP_DIR=$(mktemp -d)

# Build the ffs partition blob containing the WiFi credentials and/or Connect
# token (if supplied).
# Returns via the global ffs_blob_uf2, left empty when nothing was provisioned.
ffs_blob_uf2=
build_ffs_blob() {
   if [ -z "${wifi_ssid}" ] && [ -z "${connect_token}" ]; then
      echo "info - no WiFi credentials or Connect token supplied, ffs partition left empty"
      return 0
   fi

   sdk_root=$(find_sdk_root) || {
      echo "error - cannot locate ffsgen sources; set PICO_SDK_PATH" >&2
      exit 1
   }
   ffsgen_src="${sdk_root}/tools/ffsgen"

   echo "info - building ffsgen tool"
   cmake -S "${ffsgen_src}" -B "${TMP_DIR}/ffsgen-build" > /dev/null
   cmake --build "${TMP_DIR}/ffsgen-build" > /dev/null
   ffsgen="${TMP_DIR}/ffsgen-build/ffsgen"

   # Write the credentials to temp files with restricted permissions. Held only
   # in the temp dir, which is removed on exit. A trailing NUL is appended so
   # each stored ffs file is a NUL-terminated C string: the readers use
   # strdup(), matching how rpi_connect_ota stores strings (strlen + 1).
   # Values are always passed via %s so a '%' in a credential is safe.
   echo "# Auto-generated ffs provisioning config" > "${TMP_DIR}/ffs.cfg"
   (
      umask 077
      if [ -n "${wifi_ssid}" ]; then
         { printf '%s' "${wifi_ssid}";     printf '\000'; } > "${TMP_DIR}/wifi_ssid"
         { printf '%s' "${wifi_password}"; printf '\000'; } > "${TMP_DIR}/wifi_password"
         echo "${WIFI_SSID_FID} ${TMP_DIR}/wifi_ssid" >> "${TMP_DIR}/ffs.cfg"
         echo "${WIFI_PASSWORD_FID} ${TMP_DIR}/wifi_password" >> "${TMP_DIR}/ffs.cfg"
         echo "info - provisioning WiFi SSID '${wifi_ssid}' into ffs partition"
      fi
      if [ -n "${connect_token}" ]; then
         { printf '%s' "${connect_token}"; printf '\000'; } > "${TMP_DIR}/connect_token"
         echo "${AUTH_TOKEN_FID} ${TMP_DIR}/connect_token" >> "${TMP_DIR}/ffs.cfg"
         echo "info - provisioning Connect auth token into ffs partition"
      fi
   )

   "${ffsgen}" "${TMP_DIR}/ffs.cfg" "${TMP_DIR}/ffs_blob.bin" "${FFS_CHUNK_SIZE}"

   # Wrap the raw blob in a 'data' family UF2 so picotool routes it to the ffs
   # (data) partition, the same way the partition table and firmware UF2s load.
   picotool uf2 convert "${TMP_DIR}/ffs_blob.bin" "${TMP_DIR}/ffs_blob.uf2" \
      --family data
   ffs_blob_uf2="${TMP_DIR}/ffs_blob.uf2"
}

# The firmware must already be sealed with a HASH_DEF (the
# rpi_connect_ota_demo build does this via pico_hash_binary). Without a hash
# the bootrom only checks that the IMAGE_DEF parses, so a partially written
# image (e.g. an OTA update interrupted by a reset) can win A/B partition
# selection over the intact image and brick the board until it is reflashed.
# OTA update artefacts deployed via Connect must be sealed the same way.
check_firmware_sealed() {
   if ! picotool info -a "${firmware}" 2>/dev/null | grep -q "hash:.*verified"; then
      echo "error - firmware image '${firmware}' has no verified HASH_DEF" >&2
      echo "        build with pico_hash_binary(), see rpi-connect-ota-demo/CMakeLists.txt" >&2
      exit 1
   fi
}

# Check the image and generate the credentials blob up front so we fail fast
# on any config error, before touching the device.
check_firmware_sealed
build_ffs_blob

picotool reboot -uf
sleep $sleep_time

# Workaround lack of E10 errata handling in picotool:
# Need to request an `info` before requesting the erase!
picotool info
picotool erase --range 0x10000000 0x10400000

picotool reboot -u
sleep $sleep_time

picotool partition create "${script_dir}/$PARTTION_FILES.json" "${TMP_DIR}/$PARTTION_FILES.uf2"
picotool load -v "${TMP_DIR}/$PARTTION_FILES.uf2"

# Essential to ensure the partition table is loaded
picotool reboot -u
sleep $sleep_time

# Display the table!
picotool partition info

# Provision the WiFi credentials into the ffs (data) partition, if supplied.
# This is loaded separately from the firmware so the credentials never appear
# in the firmware UF2.
if [ -n "${ffs_blob_uf2}" ]; then
   echo "info - loading WiFi credentials into ffs partition"
   picotool load -v "${ffs_blob_uf2}"
   picotool reboot -u
   sleep $sleep_time
fi

# And then to program your executable
picotool load "${firmware}"
