#include "./ota.h"

esp_partition_t *getNextUpdatePartition();

static char ota_write_data[BUFFSIZE + 1] = { 0 };
char ota_response[200];



void download_ota_if_needed() {
    char url[200] = { 0 };

    if(!is_outdated_version(url))  { return; }
    
    download_ota(url); 

    set_ota_downloaded();
}

bool parse_download_json(double* version, char* url) {
    cJSON* json = cJSON_Parse(ota_response);

    if(json == NULL) {
        return false;
    }
    

    cJSON* jVersion = cJSON_GetObjectItemCaseSensitive(json, "version");
    cJSON* jUrl = cJSON_GetObjectItemCaseSensitive(json, "url");

    if(!cJSON_IsNumber(jVersion)) {
        cJSON_Delete(json);
        return false;
    }

    if( !(cJSON_IsString(jUrl) && (jUrl->valuestring != NULL)) )  {
        cJSON_Delete(json);
        return false;
    }

    double unpackedVersion = (jVersion->valuedouble);
    char * unpackedURL = jUrl -> valuestring;

    for(int i = 0; i < strlen(unpackedURL); i++) {
        url[i] =  unpackedURL[i];
    }

    *version = unpackedVersion;
    cJSON_Delete(json);

    return true;
}

esp_err_t download_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if(!esp_http_client_is_chunked_response(evt->client)) {
                
                strncpy(ota_response, (char*) evt->data, evt->data_len);
                
            }

            break;
        default: break;
    }
    
    return ESP_OK;
}

bool is_outdated_version(char* url) {

    double version;

    esp_http_client_config_t config = {
        .url = UPDATE_JSON_URL,
        .event_handler = download_http_event_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if(err != ESP_OK) {
        
        http_cleanup(client);
        return false;
    }

    if(!parse_download_json(&version, url)) { 
        http_cleanup(client);
        
        return false;
    }

    http_cleanup(client);
    return version > VERSION_NUMBER;
}

static void download_ota(char* update_url)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = update_url,
    };

    config.skip_cert_common_name_check = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
        return;
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
        return;
    }
    esp_http_client_fetch_headers(client);

    update_partition = getNextUpdatePartition();

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
            return;
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                        return;
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                    return;
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                printf("AE BROW %d\n", err);
                continue;
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        task_fatal_error();
        return;
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
        return;
    }

    http_cleanup(client);
}

esp_partition_t *getNextUpdatePartition() {
    esp_partition_t * res = NULL;

    if(shouldWriteInPrimary()) {
        res = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "part_0");
    } else {
        res = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "part_1");
    }

    ESP_LOGE(TAG, "Writing to partition subtype %d at offset 0x%x",
             res->subtype, res->address);
    return res;
}

