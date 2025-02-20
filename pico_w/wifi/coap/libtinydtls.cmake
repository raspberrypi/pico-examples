if (NOT LIB_TINYDTLS_DIR)
    message(FATAL_ERROR "Need to define LIB_TINYDTLS_DIR")
else()
    set(TINYDTLS_SRC
        dtls.c
        netq.c
        peer.c
        session.c
        crypto.c
        ccm.c
        hmac.c
        dtls_time.c
        dtls_debug.c
        dtls_prng.c
        aes/rijndael.c
        aes/rijndael_wrap.c
        sha2/sha2.c
        ecc/ecc.c
        )
    list(TRANSFORM TINYDTLS_SRC PREPEND ${LIB_TINYDTLS_DIR}/)

    pico_add_library(pico_tinydtls)
    target_include_directories(pico_tinydtls_headers INTERFACE
        ${LIB_TINYDTLS_DIR}/
        ${LIB_TINYDTLS_DIR}/..
        )
    target_sources(pico_tinydtls INTERFACE
        ${TINYDTLS_SRC}
        )
    target_compile_definitions(pico_tinydtls INTERFACE
        COAP_WITH_LIBTINYDTLS # This is a coap thing really?
        WITH_LWIP
        WITH_SHA256
        _DTLS_MUTEX_H_ # :(
        )
endif()
