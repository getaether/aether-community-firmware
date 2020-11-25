#include <esp_event_loop.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include <driver/gpio.h>

#include "../constants.h"

void homekit_init();
void led_init();