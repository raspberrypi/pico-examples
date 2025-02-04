cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(hello_dcp C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hello_dcp
        hello_dcp.c
        dcp_examples.S
        )

# pull in common dependencies
target_link_libraries(hello_dcp pico_stdlib)

# we need a high-precision printf to demonstrate dotx properly
pico_set_printf_implementation(hello_dcp compiler)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_dcp)

# add url via pico_set_program_url
example_auto_set_url(hello_dcp)