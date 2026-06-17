if (NOT TARGET pico_btstack_hxcmod_player)
    # mod player used by a2dp_source_demo and mod_player and demo song
    add_library(pico_btstack_hxcmod_player INTERFACE)
    target_sources(pico_btstack_hxcmod_player INTERFACE
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player/hxcmod.c
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player/mods/mod.c
    )
    target_include_directories(pico_btstack_hxcmod_player INTERFACE
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player/mods
    )
    target_compile_definitions(pico_btstack_hxcmod_player INTERFACE
        ENABLE_MODPLAYER=1
    )
endif()

add_subdirectory(a2dp_source_demo)
add_subdirectory(gap_inquiry)
add_subdirectory(gap_le_advertisements)
add_subdirectory(gatt_counter)
add_subdirectory(gatt_streamer_server)
add_subdirectory(gatt_heart_rate_client)
add_subdirectory(hid_host_demo)
add_subdirectory(hid_keyboard_demo)
add_subdirectory(hid_mouse_demo)
add_subdirectory(hog_host_demo)
add_subdirectory(hog_keyboard_demo)
add_subdirectory(hog_mouse_demo)
add_subdirectory(le_streamer_client)
add_subdirectory(pbap_client_demo)
add_subdirectory(sm_pairing_central)
add_subdirectory(sm_pairing_peripheral)
add_subdirectory(spp_counter)
add_subdirectory(spp_streamer)
add_subdirectory(spp_streamer_client)
add_subdirectory(spp_streamer_with_wifi)

# These examples connect to WiFi
# Define WIFI_SSID and WIFI_PASSWORD to enable them
if (WIFI_SSID AND WIFI_PASSWORD)
    add_subdirectory(gatt_counter_with_wifi)
    add_subdirectory(gatt_streamer_server_with_wifi)
endif()

# These examples need https://github.com/raspberrypi/pico-extras
# PICO_EXTRAS_PATH should specify its location
if (PICO_EXTRAS_PATH)
    add_subdirectory(a2dp_sink_demo)
    if (BTSTACK_EXAMPLES_ALL)
        add_subdirectory(hfp_hf_demo)
        add_subdirectory(hfp_ag_demo)
        add_subdirectory(hsp_ag_demo)
        add_subdirectory(hsp_hs_demo)
        add_subdirectory(mod_player)
        add_subdirectory(sine_player)
    endif()
endif()

# Define BTSTACK_EXAMPLES_ALL=1 to enable these examples
if (BTSTACK_EXAMPLES_ALL)
    add_subdirectory(ancs_client_demo)
    add_subdirectory(att_delayed_response)
    add_subdirectory(avrcp_browsing_client)
    add_subdirectory(dut_mode_classic)
    add_subdirectory(gap_dedicated_bonding)
    add_subdirectory(gap_link_keys)
    add_subdirectory(gatt_battery_query)
    add_subdirectory(gatt_browser)
    add_subdirectory(gatt_device_information_query)
    add_subdirectory(hog_boot_host_demo)
    add_subdirectory(le_credit_based_flow_control_mode_client)
    add_subdirectory(le_credit_based_flow_control_mode_server)
    add_subdirectory(le_mitm)
    add_subdirectory(led_counter)
    add_subdirectory(nordic_spp_le_counter)
    add_subdirectory(nordic_spp_le_streamer)
    add_subdirectory(pan_lwip_http_server)
    add_subdirectory(sdp_bnep_query)
    add_subdirectory(sdp_general_query)
    add_subdirectory(sdp_rfcomm_query)
    add_subdirectory(spp_and_gatt_counter)
    add_subdirectory(spp_and_gatt_streamer)
    add_subdirectory(spp_flowcontrol)
    add_subdirectory(ublox_spp_le_counter)
endif()
