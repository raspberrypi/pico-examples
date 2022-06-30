if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Skipping Pico W FreeRTOS examples as FREERTOS_KERNEL_PATH not defined")
else()
    include(FreeRTOS_Kernel_import.cmake)

    add_subdirectory(iperf)
    add_subdirectory(ping)
endif()