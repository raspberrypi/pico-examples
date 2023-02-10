set(NAME spp_streamer)
picow_bt_example_target_name(${NAME}_with_wifi TARGET_NAME)

picow_bt_example_poll(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_poll)
picow_bt_example_background(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_background)
picow_bt_example_freertos(${NAME} ${TARGET_NAME} picow_bt_example_cyw43_lwip_freertos)
