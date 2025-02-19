cmake_minimum_required(VERSION 3.12)
project(pico_mbedtls_host_test C)

option(ENABLE_TESTING "Build mbed TLS tests." OFF)
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)

# Pick DTLS server from environment
if (DEFINED ENV{DTLS_SERVER} AND (NOT DTLS_SERVER))
    set(DTLS_SERVER $ENV{DTLS_SERVER})
    message("Using DTLS_SERVER from environment ('${DTLS_SERVER}')")
endif()
if (NOT DTLS_SERVER)
    message("Skipping DTLS example as DTLS_SERVER is not defined")
    return()
endif()
set(DTLS_SERVER "${DTLS_SERVER}" CACHE INTERNAL "DTLS server for examples")
if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../certs/${DTLS_SERVER}")
    message("Generate DTLS certs by running certs/makecerts.sh")
    return()
endif()

if (DEFINED ENV{MBEDTLS_DIR} AND NOT MBEDTLS_DIR)
	set(MBEDTLS_DIR $ENV{MBEDTLS_DIR})
endif()
if (NOT MBEDTLS_DIR)
	if (DEFINED ENV{PICO_SDK_PATH} AND NOT PICO_SDK_PATH)
		set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
	endif()
	if (PICO_SDK_PATH)
		set(MBEDTLS_DIR ${PICO_SDK_PATH}/lib/mbedtls)
	else()
		message(FATAL_ERROR "Need to set MBEDTLS_DIR to mbedtls source location")
	endif()
endif()
add_subdirectory(${MBEDTLS_DIR} mbedtls)

add_executable(dtls_host_echo_client
	dtls_host_echo_client.c
	)
target_link_libraries(dtls_host_echo_client PRIVATE
	mbedtls
	)
target_include_directories(dtls_host_echo_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_compile_definitions(dtls_host_echo_client PRIVATE
    DTLS_SERVER=\"${DTLS_SERVER}\"
    DTLS_CERT_INC=\"certs/${DTLS_SERVER}/dtls_client.inc\"
    )

add_executable(dtls_host_echo_server
	dtls_host_echo_server.c
	)
target_link_libraries(dtls_host_echo_server PRIVATE
	mbedtls
	)
target_include_directories(dtls_host_echo_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_compile_definitions(dtls_host_echo_server PRIVATE
    DTLS_CERT_INC=\"certs/${DTLS_SERVER}/dtls_server.inc\"
    )
