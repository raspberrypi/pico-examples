# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/username/pico/pico-sdk")
set(PICO_EXTRAS_PATH "/home/username/pico/pico-extras")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)


if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wifi_vsys C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(wifi_vsys wifi_vsys.cpp )

pico_set_program_name(wifi_vsys "wifi_vsys")
pico_set_program_version(wifi_vsys "0.1")

pico_enable_stdio_uart(wifi_vsys 0)
pico_enable_stdio_usb(wifi_vsys 1)

# Add the standard library to the build
target_link_libraries(wifi_vsys
        pico_stdlib
        pico_lwip_http
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_adc
        )

# Add the standard include files to the build
target_include_directories(wifi_vsys PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

pico_add_extra_outputs(wifi_vsys)
