idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES pcm5102a_driver dsp_engine audio_decoders network_manager web_server sd_storage ota_manager oled_display esp_driver_gpio
)
