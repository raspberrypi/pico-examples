# Under normal operation PiConnect should always use the device-identity exchange
# API to obtain an auth token from the OTP private key. These build-time options
# allow the private keys to be overridden for debug - see debug_options.cmake.
include(${CMAKE_CURRENT_LIST_DIR}/debug_options.cmake)

if (PICO_ON_DEVICE)
    # Default to ${CMAKE_CURRENT_LIST_DIR}/updates, override by setting RPI_CONNECT_OTA_UPDATE_DIR
    if (DEFINED ENV{RPI_CONNECT_OTA_UPDATE_DIR} AND (NOT RPI_CONNECT_OTA_UPDATE_DIR))
        set(RPI_CONNECT_OTA_UPDATE_DIR "$ENV{RPI_CONNECT_OTA_UPDATE_DIR}")
        message("Using RPI_CONNECT_OTA_UPDATE_DIR from environment")
    endif()
    if (NOT RPI_CONNECT_OTA_UPDATE_DIR)
        set(RPI_CONNECT_OTA_UPDATE_DIR "${CMAKE_CURRENT_LIST_DIR}/updates")
    endif()
    set(RPI_CONNECT_OTA_UPDATE_DIR "${RPI_CONNECT_OTA_UPDATE_DIR}" CACHE INTERNAL "Directory to put rpi-connect-ota-demo update UF2s in")
endif()

# Image version for the picobin VERSION item (16-bit major.minor). The bootrom
# boots the highest-versioned valid image when choosing between the A/B
# partitions, so every OTA deployment must carry a strictly higher version than
# the fleet is running or the device reverts to the older image at the next
# power cycle. The commit count is monotonic per branch; CI can override with
# -DRPI_CONNECT_BUILD_NUMBER=<n>.
set(RPI_CONNECT_VERSION_MAJOR 1)
if (NOT RPI_CONNECT_BUILD_NUMBER)
    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE RPI_CONNECT_BUILD_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if (NOT RPI_CONNECT_BUILD_NUMBER)
    set(RPI_CONNECT_BUILD_NUMBER 0)
endif()

add_library(rpi_connect_ota_demo_libs INTERFACE)
target_sources(rpi_connect_ota_demo_libs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/rpi_connect_ota_demo.c
    )

# By default verification is optional (MBEDTLS_SSL_VERIFY_OPTIONAL)
# Specify UART DEBUG verbosity flags here
target_compile_definitions(rpi_connect_ota_demo_libs INTERFACE
    RPI_CONNECT_VERSION_MAJOR=${RPI_CONNECT_VERSION_MAJOR}
    RPI_CONNECT_BUILD_NUMBER=${RPI_CONNECT_BUILD_NUMBER}

    # --- Logging ----------------------------------------------------------
    # Default: show the flow of control (demo-app + library INFO and ERROR)
    # without low-level debug spam. The libraries log nothing by default.
    RPI_CONNECT_ERROR_ENABLE=1
    RPI_CONNECT_OTA_ERROR_ENABLE=1
    RPI_CONNECT_INFO_ENABLE=1
    RPI_CONNECT_OTA_INFO_ENABLE=1

    # For data-limited or metered connections: check once for a pending deployment
    # after boot sync instead of holding the SSE event stream open.
    # RPI_CONNECT_OTA_DEMO_POLL_ONLY=1
    # If artefacts are hosted on a server that does not chain to the
    # Let's Encrypt X2 root CA, disable TLS verification for the download only;
    # integrity then rests on the deployment's SHA-256 checksum.
    # RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD=1
    # If artefacts are hosted on a server that uses RSA keys instead of ECC ones,
    # add RSA support to the default mbedtls_config.h file, and use the
    # Let's Encrypt X1 root CA, which is RSA.
    # RPI_CONNECT_OTA_CONFIG_SUPPORT_RSA=1
    # If artefacts are hosted on GitHub, you can add these certificates for
    # verified downloads - also requires RPI_CONNECT_OTA_CONFIG_SUPPORT_RSA=1
    # because githubusercontent.com uses RSA certificates.
    # RPI_CONNECT_OTA_DEMO_ADD_GITHUB_CERTS=1
    # For debugging, uncomment any of these to enable verbose logging in the
    # libraries (or per-chunk progress etc. in the demo app):
    #   RPI_CONNECT_DEBUG_ENABLE=1              # core rpi_connect debug (incl. HTTP request layer)
    #   RPI_CONNECT_VERBOSE_DEBUG_ENABLE=1      # ...plus header/body dumps
    #   RPI_CONNECT_OTA_DEBUG_ENABLE=1          # OTA library debug
    #   HTTPC_DEBUG=LWIP_DBG_ON                 # lwIP HTTP client debug
    #   RPI_CONNECT_OTA_DEMO_DEBUG_ENABLE=1     # demo-app DEBUG (download progress)
    )

if (PICO_ON_DEVICE)
    target_compile_definitions(rpi_connect_ota_demo_libs INTERFACE
        # User-Agent for all requests from the lwIP HTTP client
        HTTPC_CLIENT_AGENT=\"rpi-connect-pico/${PICO_SDK_VERSION_MAJOR}.${PICO_SDK_VERSION_MINOR}\"
        ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_OPTIONAL
        RPI_CONNECT_MBEDTLS_DEBUG_LEVEL=0      # mbedTLS debug verbosity (0=off, up to 4)

        # The TLS handshake nests under the synchronous request path; the 2K
        # default overflows.
        PICO_STACK_SIZE=0x1000
        )
endif()

