idf_component_register(
    SRCS "sd_storage.cpp"
    INCLUDE_DIRS "include"
    REQUIRES fatfs esp_driver_sdmmc vfs
)
