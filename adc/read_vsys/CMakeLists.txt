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

add_executable(read_vsys
    read_vsys.c
    )
target_include_directories(read_vsys PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_link_libraries(read_vsys
    pico_stdlib
    power_status_adc
    )
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(read_vsys
        pico_cyw43_arch_none
    )
endif()

pico_add_extra_outputs(read_vsys)
example_auto_set_url(read_vsys)
