add_executable(hello_pio)

pico_generate_pio_header(hello_pio ${CMAKE_CURRENT_LIST_DIR}/hello.pio)

target_sources(hello_pio PRIVATE hello.c)

target_link_libraries(hello_pio PRIVATE
        pico_stdlib
        hardware_pio
        )

# Pass cmake -DHELLO_PIO_LED_PIN=x, where x is the pin you want to use
if(HELLO_PIO_LED_PIN)
        target_compile_definitions(hello_pio PRIVATE
                HELLO_PIO_LED_PIN=${HELLO_PIO_LED_PIN}
        )
endif()

pico_add_extra_outputs(hello_pio)

# add url via pico_set_program_url
example_auto_set_url(hello_pio)
