#include "sd_storage.hpp"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include <dirent>

static const char* TAG = "SD_STORAGE";

esp_err_t SDStorage::init() {
    ESP_LOGI(TAG, "Initializing 4-bit High-Speed SDMMC MicroSD Interface...");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;

    sdmmc_card_t* card = nullptr;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        m_mounted = true;
        ESP_LOGI(TAG, "MicroSD mounted successfully at /sdcard (Size: %llu MB)",
                 ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    } else {
        ESP_LOGW(TAG, "MicroSD mount omitted or card not present (code: 0x%x)", ret);
    }
    return ret;
}

std::vector<std::string> SDStorage::listAudioFiles(const char* path) {
    std::vector<std::string> files;
    if (!m_mounted) return files;
    DIR* dir = opendir(path);
    if (!dir) return files;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;
        if (name.find(".flac") != std::string::npos || name.find(".wav") != std::string::npos ||
            name.find(".dsf") != std::string::npos  || name.find(".irs") != std::string::npos) {
            files.push_back(name);
        }
    }
    closedir(dir);
    return files;
}

esp_err_t SDStorage::loadIRSFile(const std::string& filename, std::vector<float>& out_ir_l, std::vector<float>& out_ir_r) {
    if (!m_mounted) return ESP_FAIL;
    std::string full_path = "/sdcard/irs/" + filename;
    FILE* f = fopen(full_path.c_str(), "rb");
    if (!f) return ESP_ERR_NOT_FOUND;

    // Read basic raw 16/32-bit samples (2048 taps fallback demo)
    const size_t taps = 2048;
    out_ir_l.resize(taps, 0.0f);
    out_ir_r.resize(taps, 0.0f);
    for (size_t i = 0; i < taps; ++i) {
        int16_t sample = 0;
        if (fread(&sample, sizeof(int16_t), 1, f) > 0) {
            float val = static_cast<float>(sample) / 32768.0f;
            out_ir_l[i] = val;
            out_ir_r[i] = val;
        }
    }
    fclose(f);
    return ESP_OK;
}
