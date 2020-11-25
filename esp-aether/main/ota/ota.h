#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "errno.h"
#include "cJSON.h"

#include "../constants.h"
#include "../persistance/persistance.h"

#define BUFFSIZE 1024
#define HASH_LEN 32
#define OTA_URL_SIZE 256
#define TAG "AE OTA"

void ota_init();
void ota_task();

void download_ota_if_needed();
void update_if_allowed_by_remote();

void ota_check_download_update();
void ota_check_update_allowed();

void perform_ota();

bool is_outdated_version(char *);

bool parse_allowance_json(bool *);
bool parse_download_json(double *, char *);

esp_err_t download_http_event_handler(esp_http_client_event_t *);
esp_err_t permission_http_event_handler(esp_http_client_event_t *);

static void download_ota(char *url);

static void http_cleanup(esp_http_client_handle_t client);
static void task_fatal_error(void);