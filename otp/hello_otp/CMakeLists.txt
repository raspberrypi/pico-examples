add_executable(hello_otp
        hello_otp.c
        )

# pull in common dependencies
target_link_libraries(hello_otp pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_otp)

# add url via pico_set_program_url
example_auto_set_url(hello_otp)
