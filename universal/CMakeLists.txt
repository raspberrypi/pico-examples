if ((NOT DEFINED PICO_RISCV_TOOLCHAIN_PATH) OR (NOT DEFINED PICO_ARM_TOOLCHAIN_PATH))
    # Must define PICO_RISCV_TOOLCHAIN_PATH and PICO_ARM_TOOLCHAIN_PATH
    # to ensure both compilers are present
    message(
        "Skipping universal examples as PICO_RISCV_TOOLCHAIN_PATH and "
        "PICO_ARM_TOOLCHAIN_PATH are not defined"
        )
    return()
endif()

include(ExternalProject)

# The way this universal build works is it builds separate binaries for each platform,
# then links them into a single block loop. This universal binary will then run on any
# platform
#  - On RP2040 the bootrom will just execute the RP2040 binary at the start of flash
#  - On RP2350 the bootrom will search the block loop for the appropriate IMAGE_DEF for
#    Arm/RISC-V, translate it's address to the start of flash using the rolling window
#    delta, and execute it on the appropriate CPU
#
# The build will output a TARGET.bin file which can be written using picotool, and a
# TARGET.uf2 file which can be dragged and dropped onto the device in BOOTSEL mode
#
# If SEPARATE_RP2040 is set, the RP2040 binary will not be included in the block loop,
# so the RP2040 UF2 file will only contain the RP2040 binary, and the RP2350 UF2 file will
# contain the combined binary excluding the RP2040 binary.
function (add_universal_target TARGET SOURCE)
    set(zeroValueArgs SEPARATE_RP2040)
    set(oneValueArgs SOURCE_TARGET PADDING PACKADDR BOARD_RP2040 BOARD_RP2350)
    set(multiValueArgs PLATFORMS)
    cmake_parse_arguments(PARSE_ARGV 2 PARSED "${zeroValueArgs}" "${oneValueArgs}" "${multiValueArgs}")

    set(SOURCE_TARGET ${TARGET})
    if (PARSED_SOURCE_TARGET)
        set(SOURCE_TARGET ${PARSED_SOURCE_TARGET})
    endif()
    set(PADDING 0x1000)
    if (PARSED_PADDING)
        set(PADDING ${PARSED_PADDING})
    endif()
    set(PACKADDR 0x10000000)
    if (PARSED_PACKADDR)
        set(PACKADDR ${PARSED_PACKADDR})
    endif()
    set(PLATFORMS "rp2040;rp2350-arm-s;rp2350-riscv")
    if (PARSED_PLATFORMS)
        set(PLATFORMS ${PARSED_PLATFORMS})
    endif()
    # these could be set as CMake variables, so only override if explicitly passed as arguments
    if (PARSED_BOARD_RP2040)
        set(PICO_BOARD_RP2040 ${PARSED_BOARD_RP2040})
    endif()
    if (PARSED_BOARD_RP2350)
        set(PICO_BOARD_RP2350 ${PARSED_BOARD_RP2350})
    endif()
    # rp2040 must come first, as that has checksum requirements at the start of the binary
    list(FIND PLATFORMS "rp2040" idx)
    if (idx GREATER 0)
        message(FATAL_ERROR "rp2040 must come first in PLATFORMS for universal binaries")
    endif()

    add_custom_target(${TARGET} ALL)
    if (picotool_DIR)
        set(universal_picotool_DIR ${picotool_DIR})
    else()
        set(universal_picotool_DIR ${picotool_INSTALL_DIR}/picotool)
    endif()
    if (pioasm_DIR)
        set(universal_pioasm_DIR ${pioasm_DIR})
    else()
        set(universal_pioasm_DIR ${PIOASM_INSTALL_DIR}/pioasm)
    endif()

    set(DEPS "")
    set(BINS "")
    # Build binaries for each of the platforms
    foreach(platform IN LISTS PLATFORMS)
        ExternalProject_Add(${TARGET}_${platform}
            PREFIX pioasm
            SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/wrapper
            BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/${platform}/wrapper
            CMAKE_ARGS
                "--no-warn-unused-cli"
                "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                "-DPICO_SDK_PATH:FILEPATH=${PICO_SDK_PATH}"
                "-DPICO_EXAMPLES_PATH:FILEPATH=${PICO_EXAMPLES_PATH}"
                "-DPICO_PLATFORM=${platform}"
                "-DPICO_ARM_TOOLCHAIN_PATH=${PICO_ARM_TOOLCHAIN_PATH}"
                "-DPICO_RISCV_TOOLCHAIN_PATH=${PICO_RISCV_TOOLCHAIN_PATH}"
                "-DPICO_BOARD_RP2040=${PICO_BOARD_RP2040}"
                "-DPICO_BOARD_RP2350=${PICO_BOARD_RP2350}"
                "-DUNIVERSAL_PROJECT_DIR:FILEPATH=${SOURCE}"
                "-DUNIVERSAL_BINARY_DIR:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/${platform}"
                "-DSOURCE_TARGET=${SOURCE_TARGET}"
                "-Dpicotool_DIR=${universal_picotool_DIR}"
                "-Dpioasm_DIR=${universal_pioasm_DIR}"
            BUILD_ALWAYS 1 # force dependency checking
            INSTALL_COMMAND ""
            )

        set(ORIGINAL_BIN ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/${platform}/${SOURCE_TARGET}.bin)

        list(APPEND DEPS ${TARGET}_${platform})
        list(APPEND BINS ${ORIGINAL_BIN})
    endforeach()

    set(BINDIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET})
    set(COMBINED ${BINDIR}/${TARGET}.bin)

    pico_init_picotool()
    if (NOT picotool_FOUND)
        message(FATAL_ERROR "Cannot link universal binary without picotool")
    endif()

    list(FIND PLATFORMS "rp2040" idx)
    if (idx EQUAL 0 AND PARSED_SEPARATE_RP2040)
        # Don't include RP2040 bin in the combined BIN, just include it in the RP2040 UF2
        # POP_FRONT only added in CMake 3.15, so use GET and REMOVE_AT instead
        list(GET BINS 0 RP2040_BIN)
        list(REMOVE_AT BINS 0)
        set(RP2040_COMBINED ${RP2040_BIN})
    else()
        set(RP2040_COMBINED ${COMBINED})
    endif()

    list(LENGTH BINS BINS_COUNT)
    if (BINS_COUNT GREATER 1)
        # Link the binaries for different platforms into a single block loop, with
        # appropriate rolling window deltas. This creates a universal binary file,
        # which will run on any of the platforms when loaded using picotool.
        add_custom_target(${TARGET}_combined
                COMMAND picotool link ${COMBINED} ${BINS} --pad ${PADDING}
                DEPENDS ${DEPS}
                )
    else()
        # Only one binary left, so no picotool link is needed - just copy instead
        # This could be the case if only building for rp2040 and rp2350-arm-s,
        # with SEPARATE_RP2040 set
        add_custom_target(${TARGET}_combined
                COMMAND ${CMAKE_COMMAND} -E copy ${BINS} ${COMBINED}
                DEPENDS ${DEPS}
                )
    endif()

    # Create UF2s targeting the absolute and rp2040 family IDs, then combine these
    # into a single universal UF2. This is required as there isn't a single family
    # ID which is accepted by both RP2040 and RP2350. Instead, the 2 UF2 files are
    # concatenated together and the device ignores the part not targeting it.
    add_custom_target(${TARGET}_absolute_uf2
            COMMAND picotool uf2 convert ${COMBINED} ${BINDIR}/absolute.uf2 --family absolute --offset ${PACKADDR}
            DEPENDS ${TARGET}_combined
            )
    add_custom_target(${TARGET}_rp2040_uf2
            COMMAND picotool uf2 convert ${RP2040_COMBINED} ${BINDIR}/rp2040.uf2 --family rp2040 --offset ${PACKADDR}
            DEPENDS ${TARGET}_combined
            )

    if (CMAKE_HOST_UNIX)
        # If you just concatenate the files, then on Windows you get an alarming 'write error' dialog when copying
        # large UF2s, because the device reboots too soon. Moving the final RP2040 UF2 block to the end of the file
        # fixes this, as it won't reboot until the final block is received. This fix requires `tail` and `truncate`,
        # so only use it when building on Unix-like systems.
        add_custom_target(${TARGET}_uf2
                COMMAND tail -c 512 ${BINDIR}/rp2040.uf2 > ${BINDIR}/rp2040.end.uf2
                COMMAND cat ${BINDIR}/rp2040.uf2 > ${BINDIR}/rp2040.start.uf2
                COMMAND truncate --size=-512 ${BINDIR}/rp2040.start.uf2
                COMMAND cat ${BINDIR}/rp2040.start.uf2 ${BINDIR}/absolute.uf2 ${BINDIR}/rp2040.end.uf2 > ${BINDIR}/${TARGET}.uf2
                DEPENDS ${TARGET}_absolute_uf2 ${TARGET}_rp2040_uf2
                )
    else()
        # Not guaranteed to have `tail` or `truncate`, so just concatenate the files as before - this still loads
        # the file onto the device correctly.
        add_custom_target(${TARGET}_uf2
                COMMAND ${CMAKE_COMMAND} -E cat ${BINDIR}/rp2040.uf2 ${BINDIR}/absolute.uf2 > ${BINDIR}/${TARGET}.uf2
                DEPENDS ${TARGET}_absolute_uf2 ${TARGET}_rp2040_uf2
                )
    endif()

    add_dependencies(${TARGET} ${TARGET}_combined ${TARGET}_uf2)
endfunction()

# hello_universal binary
add_universal_target(hello_universal
    ${CMAKE_CURRENT_LIST_DIR}/hello_universal
    )

# blink binary
add_universal_target(blink_universal
    ${CMAKE_CURRENT_LIST_DIR}/blink_universal
    SOURCE_TARGET blink_universal
    BOARD_RP2040 universal
    BOARD_RP2350 universal
    # Skip RISC-V and keep RP2040 separate, as wifi firmware takes up lots of space
    PLATFORMS "rp2040;rp2350-arm-s"
    SEPARATE_RP2040
    )

# nuke binary - is no_flash, so needs to be sent to SRAM on RP2040
add_universal_target(nuke_universal
    ${CMAKE_CURRENT_LIST_DIR}/../flash/nuke
    SOURCE_TARGET flash_nuke
    PACKADDR 0x20000000
    )
