# 1 Create an INTERFACE library aggregating all the common parts of the application
add_library(common_stuff INTERFACE)

# note cmake policy is to use absolute paths for interface libraries.
target_sources(common_stuff INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/other.c
        )

target_compile_definitions(common_stuff INTERFACE
        A_DEFINE_THAT_IS_SHARED=123
        )

# can include library dependencies here
target_link_libraries(common_stuff INTERFACE
        pico_stdlib
        )

# 2 Create the first executable including all the common stuff...
#    we can set compile definitions for this executable here too. Because
#    we depend on an INTERFACE library (common_stuff) we
#    will pick up all of its definitions/dependencies too
add_executable(build_variant1)
target_link_libraries(build_variant1 common_stuff)
target_compile_definitions(build_variant1 PRIVATE
        A_DEFINE_THAT_IS_NOT_SHARED=456)
pico_add_extra_outputs(build_variant1)

# 3 Create a second executable including all the common stuff
#    this version also sets the DO_EXTRA define
add_executable(build_variant2)
target_link_libraries(build_variant2 common_stuff)
target_compile_definitions(build_variant2 PRIVATE
        A_DEFINE_THAT_IS_NOT_SHARED=789
        DO_EXTRA)
pico_add_extra_outputs(build_variant2)

