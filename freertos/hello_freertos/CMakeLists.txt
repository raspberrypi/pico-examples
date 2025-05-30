# Example running FreeRTOS on 1 core
set(TARGET_NAME hello_freertos1)
add_executable(${TARGET_NAME}
    hello_freertos.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(${TARGET_NAME} PRIVATE
        pico_cyw43_arch_none
        )
endif()
target_compile_definitions(${TARGET_NAME} PRIVATE
    configNUMBER_OF_CORES=1
    )
pico_add_extra_outputs(${TARGET_NAME})

# Example running FreeRTOS on 2 cores
set(TARGET_NAME hello_freertos2)
add_executable(${TARGET_NAME}
    hello_freertos.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(${TARGET_NAME} PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(${TARGET_NAME})

# Example running FreeRTOS on 2 cores with static RAM allocation
set(TARGET_NAME hello_freertos_static)
add_executable(${TARGET_NAME}
    hello_freertos.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Static
    pico_stdlib
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    configSUPPORT_STATIC_ALLOCATION=1
    configSUPPORT_DYNAMIC_ALLOCATION=0
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(${TARGET_NAME} PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(${TARGET_NAME})
