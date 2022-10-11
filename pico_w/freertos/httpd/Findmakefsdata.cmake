if (NOT makefsdata_FOUND)

    include(ExternalProject)

    set(MAKEFSDATA_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/makefsdata)
    set(MAKEFSDATA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/makefsdata)

    set(makefsdataBuild_TARGET makefsdataBuild)
    set(makefsdata_TARGET makefsdata)

    if (NOT TARGET ${makefsdataBuild_TARGET})
        ExternalProject_Add(${makefsdataBuild_TARGET}
                PREFIX makefsdata SOURCE_DIR ${MAKEFSDATA_SOURCE_DIR}
                BINARY_DIR ${MAKEFSDATA_BINARY_DIR}
                BUILD_ALWAYS 1 # force dependency checking
                INSTALL_COMMAND ""
                CMAKE_CACHE_ARGS "-DPICO_SDK_PATH:STRING=${PICO_SDK_PATH}"
                )
    endif()

    if (CMAKE_HOST_WIN32)
        set(makefsdata_EXECUTABLE ${MAKEFSDATA_BINARY_DIR}/makefsdata.exe)
    else()
        set(makefsdata_EXECUTABLE ${MAKEFSDATA_BINARY_DIR}/makefsdata)
    endif()
    if(NOT TARGET ${makefsdata_TARGET})
        add_executable(${makefsdata_TARGET} IMPORTED)
    endif()
    set_property(TARGET ${makefsdata_TARGET} PROPERTY IMPORTED_LOCATION
            ${makefsdata_EXECUTABLE})

    add_dependencies(${makefsdata_TARGET} ${makefsdataBuild_TARGET})

set(makefsdata_FOUND 1)
endif()
