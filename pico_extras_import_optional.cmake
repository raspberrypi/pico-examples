# This is a copy of <PICO_EXTRAS_PATH>/external/pico_extras_import.cmake

# This can be dropped into an external project to help locate pico-extras
# It should be include()ed prior to project()

if (DEFINED ENV{PICO_EXTRAS_PATH} AND (NOT PICO_EXTRAS_PATH))
    set(PICO_EXTRAS_PATH $ENV{PICO_EXTRAS_PATH})
    message("Using PICO_EXTRAS_PATH from environment ('${PICO_EXTRAS_PATH}')")
endif ()

if (DEFINED ENV{PICO_EXTRAS_FETCH_FROM_GIT} AND (NOT PICO_EXTRAS_FETCH_FROM_GIT))
    set(PICO_EXTRAS_FETCH_FROM_GIT $ENV{PICO_EXTRAS_FETCH_FROM_GIT})
    message("Using PICO_EXTRAS_FETCH_FROM_GIT from environment ('${PICO_EXTRAS_FETCH_FROM_GIT}')")
endif ()

if (DEFINED ENV{PICO_EXTRAS_FETCH_FROM_GIT_PATH} AND (NOT PICO_EXTRAS_FETCH_FROM_GIT_PATH))
    set(PICO_EXTRAS_FETCH_FROM_GIT_PATH $ENV{PICO_EXTRAS_FETCH_FROM_GIT_PATH})
    message("Using PICO_EXTRAS_FETCH_FROM_GIT_PATH from environment ('${PICO_EXTRAS_FETCH_FROM_GIT_PATH}')")
endif ()

if (NOT PICO_EXTRAS_PATH)
    if (PICO_EXTRAS_FETCH_FROM_GIT)
        include(FetchContent)
        set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
        if (PICO_EXTRAS_FETCH_FROM_GIT_PATH)
            get_filename_component(FETCHCONTENT_BASE_DIR "${PICO_EXTRAS_FETCH_FROM_GIT_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif ()
        FetchContent_Declare(
                pico_extras
                GIT_REPOSITORY https://github.com/raspberrypi/pico-extras
                GIT_TAG master
        )
        if (NOT pico_extras)
            message("Downloading Raspberry Pi Pico Extras")
            FetchContent_Populate(pico_extras)
            set(PICO_EXTRAS_PATH ${pico_extras_SOURCE_DIR})
        endif ()
        set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})
    else ()
        if (PICO_SDK_PATH AND EXISTS "${PICO_SDK_PATH}/../pico-extras")
            set(PICO_EXTRAS_PATH ${PICO_SDK_PATH}/../pico-extras)
            message("Defaulting PICO_EXTRAS_PATH as sibling of PICO_SDK_PATH: ${PICO_EXTRAS_PATH}")
        endif()
    endif ()
endif ()

if (PICO_EXTRAS_PATH)
    set(PICO_EXTRAS_PATH "${PICO_EXTRAS_PATH}" CACHE PATH "Path to the PICO EXTRAS")
    set(PICO_EXTRAS_FETCH_FROM_GIT "${PICO_EXTRAS_FETCH_FROM_GIT}" CACHE BOOL "Set to ON to fetch copy of PICO EXTRAS from git if not otherwise locatable")
    set(PICO_EXTRAS_FETCH_FROM_GIT_PATH "${PICO_EXTRAS_FETCH_FROM_GIT_PATH}" CACHE FILEPATH "location to download EXTRAS")

    get_filename_component(PICO_EXTRAS_PATH "${PICO_EXTRAS_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
    if (NOT EXISTS ${PICO_EXTRAS_PATH})
        message(FATAL_ERROR "Directory '${PICO_EXTRAS_PATH}' not found")
    endif ()

    set(PICO_EXTRAS_PATH ${PICO_EXTRAS_PATH} CACHE PATH "Path to the PICO EXTRAS" FORCE)
    add_subdirectory(${PICO_EXTRAS_PATH} pico_extras)
endif()