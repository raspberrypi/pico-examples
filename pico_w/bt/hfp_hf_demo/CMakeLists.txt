add_library(hfp_hf_demo_pins INTERFACE)
target_compile_definitions(hfp_hf_demo_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )
picow_bt_example(hfp_hf_demo pico_btstack_sco_demo_util pico_btstack_sbc_encoder hfp_hf_demo_pins)
