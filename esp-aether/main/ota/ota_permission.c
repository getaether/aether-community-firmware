#include "ota.h"

char permission_response[200];

void update_if_allowed_by_remote() {
    int arrSize = strlen(AE_SERIAL_NUMBER) + strlen(UPDATE_PERMISSION_URL);
	char url[arrSize];
    bool canUpdate = false;

    for(int i = 0; i < arrSize; i++) {
        if (i < strlen(UPDATE_PERMISSION_URL)){
            url[i] =  UPDATE_PERMISSION_URL[i];
        } else {
            url[i] = AE_SERIAL_NUMBER[i - strlen(UPDATE_PERMISSION_URL)];
        }
    }


    esp_http_client_config_t config = {
        .url = url,
        .event_handler = permission_http_event_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if(err != ESP_OK) {
        http_cleanup(client);
        return;
    }

    if (!parse_allowance_json(&canUpdate)) {
        http_cleanup(client);
        return;
    }
        
    http_cleanup(client);

    if (canUpdate) { perform_ota(); }
}

bool parse_allowance_json(bool *result) {

    cJSON* json = cJSON_Parse(permission_response);
    printf("BROW %s\n", permission_response);

    if(json == NULL) { 
        cJSON_Delete(json);
        return false;
    }
    
    cJSON* jVersion = cJSON_GetObjectItemCaseSensitive(json, "canUpdate");

    if(!cJSON_IsNumber(jVersion)) { 
        cJSON_Delete(json);
        return false;
    }

    printf("BROW %d\n", jVersion->valueint);

    (*result) = (jVersion->valueint) == 1;

    cJSON_Delete(json);
    
    return true;
}

esp_err_t permission_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if(!esp_http_client_is_chunked_response(evt->client)) {
                strncpy(permission_response, (char*) evt->data, evt->data_len);
            }
            break;
        default: break;
    }
    
    return ESP_OK;
}

void perform_ota() {
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = getNextUpdatePartition();

    clear_ota_downloaded();
    changePartition();

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
        return;
    }
    ESP_LOGI(TAG, "Prepare to restart system!");

    esp_restart();
    return ;
}
