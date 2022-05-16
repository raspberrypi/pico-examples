add_executable(clocks_detached_clk_peri
        detached_clk_peri.c
        )

# pull in common dependencies
target_link_libraries(clocks_detached_clk_peri pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(clocks_detached_clk_peri)

# add url via pico_set_program_url
example_auto_set_url(clocks_detached_clk_peri)
