add_subdirectory(standalone)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_EXAMPLE_PATH ${BTSTACK_ROOT}/example)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)
set(BT_EXAMPLE_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}")

if (NOT PICO_EXTRAS_PATH)
        message("Skipping some Pico W BTstack examples that require pico-extras")
else()
        add_library(pico_btstack_audio_example INTERFACE)
        target_sources(pico_btstack_audio_example INTERFACE
                ${PICO_BTSTACK_PATH}/src/btstack_audio.c
                ${CMAKE_CURRENT_LIST_DIR}/btstack_audio_pico.c
                )
        target_link_libraries(pico_btstack_audio_example INTERFACE
                pico_audio_i2s
                )
endif()

# mod player used by a2dp_source_demo and mod_player and demo song
add_library(pico_btstack_hxcmod_player INTERFACE)
target_sources(pico_btstack_hxcmod_player INTERFACE
        ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/hxcmod.c
        ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/mods/nao-deceased_by_disease.c
        )
target_include_directories(pico_btstack_hxcmod_player INTERFACE
        ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player
        ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/mods
        )

add_library(pico_btstack_sco_demo_util INTERFACE)
target_sources(pico_btstack_sco_demo_util INTERFACE
        # sco demo utils
        ${BTSTACK_EXAMPLE_PATH}/sco_demo_util.c
        )

# Adds common stuff for all the examples
add_library(picow_bt_example_common INTERFACE)
target_sources(picow_bt_example_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/picow_bt_example_common.c
        )
target_link_libraries(picow_bt_example_common INTERFACE
        pico_stdlib
        pico_btstack_cyw43
        )
target_include_directories(picow_bt_example_common INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/config # Use our own config
        ${BTSTACK_EXAMPLE_PATH}/
        )
target_compile_definitions(picow_bt_example_common INTERFACE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        #WANT_HCI_DUMP=1 # This enables btstack debug
        #ENABLE_SEGGER_RTT=1
        )
if (TARGET pico_btstack_audio_example)
        target_link_libraries(picow_bt_example_common INTERFACE
                pico_btstack_audio_example
                )
        target_compile_definitions(picow_bt_example_common INTERFACE
                TEST_AUDIO=1
                )
endif()

# Adds stuff needed for cyw43 lwip
add_library(picow_bt_example_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_example_cyw43_lwip INTERFACE
        picow_bt_example_common
        )
target_compile_definitions(picow_bt_example_cyw43_lwip INTERFACE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        CYW43_LWIP=1
        )
target_link_libraries(picow_bt_example_cyw43_lwip INTERFACE
        pico_lwip_iperf
        )
