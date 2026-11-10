idf_component_register(
    SRCS "ota_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app_update esp_partition esp_system
)
