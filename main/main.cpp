#include <stdio.h>


#include "esp_log.h"
#include "esp_event.h"
#include "esp_check.h"
#include "nvs_flash.h"

#include "esp_netif.h"
#include "esp_event.h"


#include "usb_netif.h"

static const char *TAG = "USB_NCM";

void start_webserver();
void mount_filesystem();

extern "C"
void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    usb_ip_init_default_config();

    mount_filesystem();
    start_webserver();

    ESP_LOGI(TAG, "USB NCM initialized and started");
    return;
}
