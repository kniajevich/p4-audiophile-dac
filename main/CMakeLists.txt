idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES pcm5102a_driver dsp_engine network_manager web_server esp_driver_gpio
)
