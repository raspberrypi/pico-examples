# This wrapper CMakeLists.txt file is required to add the top-level CMakeLists.txt
# setup for each build. This is very similar to the top-level CMakeLists.txt file
# in pico-examples, with some added bits to ensure it can be turned into a universal
# binary.
cmake_minimum_required(VERSION 3.12)

# Setup toolchain and compiler
if (PICO_PLATFORM MATCHES riscv)
    set(PICO_TOOLCHAIN_PATH ${PICO_RISCV_TOOLCHAIN_PATH})
else()
    set(PICO_TOOLCHAIN_PATH ${PICO_ARM_TOOLCHAIN_PATH})
endif()

# Clear any environment variables that may break the build
if (DEFINED ENV{PICO_BOARD})
    message(WARNING "Unsetting environment variable PICO_BOARD=$ENV{PICO_BOARD} for universal builds")
    unset(ENV{PICO_BOARD})
endif()

if (DEFINED ENV{PICO_GCC_TRIPLE})
    message(WARNING "Unsetting environment variable PICO_GCC_TRIPLE=$ENV{PICO_GCC_TRIPLE} for universal builds")
    unset(ENV{PICO_GCC_TRIPLE})
endif()

# Set PICO_BOARD if defined
if (PICO_BOARD_RP2040 AND (PICO_PLATFORM MATCHES rp2040))
    set(PICO_BOARD ${PICO_BOARD_RP2040})
elseif(PICO_BOARD_RP2350 AND (PICO_PLATFORM MATCHES rp2350))
    set(PICO_BOARD ${PICO_BOARD_RP2350})
endif()

# Pull in SDK (must be before project)
include(${PICO_EXAMPLES_PATH}/pico_sdk_import.cmake)

project(universal_wrapper C CXX ASM)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

include(${PICO_EXAMPLES_PATH}/example_auto_set_url.cmake)

# Ensure a picobin block is present, even on RP2040, so it can be linked into the block loop
target_compile_definitions(pico_crt0 INTERFACE PICO_CRT0_INCLUDE_PICOBIN_BLOCK=1)

# Redefine this to ensure no_flash binaries are packaged
function(pico_add_extra_outputs TARGET)
    if (NOT (PICO_PLATFORM MATCHES rp2040))
        get_target_property(BINARY_TYPE ${SOURCE_TARGET} PICO_TARGET_BINARY_TYPE)
        if (${BINARY_TYPE} STREQUAL "no_flash")
            message("Packaging no_flash universal binary to SRAM, so it has a load_map")
            pico_package_uf2_output(${SOURCE_TARGET} 0x20000000)
        endif()
    endif()
    _pico_add_extra_outputs(${SOURCE_TARGET})
endfunction()

# Build the binary
add_subdirectory(${UNIVERSAL_PROJECT_DIR} ${UNIVERSAL_BINARY_DIR})
