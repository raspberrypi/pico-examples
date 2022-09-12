cmake_minimum_required(VERSION 3.12)

if (PICO_CYW43_SUPPORTED) # set by PICO_BOARD=pico_w
    if (NOT TARGET pico_cyw43_arch)
        message("Skipping Pico W examples as support is not available")
    else()

        if (DEFINED ENV{WIFI_SSID} AND (NOT WIFI_SSID))
            set(WIFI_SSID $ENV{WIFI_SSID})
            message("Using WIFI_SSID from environment ('${WIFI_SSID}')")
        endif()

        if (DEFINED ENV{WIFI_PASSWORD} AND (NOT WIFI_PASSWORD))
            set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
            message("Using WIFI_PASSWORD from environment")
        endif()

        set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
        set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")

        add_subdirectory(blink)
        add_subdirectory(wifi_scan)
        add_subdirectory(access_point)

        if ("${WIFI_SSID}" STREQUAL "")
            message("Skipping some Pico W examples as WIFI_SSID is not defined")
        elseif ("${WIFI_PASSWORD}" STREQUAL "")
            message("Skipping some Pico W examples as WIFI_PASSWORD is not defined")
        else()
            add_subdirectory(iperf)
            add_subdirectory(ntp_client)
            add_subdirectory(tcp_client)
            add_subdirectory(tcp_server)
            add_subdirectory(freertos)
            add_subdirectory(udp)
        endif()
    endif()
endif()
