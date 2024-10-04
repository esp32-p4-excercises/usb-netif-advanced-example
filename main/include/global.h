#pragma once

#include "esp_err.h"

esp_err_t ota_update_start(const char* name, size_t len);
bool ota_update_write(char* data, size_t len);
esp_err_t ota_update_finish();
esp_err_t boot_from_partition(const char *name);

