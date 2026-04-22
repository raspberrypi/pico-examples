if (0)
    add_library(powman_example INTERFACE)
    target_sources(powman_example INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/powman_example.c
    )
    target_include_directories(powman_example INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )
    target_link_libraries(powman_example INTERFACE
        hardware_powman
        hardware_gpio
    )
endif()

add_subdirectory(low_power_sleep)
add_subdirectory(low_power_dormant)
add_subdirectory(low_power_pstate)
#add_subdirectory(powman_gpio)
