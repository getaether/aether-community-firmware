#include "./aehomekit.h"

const int led_gpio = 2;
bool led_on = false;


void led_write(bool on) {
    gpio_set_level(led_gpio, on ? 1 : 0);
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        return;
    }

    led_on = value.bool_value;
    led_write(led_on);
}


void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write(led_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    xTaskCreate(led_identify_task, "LED identify", 512, NULL, 2, NULL);
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, PRODUCT_NAME),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Aether Inc"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, AE_SERIAL_NUMBER),
            HOMEKIT_CHARACTERISTIC(MODEL, "Lumni"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, VERSION_NUMBER_STR),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, PRODUCT_NAME),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=led_on_get,
                .setter=led_on_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = AE_PASSWORD,
    .setupId=AE_SETUP_ID,
};

void led_init() {
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
    led_write(led_on);
}

void homekit_init() {
    ESP_LOGI("AE HOMEKIT", "Initialized HomeKit\n");
    homekit_server_init(&config);
}