if (TARGET hardware_uart)
    add_subdirectory_exclude_platforms(hello_uart)
    add_subdirectory_exclude_platforms(lcd_uart host)
    add_subdirectory_exclude_platforms(uart_advanced host)
else()
    message("Skipping UART examples as hardware_uart is unavailable on this platform")
endif()