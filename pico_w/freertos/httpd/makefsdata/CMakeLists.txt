cmake_minimum_required(VERSION 3.12)
project(makefsdata)

set(CMAKE_CXX_STANDARD 14)

add_executable(makefsdata
	${PICO_SDK_PATH}/lib/lwip/src/apps/http/makefsdata/makefsdata.c
)
target_include_directories(makefsdata PRIVATE
	${PICO_SDK_PATH}/lib/lwip/src/include
	${PICO_SDK_PATH}/lib/lwip/contrib/ports/unix/port/include
	.. # for lwipopts.h
	../../.. # for common lwipopts.h
)
