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

    project(sha256 C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

if (NOT TARGET hardware_sha256)
    return()
endif()

# Generates an inc file from a txt file, and return a target for dependencies
function(make_sample_inc input_filename target_name)
    # Write script file to run later, to generate the inc file
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/inc.cmake"
        "file(READ \${input_file} content)\n"
        "file(WRITE \${output_file} \"static const char sample_txt[] = { \\\"\${content}\\\" };\")"
        )
    # Add command to run this whenever input_filename changes
    get_filename_component(input_file "${input_filename}" REALPATH)
    set(output_file "${CMAKE_CURRENT_BINARY_DIR}/generated/${input_filename}.inc")
    add_custom_command(OUTPUT ${output_file}
        COMMAND ${CMAKE_COMMAND} -Dinput_file:FILEPATH=${input_file} -Doutput_file:FILEPATH=${output_file} -P "${CMAKE_CURRENT_BINARY_DIR}/generated/inc.cmake"
        DEPENDS ${input_filename})
    add_custom_target(${target_name} DEPENDS ${output_file})
endfunction(make_sample_inc)

make_sample_inc(sample.txt sample.txt.inc)

add_executable(hello_sha256
        hello_sha256.c
        )
add_dependencies(hello_sha256 sample.txt.inc)
target_link_libraries(hello_sha256
        pico_stdlib
        pico_sha256
)
target_include_directories(hello_sha256 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)
pico_add_extra_outputs(hello_sha256)
example_auto_set_url(hello_sha256)
