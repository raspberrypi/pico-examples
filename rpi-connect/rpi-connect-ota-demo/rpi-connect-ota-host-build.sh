#!/bin/sh

cmake -S $PICO_EXAMPLES_PATH -B $PICO_EXAMPLES_PATH/build-host -DPICO_PLATFORM=host -DPICO_BOARD=none
cmake --build $PICO_EXAMPLES_PATH/build-host --target rpi_connect_ota_demo
