# This builds the tool makefsdata.linux
# export PICO_SDK_PATH=~/pico-sdk
# mkdir build
# cd build
# make -j8
# cp makefsdata.linux ..

cmake_minimum_required(VERSION 3.12)
project(makefsdata.linux)

set(CMAKE_CXX_STANDARD 14)

add_executable(makefsdata.linux
	$ENV{PICO_SDK_PATH}/lib/lwip/src/apps/http/makefsdata/makefsdata.c
)
target_include_directories(makefsdata.linux PRIVATE
	$ENV{PICO_SDK_PATH}/lib/lwip/src/include
	$ENV{PICO_SDK_PATH}/lib/lwip/contrib/ports/unix/port/include
	.. # for lwipopts.h
	../../.. # for common lwipopts.h
)
