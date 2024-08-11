// External inclusions
#include "esp_system.h"
#include "esp_event.h"

// Global variables initialization

char* ssid                              = "";
char* pass                              = "";
unsigned char has_wifi                  = 0;
EventGroupHandle_t sta_wifi_event_group = NULL;

// Function declarations
// void init_mqtt(void);
void init_wifi_sta(void);
void init_spiffs(void);

// --------------------------------------------- //

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    init_spiffs();
    init_wifi_sta();
    // init_mqtt();
}