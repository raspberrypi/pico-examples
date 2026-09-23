# Under normal operation PiConnect should always use the device-identity exchange
# API to obtain an auth token from the OTP private key. These build-time options
# allow the private keys (and WiFi creds / serial) to be overridden for debug -
# see debug_options.cmake. Auth tokens are never compiled into the image; for
# development, provision one into FFS with partition_pico2_for_ffs.sh
# --connect-token.
include(${CMAKE_CURRENT_LIST_DIR}/debug_options.cmake)

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

if (PICO_ON_DEVICE)
    add_executable(rpi_connect_ota_demo
        main_pico.c
        )
    target_link_libraries(rpi_connect_ota_demo rpi_connect_ota_demo_libs)
else()
    # Host build: the registration/debug CLI (main.c) driving the same OTA
    # app loop over the SDK host builds of the Pi Connect libraries.
    add_executable(rpi_connect_ota_demo
        main.c
        )
    target_link_libraries(rpi_connect_ota_demo rpi_connect_ota_demo_libs)
endif()

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
    # If artefacts are hosted on a server that does not chain to the Pi
    # Connect root CA, disable TLS verification for the download only;
    # integrity then rests on the deployment's SHA-256 checksum.
    # RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD=1
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
        )
endif()

if (PICO_ON_DEVICE)
    # Program version reported at boot and by picotool info. The picobin image
    # VERSION used for bootrom A/B selection is set separately below from
    # RPI_CONNECT_VERSION_MAJOR / RPI_CONNECT_BUILD_NUMBER.
    pico_set_program_version(rpi_connect_ota_demo "1.0.0")

    # Embed the VERSION item (via crt0, so it lives in the first block alongside
    # the IMAGE_DEF) and seal the output with a HASH_DEF: the bootrom then
    # verifies the whole image during A/B selection, rejecting partially written
    # updates, and deterministically boots the newest valid image. Must be set
    # before pico_add_extra_outputs(), which configures the picotool seal step.
    # The UF2 this build produces is the artefact to provision and to deploy via
    # Connect - it does not need re-sealing downstream.
    pico_set_binary_version(rpi_connect_ota_demo
        MAJOR ${RPI_CONNECT_VERSION_MAJOR}
        MINOR ${RPI_CONNECT_BUILD_NUMBER}
        )
    pico_hash_binary(rpi_connect_ota_demo)

    pico_add_extra_outputs(rpi_connect_ota_demo)
endif()

example_auto_set_url(rpi_connect_ota_demo)

if (PICO_ON_DEVICE)
    # Ignore warnings from lwip code
    set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
        )
endif()
