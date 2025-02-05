cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    # Pull in SDK (must be before project)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
        include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    elseif (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
        # The root pico-examples one
        include(${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
    else()
        message(FATAL_ERROR "Unable to locate pico_sdk_import.cmake - make sure you have copied it into this directory")
    endif()

    project(runtime_flash_permissions C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(runtime_flash_permissions
        runtime_flash_permissions.c
        )

# pull in common dependencies
target_link_libraries(runtime_flash_permissions pico_stdlib)

# not necessary for the example functionality, but shows you how you would add a partition table
pico_embed_pt_in_binary(runtime_flash_permissions ${CMAKE_CURRENT_LIST_DIR}/pt.json)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(runtime_flash_permissions)

# add url via pico_set_program_url
example_auto_set_url(runtime_flash_permissions)
