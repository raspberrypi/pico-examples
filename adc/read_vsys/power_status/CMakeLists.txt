add_library(power_status_adc INTERFACE)
target_sources(power_status_adc INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/power_status.c
        )
target_include_directories(power_status_adc INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(power_status_adc INTERFACE
        hardware_adc
        hardware_gpio
        )
