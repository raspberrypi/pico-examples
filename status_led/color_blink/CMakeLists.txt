# Blink the colored "status" LED connected to the GPIO defined by PICO_DEFAULT_WS2812_PIN for your board
add_executable(color_blink
    color_blink.c
)
# You can define PICO_DEFAULT_WS2812_PIN yourself to add a WS2812 led to a normal GPIO
#target_compile_definitions(color_blink PRIVATE
#   PICO_DEFAULT_WS2812_PIN=16
#)
target_link_libraries(color_blink
    pico_stdlib
    pico_status_led
)
pico_add_extra_outputs(color_blink)
example_auto_set_url(color_blink)
