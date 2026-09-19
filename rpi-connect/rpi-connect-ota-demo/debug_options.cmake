# Rarely-used build-time debug overrides for rpi_connect_ota_demo.
#
# Under normal operation this is not set: the device-identity ECDSA key
# lives in OTP. The build-time PEM key only exists for debugging, and is
# included from CMakeLists.txt; it can be supplied via -D or an environment
# variable of the same name. The serial number always comes from the Pico's
# OTP unique board ID. WiFi credentials and auth tokens are never compiled
# in - provision them into FFS with tools/partition_pico2_for_ffs.sh instead.

# For debug, use build-time PEM files for the device-identity ECDSA key.
# Under normal operation the ECDSA private key is stored in OTP with the public key PEM
# file being generated at run-time.
if (DEFINED ENV{RPI_CONNECT_IDENTITY_PRIVKEY_PEM} AND (NOT RPI_CONNECT_IDENTITY_PRIVKEY_PEM))
    set(RPI_CONNECT_IDENTITY_PRIVKEY_PEM $ENV{RPI_CONNECT_IDENTITY_PRIVKEY_PEM})
endif()
if (DEFINED ENV{RPI_CONNECT_IDENTITY_PUBKEY_PEM} AND (NOT RPI_CONNECT_IDENTITY_PUBKEY_PEM))
    set(RPI_CONNECT_IDENTITY_PUBKEY_PEM $ENV{RPI_CONNECT_IDENTITY_PUBKEY_PEM})
endif()
if (RPI_CONNECT_IDENTITY_PRIVKEY_PEM AND RPI_CONNECT_IDENTITY_PUBKEY_PEM)
    # Extract raw P-256 private key scalar as hex string
    execute_process(
        COMMAND openssl ec -in "${RPI_CONNECT_IDENTITY_PRIVKEY_PEM}" -text -noout
        OUTPUT_VARIABLE _ec_text
        ERROR_QUIET
        RESULT_VARIABLE _ec_result
    )
    if (_ec_result EQUAL 0)
        string(FIND "${_ec_text}" "priv:" _priv_pos)
        string(FIND "${_ec_text}" "pub:" _pub_pos)
        math(EXPR _priv_start "${_priv_pos} + 5")
        math(EXPR _priv_len "${_pub_pos} - ${_priv_start}")
        string(SUBSTRING "${_ec_text}" ${_priv_start} ${_priv_len} _priv_hex_raw)
        string(REPLACE ":" "" _priv_hex "${_priv_hex_raw}")
        string(REPLACE " " "" _priv_hex "${_priv_hex}")
        string(REPLACE "\n" "" _priv_hex "${_priv_hex}")
        string(STRIP "${_priv_hex}" _priv_hex)

        # Read public key PEM and escape newlines for C string literal
        file(READ "${RPI_CONNECT_IDENTITY_PUBKEY_PEM}" _pubkey_pem)
        string(STRIP "${_pubkey_pem}" _pubkey_pem)
        string(REPLACE "\n" "\\n" _pubkey_c "${_pubkey_pem}")

        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/device_identity_keys.h"
            "// Auto-generated from PEM key files - do not edit\n"
            "#define RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX \"${_priv_hex}\"\n"
            "#define RPI_CONNECT_DEVICE_IDENTITY_PUBKEY_PEM \"${_pubkey_c}\\n\"\n"
        )
    else()
        message(WARNING "Failed to parse private key PEM: ${RPI_CONNECT_IDENTITY_PRIVKEY_PEM}")
    endif()
endif()
