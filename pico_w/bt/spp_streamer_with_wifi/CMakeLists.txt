set(NAME spp_streamer)
picow_bt_example_target_name(${NAME}_with_wifi TARGET_NAME)

picow_bt_example_poll(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_poll pico_btstack_classic)
picow_bt_example_background(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_background pico_btstack_classic)
picow_bt_example_freertos(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_freertos pico_btstack_classic)
