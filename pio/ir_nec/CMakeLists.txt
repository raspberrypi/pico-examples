# build the transmit and receive libraries
#
add_subdirectory_exclude_platforms(nec_transmit_library)
add_subdirectory_exclude_platforms(nec_receive_library)

# build the example program
#
add_subdirectory_exclude_platforms(ir_loopback)
