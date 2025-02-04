cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(hello_rtc C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hello_rtc
        hello_rtc.c
        )

# pull in common dependencies and additional rtc hardware support
target_link_libraries(hello_rtc pico_stdlib hardware_rtc)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_rtc)

# add url via pico_set_program_url
example_auto_set_url(hello_rtc)
