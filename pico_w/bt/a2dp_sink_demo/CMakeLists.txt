add_library(a2dp_sink_demo_pins INTERFACE)
target_compile_definitions(a2dp_sink_demo_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )

picow_bt_example(a2dp_sink_demo pico_btstack_sbc_decoder a2dp_sink_demo_pins)