target_include_directories(picow_bt_example_cyw43_lwip INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

# disables cyw43 lwip
add_library(picow_bt_example_no_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_example_no_cyw43_lwip INTERFACE
        picow_bt_example_common
        )
target_compile_definitions(picow_bt_example_no_cyw43_lwip INTERFACE
        CYW43_LWIP=0
        )

# variant: no cyw43 lwip | poll
add_library(picow_bt_example_no_cyw43_lwip_poll INTERFACE)
target_sources(picow_bt_example_no_cyw43_lwip_poll INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_poll.c
        )
target_link_libraries(picow_bt_example_no_cyw43_lwip_poll INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_poll
        )

# variant: cyw43 lwip | poll
add_library(picow_bt_example_cyw43_lwip_poll INTERFACE)
target_sources(picow_bt_example_cyw43_lwip_poll INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_poll.c
        )
target_link_libraries(picow_bt_example_cyw43_lwip_poll INTERFACE
        picow_bt_example_cyw43_lwip
        pico_cyw43_arch_lwip_poll
        )

# variant: btstack lwip | poll
add_library(picow_bt_example_btstack_lwip_poll INTERFACE)
target_sources(picow_bt_example_btstack_lwip_poll INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_poll.c
        )
target_link_libraries(picow_bt_example_btstack_lwip_poll INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_poll
        pico_lwip_nosys
        pico_btstack_bnep_lwip
        )
target_include_directories(picow_bt_example_btstack_lwip_poll INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

# variant: no cyw43 lwip | background
add_library(picow_bt_example_no_cyw43_lwip_background INTERFACE)
target_sources(picow_bt_example_no_cyw43_lwip_background INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_no_cyw43_lwip_background INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_threadsafe_background
        )

# variant: cyw43 lwip | background
add_library(picow_bt_example_cyw43_lwip_background INTERFACE)
target_sources(picow_bt_example_cyw43_lwip_background INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_cyw43_lwip_background INTERFACE
        picow_bt_example_cyw43_lwip
        pico_cyw43_arch_lwip_threadsafe_background
        )

# variant: btstack lwip | background
add_library(picow_bt_example_btstack_lwip_background INTERFACE)
target_sources(picow_bt_example_btstack_lwip_background INTERFACE
        ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_btstack_lwip_background INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_threadsafe_background
        pico_lwip_nosys
        pico_btstack_bnep_lwip
        )
target_include_directories(picow_bt_example_btstack_lwip_background INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

if (FREERTOS_KERNEL_PATH OR DEFINED ENV{FREERTOS_KERNEL_PATH})
include(FreeRTOS_Kernel_import.cmake)
endif()

if (TARGET FreeRTOS-Kernel)
        # common freertos stuff
        add_library(picow_bt_example_common_freertos INTERFACE)
        target_sources(picow_bt_example_common_freertos INTERFACE
                ${BT_EXAMPLE_COMMON_DIR}/picow_bt_example_freertos.c
                )
        target_link_libraries(picow_bt_example_common_freertos INTERFACE
                picow_bt_example_common
                FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
                )
        target_compile_definitions(picow_bt_example_common_freertos INTERFACE
                CYW43_TASK_STACK_SIZE=1024
                NO_SYS=0
                )
        target_include_directories(picow_bt_example_common_freertos INTERFACE
                ${FREERTOS_KERNEL_PATH}/include
                ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
                )

        # variant: no cyw43 lwip | freertos
        add_library(picow_bt_example_no_cyw43_lwip_freertos INTERFACE)
        target_link_libraries(picow_bt_example_no_cyw43_lwip_freertos INTERFACE
                picow_bt_example_common_freertos
                picow_bt_example_no_cyw43_lwip
                pico_cyw43_arch_sys_freertos
                )

        # variant: cyw43 lwip | freertos
        add_library(picow_bt_example_cyw43_lwip_freertos INTERFACE)
        target_link_libraries(picow_bt_example_cyw43_lwip_freertos INTERFACE
                picow_bt_example_common_freertos
                picow_bt_example_cyw43_lwip
                pico_cyw43_arch_lwip_sys_freertos
                )
        target_compile_definitions(picow_bt_example_cyw43_lwip_freertos INTERFACE
                HAVE_LWIP=1 # stops btstack calling lwip_init
                )

        # variant: btstack lwip | freertos
        add_library(picow_bt_example_btstack_lwip_freertos INTERFACE)
        target_link_libraries(picow_bt_example_btstack_lwip_freertos INTERFACE
                picow_bt_example_common_freertos
                picow_bt_example_no_cyw43_lwip
                pico_cyw43_arch_sys_freertos
                pico_btstack_bnep_lwip_sys_freertos
                pico_lwip_freertos
                )
        target_include_directories(picow_bt_example_btstack_lwip_freertos INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
                )
        target_compile_definitions(picow_bt_example_btstack_lwip_freertos INTERFACE
                HAVE_LWIP=1 # stops btstack calling lwip_init
                )
endif()

# Common stuff for all examples. Pass the example name
function(picow_bt_example_common NAME)
        if (NOT TARGET picow_bt_example_common_${NAME})
                add_library(picow_bt_example_common_${NAME} INTERFACE)
                target_sources(picow_bt_example_common_${NAME} INTERFACE
                        # actual example
                        ${BTSTACK_EXAMPLE_PATH}/${NAME}.c
                        )
                if (EXISTS "${BTSTACK_EXAMPLE_PATH}/${NAME}.gatt")
                        pico_btstack_make_gatt_header(picow_bt_example_common_${NAME} INTERFACE ${BTSTACK_EXAMPLE_PATH}/${NAME}.gatt)
                endif()
        endif()
endfunction()

# The type of example to build
if(NOT DEFINED BTSTACK_EXAMPLE_TYPE)
   set(BTSTACK_EXAMPLE_TYPE "background")
endif()
if ((NOT BTSTACK_EXAMPLE_TYPE STREQUAL "poll") AND
    (NOT BTSTACK_EXAMPLE_TYPE STREQUAL "background") AND
    (NOT BTSTACK_EXAMPLE_TYPE STREQUAL "freertos") AND
    (NOT BTSTACK_EXAMPLE_TYPE STREQUAL "all"))
        message(FATAL_ERROR "Unknown BTSTACK_EXAMPLE_TYPE '${BTSTACK_EXAMPLE_TYPE}'; valid options are 'poll', 'background', 'freertos' or 'all'")
endif()
set(BTSTACK_EXAMPLE_TYPE "${BTSTACK_EXAMPLE_TYPE}" CACHE INTERNAL "BT stack example type (poll|background|freertos|all)")

# Add an poll example, pass btstack example name, target name, variant lib and extra libs
function(picow_bt_example_poll NAME TARGET_NAME VARIANT_LIB)
        if ("${BTSTACK_EXAMPLE_TYPE}" STREQUAL "poll" OR "${BTSTACK_EXAMPLE_TYPE}" STREQUAL "all")
                picow_bt_example_common(${NAME})
                add_executable(${TARGET_NAME}_poll)
                target_link_libraries(${TARGET_NAME}_poll PRIVATE
                        picow_bt_example_common_${NAME}
                        ${VARIANT_LIB}
                        ${ARGN} # extra libs
                        )
                pico_add_extra_outputs(${TARGET_NAME}_poll)
                example_auto_set_url(${TARGET_NAME}_poll)
        endif()
endfunction()

# Add an background example, pass btstack example name, target name, variant lib and extra libs
function(picow_bt_example_background NAME TARGET_NAME VARIANT_LIB)
        if ("${BTSTACK_EXAMPLE_TYPE}" STREQUAL "background" OR "${BTSTACK_EXAMPLE_TYPE}" STREQUAL "all")
                picow_bt_example_common(${NAME})
                add_executable(${TARGET_NAME}_background)
                target_link_libraries(${TARGET_NAME}_background PRIVATE
                        picow_bt_example_common_${NAME}
                        ${VARIANT_LIB}
                        ${ARGN} # extra libs
                        )
                pico_add_extra_outputs(${TARGET_NAME}_background)
                example_auto_set_url(${TARGET_NAME}_background)
        endif()
endfunction()

# Add a freertos example, pass btstack example name, target name, variant lib and extra libs
function(picow_bt_example_freertos NAME TARGET_NAME VARIANT_LIB)
        if ("${BTSTACK_EXAMPLE_TYPE}" STREQUAL "freertos" OR "${BTSTACK_EXAMPLE_TYPE}" STREQUAL "all")
                if (TARGET FreeRTOS-Kernel)
                        picow_bt_example_common(${NAME})
                        add_executable(${TARGET_NAME}_freertos)
                        target_link_libraries(${TARGET_NAME}_freertos PRIVATE
                                picow_bt_example_common_${NAME}
                                ${VARIANT_LIB}
                                ${ARGN} # extra libs
                                )
                        pico_add_extra_outputs(${TARGET_NAME}_freertos)
                        example_auto_set_url(${TARGET_NAME}_freertos)
                endif()
        endif()
endfunction()

# The default name of the bt example target
function(picow_bt_example_target_name NAME RET)
        SET(${RET} "picow_bt_example_${NAME}" PARENT_SCOPE)
endfunction()

# Make a btstack example, NAME should match source file in lib/btstack/example
# Extra parameters indicate extra libraries to link to
function(picow_bt_example NAME)
        picow_bt_example_target_name(${NAME} TARGET_NAME)
        picow_bt_example_poll(${NAME} ${TARGET_NAME} picow_bt_example_no_cyw43_lwip_poll pico_btstack_ble pico_btstack_classic ${ARGN})
        picow_bt_example_background(${NAME} ${TARGET_NAME} picow_bt_example_no_cyw43_lwip_background pico_btstack_ble pico_btstack_classic ${ARGN})
        picow_bt_example_freertos(${NAME} ${TARGET_NAME} picow_bt_example_no_cyw43_lwip_freertos pico_btstack_ble pico_btstack_classic ${ARGN})
endfunction()

# List of examples from btstack
include(${BTSTACK_ROOT}/example/CMakeLists.txt)

# Full list of bluetooth examples
set(BTSTACK_EXAMPLES
        ${EXAMPLES_GENERAL}
        ${EXAMPLES_CLASSIC_ONLY}
        ${EXAMPLES_LE_ONLY}
        ${EXAMPLES_DUAL_MODE}
        pan_lwip_http_server
)

# These examples run wifi and bt at the same time
if (WIFI_SSID AND WIFI_PASSWORD)
        list(APPEND BTSTACK_EXAMPLES
                gatt_counter_with_wifi
                gatt_streamer_server_with_wifi
                #spp_streamer_with_wifi
        )
endif()

# Extra examples that are not that interesting or a bit tricky to run
# They are not built unless BTSTACK_EXAMPLES_ALL=1
set(BTSTACK_EXAMPLES_ADDITIONAL
        ancs_client_demo
        att_delayed_response
        avrcp_browsing_client
        dut_mode_classic
        gap_dedicated_bonding
        gap_link_keys
        le_credit_based_flow_control_mode_client
        le_credit_based_flow_control_mode_server
        le_mitm
        led_counter
        sdp_bnep_query
        sdp_general_query
        spp_flowcontrol
        sdp_rfcomm_query
        spp_and_gatt_counter
        spp_and_gatt_streamer
        hfp_ag_demo
        hsp_ag_demo
        hfp_hf_demo
        hsp_hs_demo
        mod_player
        sine_player
        ublox_spp_le_counter
        nordic_spp_le_streamer
        nordic_spp_le_counter
        hog_boot_host_demo
        gatt_battery_query
        gatt_browser
        gatt_device_information_query
        le_streamer_client
)

# These examples will only be built if pico-extras exists
set(BTSTACK_EXAMPLES_NEED_EXTRAS
        a2dp_sink_demo
        hfp_hf_demo
        hfp_ag_demo
        hsp_ag_demo
        hsp_hs_demo
        mod_player
        sine_player
)

# Add examples
set(BTSTACK_EXAMPLE_COUNT 0)
list(REMOVE_DUPLICATES BTSTACK_EXAMPLES)
foreach(NAME ${BTSTACK_EXAMPLES})
        # Ignore if sub folder not found
        if (NOT IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${NAME})
                continue()
        endif()
        # Ignore example if it needs pico-extras and pico-extras could not be found
        if (NOT PICO_EXTRAS_PATH AND ${NAME} IN_LIST BTSTACK_EXAMPLES_NEED_EXTRAS)
                continue()
        endif()
        # Ignore less interesting examples if BTSTACK_EXAMPLES_ALL=1 is not set
        if (NOT BTSTACK_EXAMPLES_ALL AND ${NAME} IN_LIST BTSTACK_EXAMPLES_ADDITIONAL)
                continue()
        endif()
        # add example
        add_subdirectory(${NAME})
        MATH(EXPR BTSTACK_EXAMPLE_COUNT "${BTSTACK_EXAMPLE_COUNT}+1")
endforeach()
message("Adding ${BTSTACK_EXAMPLE_COUNT} BTstack examples of type '${BTSTACK_EXAMPLE_TYPE}'")

suppress_btstack_warnings()
