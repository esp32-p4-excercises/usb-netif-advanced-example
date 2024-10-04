#pragma once

#include "esp_ota_ops.h"


class OTA
{
private:
    const esp_partition_t *update_partition;
    const esp_partition_t *configured;
    const esp_partition_t *running;
    esp_ota_handle_t update_handle = 0;
    bool image_header_was_checked = false;

public:
    OTA();
    ~OTA() {}

public:
    esp_err_t init(const char* name);
    esp_err_t init(const esp_partition_t *start_from = NULL);
    esp_err_t begin(size_t size = OTA_SIZE_UNKNOWN);
    esp_err_t write(uint8_t* bytes, size_t len);
    esp_err_t compare(uint8_t* data, size_t len);
    esp_err_t boot();
    esp_err_t finish();
};

