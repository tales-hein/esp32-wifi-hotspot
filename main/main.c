// External inclusions
#include "esp_system.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Global variables

char ssid[32] = {0};
char pass[64] = {0};
bool has_wifi = 0;
EventGroupHandle_t sta_wifi_event_group;
EventGroupHandle_t mqtt_event_group;

// Function declarations

void init_mqtt(void);
void init_wifi_sta(void);
void init_storage(void);

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_wifi_event_group = xEventGroupCreate();
    mqtt_event_group = xEventGroupCreate();
    init_storage();
    init_wifi_sta();
    init_mqtt();

    while (1)
    {
        ESP_LOGI("MAIN", "loop");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
