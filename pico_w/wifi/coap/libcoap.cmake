if (NOT LIB_COAP_DIR)
    message(FATAL_ERROR "Need to define LIB_COAP_DIR")
else()
    set(LIB_COAP_DIR "${LIB_COAP_DIR}" CACHE INTERNAL "libcoap directory location")

    set(PACKAGE_URL "https://libcoap.net/")
    set(PACKAGE_NAME "libcoap")
    set(PACKAGE_STRING "libcoap 4.3.4")
    set(PACKAGE_VERSION "4.3.4")
    set(PACKAGE_BUGREPORT "libcoap-developers@lists.sourceforge.net")
    set(LIBCOAP_API_VERSION 3)
    set(LIBCOAP_VERSION 4003004)

    set(COAP_SRC
        coap_asn1.c
        coap_async.c
        coap_block.c
        coap_cache.c
        coap_debug.c
        coap_dtls.c
        coap_encode.c
        coap_hashkey.c
        coap_io.c
        coap_io_lwip.c
        coap_layers.c
        coap_net.c
        coap_netif.c
        coap_notls.c
        coap_option.c
        coap_oscore.c
        coap_pdu.c
        coap_resource.c
        coap_session.c
        coap_subscribe.c
        coap_str.c
        coap_tcp.c
        coap_uri.c
        coap_ws.c
        coap_tinydtls.c
        coap_prng.c
        )
    list(TRANSFORM COAP_SRC PREPEND ${LIB_COAP_DIR}/src/)

    pico_add_library(pico_coap)
    target_include_directories(pico_coap_headers INTERFACE
        ${LIB_COAP_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        )
    target_sources(pico_coap INTERFACE
        ${COAP_SRC}
        )
    target_compile_definitions(pico_coap INTERFACE
        COAP_SERVER_SUPPORT
        COAP_CLIENT_SUPPORT
        )
endif()
