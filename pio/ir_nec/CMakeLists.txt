# build the transmit and receive libraries
#
add_subdirectory(nec_transmit_library)
add_subdirectory(nec_receive_library)

# build the example program
#
add_subdirectory(ir_loopback)