# The bash test suite builds the host tool with these.
if (NOT PICO_ON_DEVICE)
    if (RPI_CONNECT_OTA_DEMO_POLL_ONLY)
        target_compile_definitions(rpi_connect_ota_demo_libs INTERFACE
            RPI_CONNECT_OTA_DEMO_POLL_ONLY=1
            )
    endif()
    option(RPI_CONNECT_OTA_DEMO_ASAN "Build the host demo with AddressSanitizer" OFF)
    if (RPI_CONNECT_OTA_DEMO_ASAN)
        target_compile_options(rpi_connect_ota_demo_libs INTERFACE -fsanitize=address)
        target_link_options(rpi_connect_ota_demo_libs INTERFACE -fsanitize=address -static-libasan)
    endif()
endif()

target_include_directories(rpi_connect_ota_demo_libs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

target_link_libraries(rpi_connect_ota_demo_libs INTERFACE
    pico_rpi_connect
    pico_rpi_connect_ota
    pico_stdlib
    )
if (PICO_ON_DEVICE)
    # Polled network stack: TLS and lwIP run in thread context from
    # async_context_poll(), not in the low-priority IRQ.
    target_link_libraries(rpi_connect_ota_demo_libs INTERFACE
        pico_cyw43_arch_lwip_poll

        pico_cjson
        pico_ffs
        pico_flash_image
        pico_lwip_mbedtls
        pico_mbedtls
        pico_rpi_connect_ota_default_config_headers
        )
endif()

function(finalise_rpi_connect_ota_demo TARGET)
    target_link_libraries(${TARGET} rpi_connect_ota_demo_libs)
    # Program version reported at boot and by picotool info. The picobin image
    # VERSION used for bootrom A/B selection is set separately below from
    # RPI_CONNECT_VERSION_MAJOR / RPI_CONNECT_BUILD_NUMBER.
    pico_set_program_version(${TARGET} "1.0.0")

    # Embed the VERSION item (via crt0, so it lives in the first block alongside
    # the IMAGE_DEF) and seal the output with a HASH_DEF: the bootrom then
    # verifies the whole image during A/B selection, rejecting partially written
    # updates, and deterministically boots the newest valid image. Must be set
    # before pico_add_extra_outputs(), which configures the picotool seal step.
    # The UF2 this build produces is the artefact to provision and to deploy via
    # Connect - it does not need re-sealing downstream.
    pico_set_binary_version(${TARGET}
        MAJOR ${RPI_CONNECT_VERSION_MAJOR}
        MINOR ${RPI_CONNECT_BUILD_NUMBER}
        )
    pico_hash_binary(${TARGET})

    # add url via pico_set_program_url
    example_auto_set_url(${TARGET})

    # seal the binary and add extra outputs
    pico_add_extra_outputs(${TARGET})
endfunction()

if (NOT PICO_ON_DEVICE)
    # Host build: the registration/debug CLI (main.c) driving the same OTA
    # app loop over the SDK host builds of the Pi Connect libraries.
    add_executable(rpi_connect_ota_demo
        main.c
        )
    target_link_libraries(rpi_connect_ota_demo rpi_connect_ota_demo_libs)

    example_auto_set_url(rpi_connect_ota_demo)
endif()

if (PICO_ON_DEVICE)
    # Add base version
    add_executable(rpi_connect_ota_demo
        main_pico.c
        )
    # Common finalising for the demo
    finalise_rpi_connect_ota_demo(rpi_connect_ota_demo)
    # Add combined output for easy flashing
    pico_add_rpi_connect_ota_outputs(rpi_connect_ota_demo default
        WIFI_SSID ${WIFI_SSID} WIFI_PASSWORD ${WIFI_PASSWORD}
        )

    # Add bootsel reset version
    add_executable(rpi_connect_ota_bootsel_demo
        main_pico.c
        )
    target_compile_definitions(rpi_connect_ota_bootsel_demo PRIVATE
        # Exit immediately
        EXIT_IMMEDIATELY=1
        # Reset to BOOTSEL on exit
        PICO_ENTER_USB_BOOT_ON_EXIT=1
        )
    # Common finalising for the demo
    finalise_rpi_connect_ota_demo(rpi_connect_ota_bootsel_demo)
    pico_add_rpi_connect_ota_outputs(rpi_connect_ota_bootsel_demo default
        WIFI_SSID ${WIFI_SSID} WIFI_PASSWORD ${WIFI_PASSWORD}
        )

    if (PICO_RP2040)
        message("Skipping rpi_connect_ota_packaged_demo example which is unsupported on this platform")
    else()
        # Add packaged no_flash variant
        add_executable(rpi_connect_ota_packaged_demo
            main_pico.c
            )
        # Packaged no_flash binary
        pico_set_binary_type(rpi_connect_ota_packaged_demo no_flash)
        pico_package_uf2_output(rpi_connect_ota_packaged_demo)
        # Optionally sign the no_flash binary with your private key, if using secure boot
        pico_sign_binary(rpi_connect_ota_packaged_demo ${CMAKE_CURRENT_LIST_DIR}/private.pem)
        # Common finalising for the demo
        finalise_rpi_connect_ota_demo(rpi_connect_ota_packaged_demo)
        # Add combined output for easy flashing
        pico_add_rpi_connect_ota_outputs(rpi_connect_ota_packaged_demo default
            WIFI_SSID ${WIFI_SSID} WIFI_PASSWORD ${WIFI_PASSWORD}
            )
    endif()
endif()

if (PICO_ON_DEVICE)
    # Ignore warnings from lwip code
    set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
        )

    # Add convenience target to build all demos
    add_custom_target(rpi_connect_ota_demos)
    get_directory_property(targets BUILDSYSTEM_TARGETS DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    foreach(target IN LISTS targets)
        if (${target} MATCHES "rpi_connect_ota_.*demo_combined")
            add_dependencies(rpi_connect_ota_demos ${target})
        endif()
    endforeach()
endif()
