# Example running FreeRTOS on 1 core
add_executable(hello_freertos_one_core
    hello_freertos.c
    )
target_include_directories(hello_freertos_one_core PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
# Linking to FreeRTOS-Kernel-Heap4 means we use a dynamic heap for allocations
target_link_libraries(hello_freertos_one_core PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
    )
# Set the nunber of cores to 1.
# This defaults to 2 in FreeRTOSConfig_examples_common.h if not defined in here
target_compile_definitions(hello_freertos_one_core PRIVATE
    configNUMBER_OF_CORES=1
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(hello_freertos_one_core PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(hello_freertos_one_core)

# Example running FreeRTOS on 2 cores
add_executable(hello_freertos_two_cores
    hello_freertos.c
    )
target_include_directories(hello_freertos_two_cores PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
# Linking to FreeRTOS-Kernel-Heap4 to use a dynamic heap for allocations
target_link_libraries(hello_freertos_two_cores PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(hello_freertos_two_cores PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(hello_freertos_two_cores)

# Example running FreeRTOS on 2 cores with static RAM allocation
add_executable(hello_freertos_static_allocation
    hello_freertos.c
    )
target_include_directories(hello_freertos_static_allocation PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
# Linking to FreeRTOS-Kernel-Static to use static memory instead of a dynamic heap for allocations
target_link_libraries(hello_freertos_static_allocation PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Static
    pico_stdlib
    )
# Change the configuration to just use static RAM allocation
# If configSUPPORT_DYNAMIC_ALLOCATION is left undefined it will default to 1
# If configSUPPORT_STATIC_ALLOCATION is left undefined it will default to 0
target_compile_definitions(hello_freertos_static_allocation PRIVATE
    configSUPPORT_STATIC_ALLOCATION=1
    configSUPPORT_DYNAMIC_ALLOCATION=0
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(hello_freertos_static_allocation PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(hello_freertos_static_allocation)
