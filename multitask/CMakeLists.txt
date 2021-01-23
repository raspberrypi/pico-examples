set(st_sources
    ./s_task/src/s_task.c
    ./s_task/src/s_mutex.c
    ./s_task/src/s_event.c
    ./s_task/src/s_event_embed.c
    ./s_task/src/s_chan.c
    ./s_task/src/s_chan_embed.c
    ./s_task/src/s_timer_fast.c
    ./s_task/src/s_timer_small.c
    ./s_task/src/s_list.c
    ./s_task/src/s_rbtree.c)

add_executable(s_task
        main.c
        ${st_sources}
    )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(s_task pico_stdlib)
target_include_directories(s_task PRIVATE ./s_task/include)

# enable usb output, disable uart output
pico_enable_stdio_usb(s_task 1)
pico_enable_stdio_uart(s_task 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(s_task)

# add url via pico_set_program_url
example_auto_set_url(s_task)
