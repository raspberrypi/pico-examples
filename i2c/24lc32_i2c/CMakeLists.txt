# Add libary
add_library(24lc32_i2c_lib INTERFACE)
target_sources(24lc32_i2c_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/24lc32_i2c_lib.c
        )
target_include_directories(24lc32_i2c_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(24lc32_i2c_lib INTERFACE
        pico_stdlib
        hardware_i2c
        )

# Add executable 
add_executable(24lc32_i2c 
        example.c 
        )

target_link_libraries(24lc32_i2c
        24lc32_i2c_lib
        pico_stdlib
        hardware_i2c
        )

pico_add_extra_outputs(24lc32_i2c)

# add url via pico_set_program_url
example_auto_set_url(24lc32_i2c)