add_library(mod_player_pins INTERFACE)
target_compile_definitions(mod_player_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )
picow_bt_example(mod_player pico_btstack_hxcmod_player mod_player_pins)
