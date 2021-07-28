if (NOT IO_LIBRARY_DRIVER_DIR)
    #replace path to the library here
    set(IO_LIBRARY_DRIVER_DIR ${CMAKE_CURRENT_LIST_DIR}/ioLibrary_Driver)
endif()

if (EXISTS ${IO_LIBRARY_DRIVER_DIR}/Ethernet)
    set(IO_LIBRARY_ETHERNET_DIR ${IO_LIBRARY_DRIVER_DIR}/Ethernet)
    set(IO_LIBRARY_LOOPBACK_DIR ${IO_LIBRARY_DRIVER_DIR}/Application/loopback)

    add_library(io_library_ethernet INTERFACE)
    target_sources(io_library_ethernet INTERFACE
            ${IO_LIBRARY_ETHERNET_DIR}/socket.c
            ${IO_LIBRARY_ETHERNET_DIR}/wizchip_conf.c
            ${IO_LIBRARY_LOOPBACK_DIR}/loopback.c)
    target_include_directories(io_library_ethernet INTERFACE ${IO_LIBRARY_ETHERNET_DIR} ${IO_LIBRARY_LOOPBACK_DIR})

    add_library(w5500_ethernet INTERFACE)
    target_sources(w5500_ethernet INTERFACE ${IO_LIBRARY_ETHERNET_DIR}/W5500/w5500.c)
    target_include_directories(w5500_ethernet INTERFACE ${IO_LIBRARY_ETHERNET_DIR}/W5500)
    target_compile_definitions(w5500_ethernet INTERFACE _WIZCHIP_=W5500)
    target_link_libraries(w5500_ethernet INTERFACE io_library_ethernet)


else()
    message(WARNING "IOLibrary Ethernet Driver not found; looked in ${IO_LIBRARY_DRIVER_DIR}")
endif()

# proceeds if library is present 
if (TARGET w5500_ethernet)
    add_executable(w5500_spi w5500_spi.c)

    pico_enable_stdio_uart(w5500_spi 1)
    pico_enable_stdio_usb(w5500_spi 0)

    target_link_libraries(w5500_spi PUBLIC pico_stdlib hardware_spi w5500_ethernet)

    pico_add_extra_outputs(w5500_spi)

    # add url via pico_set_program_url
    example_auto_set_url(w5500_spi)
else()
    message("Skipping w5500_spi example because the ethernet driver is not found")
endif()