#include "ota.h"

#include "./ota_common.c"
#include "./ota_download.c"
#include "./ota_permission.c"

void ota_init() {
    printf("Initialized OTA\n");
    xTaskCreatePinnedToCore(&ota_task, "ota_task", 8192, NULL, 5, NULL, 0);
}

void ota_task() {

    while(1) {
        if(ota_downloaded()) {
            ESP_LOGI(TAG, "OTA downloaded! Checking for remote permission");
            update_if_allowed_by_remote();
        } else {
            ESP_LOGI(TAG, "Searching for OTA download");
            download_ota_if_needed();
        }

        ESP_LOGI(TAG, "Sleeping for a min...");  
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}
