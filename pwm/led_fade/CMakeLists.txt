cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(led_fade C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(pwm_led_fade
        pwm_led_fade.c
        )

# pull in common dependencies and additional pwm hardware support
target_link_libraries(pwm_led_fade pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_led_fade)

# add url via pico_set_program_url
example_auto_set_url(pwm_led_fade)
