# Blink the "status" LED either connected to the GPIO defined by PICO_DEFAULT_LED_PIN for your board
# or via the WiFi chip on boards like Pico 2 or Pico 2 W

add_executable(status_blink
    status_blink.c
)
# You can define PICO_DEFAULT_LED_PIN yourself to add a led to a different GPIO
#target_compile_definitions(status_blink PRIVATE
#   PICO_DEFAULT_LED_PIN=15
#)
target_link_libraries(status_blink
    pico_stdlib
    pico_status_led
)
pico_add_extra_outputs(status_blink)
example_auto_set_url(status_blink)
