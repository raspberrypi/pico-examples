cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(hello_gpio_irq C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hello_gpio_irq
        hello_gpio_irq.c
        )

# pull in common dependencies
target_link_libraries(hello_gpio_irq pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_gpio_irq)

# add url via pico_set_program_url
example_auto_set_url(hello_gpio_irq)
