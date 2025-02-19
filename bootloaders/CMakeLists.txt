if (TARGET pico_mbedtls)
    # older clang seem to have a segment overlap issue that confuses picotool
    if (PICO_C_COMPILER_IS_CLANG AND CMAKE_C_COMPILER_VERSION VERSION_LESS "17.0.0")
        message("Skipping encrypted bootloader example on LLVM/Clang version < 17; please use GCC or newer LLVM/Clang")
    else()
        add_subdirectory_exclude_platforms(encrypted host rp2040 rp2350-riscv)
    endif()
else()
    # Assume picotool has no signing support, if no pico_mbedtls available
    message("Skipping encrypted bootloader example as pico_mbedtls unavailable")
endif ()
