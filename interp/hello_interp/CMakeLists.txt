cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(hello_interp C CXX ASM)
    include(../../CMakeLists.txt)
endif()

if (TARGET hardware_interp)
    add_executable(hello_interp
            hello_interp.c
            )

    # pull in common dependencies and additional interpolator hardware support
    target_link_libraries(hello_interp pico_stdlib hardware_interp)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(hello_interp)

    # add url via pico_set_program_url
    example_auto_set_url(hello_interp)
endif ()
