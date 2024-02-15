add_library(sine_player_pins INTERFACE)
target_compile_definitions(sine_player_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )
picow_bt_example(sine_player sine_player_pins)
