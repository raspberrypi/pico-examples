if (NOT Makefsdata_FOUND)

    include(ExternalProject)

    set(MAKEFSDATA_SOURCE_DIR ${PICO_EXAMPLES_PATH}/pico_w/freertos/httpd/makefsdata)
    set(MAKEFSDATA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/makefsdata)

    set(MakefsdataBuild_TARGET MakefsdataBuild)
    set(Makefsdata_TARGET Makefsdata)

    if (NOT TARGET ${MakefsdataBuild_TARGET})
        pico_message_debug("MAKEFSDATA will need to be built")
        pico_message_debug("Adding external project ${MakefsdataBuild_TARGET} in ${CMAKE_CURRENT_LIST_DIR}")
        ExternalProject_Add(${MakefsdataBuild_TARGET}
                PREFIX makefsdata
                SOURCE_DIR ${MAKEFSDATA_SOURCE_DIR}
                BINARY_DIR ${MAKEFSDATA_BINARY_DIR}
                CMAKE_ARGS "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                CMAKE_CACHE_ARGS "-DPICO_LWIP_PATH:STRING=${PICO_LWIP_PATH}"
                BUILD_ALWAYS 1 # force dependency checking
                INSTALL_COMMAND ""
                )
    endif()

    if (CMAKE_HOST_WIN32)
        set(Makefsdata_EXECUTABLE ${MAKEFSDATA_BINARY_DIR}/makefsdata.exe)
    else()
        set(Makefsdata_EXECUTABLE ${MAKEFSDATA_BINARY_DIR}/makefsdata)
    endif()
    if(NOT TARGET ${Makefsdata_TARGET})
        pico_message_debug("Adding executable ${Makefsdata_TARGET} in ${CMAKE_CURRENT_LIST_DIR}")
        add_executable(${Makefsdata_TARGET} IMPORTED)
    endif()
    set_property(TARGET ${Makefsdata_TARGET} PROPERTY IMPORTED_LOCATION
        ${Makefsdata_EXECUTABLE})

    pico_message_debug("EXE is ${Makefsdata_EXECUTABLE}")
    add_dependencies(${Makefsdata_TARGET} ${MakefsdataBuild_TARGET})
    set(Makefsdata_FOUND 1)
endif()
