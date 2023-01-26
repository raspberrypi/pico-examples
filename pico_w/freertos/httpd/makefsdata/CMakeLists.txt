cmake_minimum_required(VERSION 3.4)
project(makefsdata)

add_executable(makefsdata
    ${PICO_LWIP_PATH}/src/apps/http/makefsdata/makefsdata.c
)
target_include_directories(makefsdata PRIVATE
    ${PICO_LWIP_PATH}/src/include
    ${PICO_LWIP_PATH}/contrib/ports/unix/port/include
    .. # for lwipopts.h
    ../../.. # for common lwipopts.h
)
