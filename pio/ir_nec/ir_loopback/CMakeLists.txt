add_executable (pio_ir_loopback ir_loopback.c)

# link the executable using the IR transmit and receive libraries
#
target_link_libraries(pio_ir_loopback LINK_PUBLIC
  pico_stdlib
  hardware_pio
  nec_transmit_library
  nec_receive_library
  )

pico_add_extra_outputs(pio_ir_loopback)

# add url via pico_set_program_url
example_auto_set_url(pio_ir_loopback)
