#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"


bool shouldWriteInPrimary();
void changePartition();

void set_credentials(char *);
bool get_credentials(char**, char**);

bool wifi_configured();
void write_credentials(char* );

void set_ota_downloaded();
void clear_ota_downloaded();
bool ota_downloaded();