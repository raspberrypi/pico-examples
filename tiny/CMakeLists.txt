add_executable(blink_tiny
        blink_tiny.c
)

# pull in common dependencies
target_link_libraries(blink_tiny pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(blink_tiny)

# call pico_set_program_url to set path to example on github, so users can find the source for an example via picotool
example_auto_set_url(blink_tiny)

# Useful CMKake function to help minimize the runtime; we are disabling everything - be careful as disabled things obviously wont work
#
# pico_minimize_runtime(blink_tiny) is equivalent (i.e. everything defaults to off)
#
# you could do something like the following to include standard stuff except the default alarm pool, single precision
#   floating point support and double precision floating point support.
# pico_minimize_runtime(blink_tiny INCLUDE ALL EXCLUDE DEFAULT_ALARM_POOL FLOAT DOUBLE)
pico_minimize_runtime(blink_tiny EXCLUDE ALL)

# More space savings can be made by skipping various bits of the runtime_init (you could alternatively replace the
# implementations as needed either via a non weak function of the appropriate name - note you can omit the default
# weak implementation via PICO_RUNTIME_NO_INIT_XXX=1)
#
# you should be even more careful here skipping bits of runtime initialization code
target_compile_definitions(blink_tiny PRIVATE
    PICO_RUNTIME_SKIP_INIT_CLOCKS=1 # we can run with the bootrom clock setup
        PICO_RUNTIME_SKIP_POST_CLOCK_RESETS=1 # hangs without init clocks - means we get no clocked peripherals
    PICO_RUNTIME_SKIP_INIT_USB_POWER_DOWN=1 # don't care about USB power usage
    PICO_RUNTIME_SKIP_INIT_PER_CORE_ENABLE_COPROCESSORS=1 # don't need any coprocessors (float, DCP, or GPIO)
        PICO_USE_GPIO_COPROCESSOR=0 # we'd have to init it if we used it
    PICO_RUNTIME_SKIP_INIT_RP2040_GPIO_IE_DISABLE=1 # really don't care
    PICO_RUNTIME_SKIP_INIT_SPIN_LOCKS_RESET=1 # we don't use spin locks so resetting them is not necessary
        PICO_USE_SW_SPIN_LOCKS=0 # no need to sw spin locks on RP2350 as we don;t use them or the affected SIO regions
    PICO_RUNTIME_SKIP_INIT_BOOT_LOCKS_RESET=1 # we don't use boot locks so resetting them is not necessary
    PICO_RUNTIME_SKIP_INIT_BOOTROM_LOCKING_ENABLE=1 # we don't call bootrom functions requiring locking
    PICO_RUNTIME_SKIP_INIT_PER_CORE_IRQ_PRIORITIES=1 # no IRQ use
        PICO_NO_RAM_VECTOR_TABLE=1 # so no need for ram vector table
        PICO_RUNTIME_SKIP_INIT_PER_CORE_H3_IRQ_REGISTERS=1 # no need to setup RISC-V IRQ handling
)

if (PICO_C_COMPILER_IS_GNU AND PICO_ON_DEVICE)
    # pending resolution of https://github.com/raspberrypi/pico-sdk/issues/1368 we can save space by explicitly
    # disabling startfiles at least for tiny_blink
    target_link_options(blink_tiny PRIVATE "-nostartfiles")
endif()

# we can save more by disabling BINARY_INFO if you don't want it, however we leave binary_info on by default
# pass -DBLINK_TINY_NO_BINARY_INFO=1 to CMake to disable it
if (BLINK_TINY_NO_BINARY_INFO)
    target_compile_definitions(blink_tiny PRIVATE
            PICO_NO_BINARY_INFO=1
    )
endif()