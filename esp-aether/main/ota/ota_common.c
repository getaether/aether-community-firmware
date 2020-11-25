#include "ota.h"


static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    // (void)vTaskDelete(NULL);
}

