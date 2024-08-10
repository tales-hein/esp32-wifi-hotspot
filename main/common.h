// External inclusions

#include "freertos/event_groups.h"

// WIFI related global variables

    /* Definitions */
#define WIFI_ON  1
#define WIFI_OFF 0
#define WIFI_FILE "/spiffs/wifi.txt"

    /* Variables */
char* ssid                                     = "";
char* pass                                     = "";
unsigned char has_wifi                         = 0;
static EventGroupHandle_t sta_wifi_event_group = NULL;

// MQTT related global variables

    /* Variables */
// static EventGroupHandle_t mqtt_event_group = NULL;
