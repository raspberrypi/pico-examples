get_filename_component(project_name ${CMAKE_CURRENT_LIST_DIR} NAME)

add_executable(${project_name}
main.c
)

target_compile_definitions(${project_name} PUBLIC -DxPortIsInsideInterrupt=portCHECK_IF_IN_ISR)
target_include_directories(${project_name} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR} ../../config)

# pull in common dependencies
target_link_libraries(${project_name} PRIVATE
pico_stdlib
hardware_resets
hardware_irq
cherryusb_host
pico_async_context_freertos
FreeRTOS-Kernel-Heap4)
# create map/bin/hex file etc.
pico_add_extra_outputs(${project_name})

# add url via pico_set_program_url
example_auto_set_url(${project_name})