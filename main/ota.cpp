#include "ota.h"
#include "esp_check.h"

#include "global.h"

#define TAG "ota"

static OTA ota;

esp_err_t ota_update_start(const char *name, size_t len)
{
    ESP_RETURN_ON_ERROR(ota.init(name), TAG, "failed to init: %s", name);
    ESP_RETURN_ON_ERROR(ota.begin(len), TAG, "failed to begin: %d", len);

    return ESP_OK;
}

bool ota_update_write(char *data, size_t len)
{
    auto ret = ota.write((uint8_t*)data, len);
    return ret == ESP_OK;
}

esp_err_t ota_update_finish()
{
    ESP_RETURN_ON_ERROR(ota.finish(), TAG, "failed to finish");
    ESP_RETURN_ON_ERROR(ota.boot(), TAG, "failed to boot");
    return ESP_OK;
}

esp_err_t boot_from_partition(const char *name)
{
    ESP_RETURN_ON_ERROR(ota.init(name), TAG, "failed to init: %s", name);
    ESP_RETURN_ON_ERROR(ota.boot(), TAG, "failed to boot");

    return ESP_OK;
}
