#include <stdio.h>
#include <cstring>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_partition.h"


#include "ota.h"

#define TAG "OTA component"
#define SKIP_VERSION_CHECK 1
OTA::OTA()
{
    update_partition = NULL;
    configured = NULL;
    running = NULL;
}

esp_err_t OTA::init(const char *name)
{
    update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, name);
    return update_partition ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t OTA::init(const esp_partition_t *start_from)
{
    esp_err_t err = ESP_OK;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    ESP_LOGI(TAG, "Init OTA");

    configured = esp_ota_get_boot_partition();
    running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGI(TAG, "Configured OTA boot partition at offset 0x%08lx, but running from offset 0x%08lx",
                 configured->address, running->address);
        ESP_LOGI(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08lx)",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(start_from);
    if (!update_partition)
        return ESP_FAIL;
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
             update_partition->subtype, update_partition->address);

    return err;
}

esp_err_t OTA::begin(size_t size)
{
    esp_err_t err = ESP_OK;
    if (!update_partition)
        return ESP_FAIL;

    err = esp_ota_begin(update_partition, size, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return err;
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    return err;
}

esp_err_t OTA::write(uint8_t *data, size_t len)
{
    esp_err_t err;

    err = esp_ota_write(update_handle, (const void *)data, len);
    if (err != ESP_OK)
    {
        esp_ota_abort(update_handle);
        return err;
    }

    return err;
}

esp_err_t OTA::finish()
{
    esp_err_t err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        else
        {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        return err;
    }

    return err;
}

esp_err_t OTA::compare(uint8_t *data, size_t len)
{
    esp_err_t err = ESP_OK;
    esp_app_desc_t new_app_info;
    if (image_header_was_checked == false)
    {
        if (len > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
        {
            // check current version with downloading
            memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            esp_app_desc_t running_app_info;
            if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
            {
                ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
            }

            const esp_partition_t *last_invalid_app = esp_ota_get_last_invalid_partition();
            esp_app_desc_t invalid_app_info;
            if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
            {
                ESP_LOGE(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
            }

            // check current version with last invalid partition
            if (last_invalid_app != NULL)
            {
                if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
                {
                    ESP_LOGE(TAG, "New version is the same as invalid version.");
                    ESP_LOGE(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                    ESP_LOGE(TAG, "The firmware has been rolled back to the previous version.");
                    return ESP_FAIL;
                }
            }

            image_header_was_checked = true;

            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0)
            {
#ifndef SKIP_VERSION_CHECK
                ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                return ESP_FAIL;
#endif
                ESP_LOGW(TAG, "Current running version is the same as a new. We will continue the update anyway.");
            }
        }
        else
        {
            return ESP_ERR_INVALID_SIZE;
        }
    }
    return err;
}

esp_err_t OTA::boot()
{
    return esp_ota_set_boot_partition(update_partition);
}
