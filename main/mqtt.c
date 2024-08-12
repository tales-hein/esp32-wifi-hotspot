#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_mac.h"

// Definitions

#define BROKER_URI       "mqtt://192.168.18.20:1883"
#define WIFI_PROVISIONED BIT3

// Global variables

extern EventGroupHandle_t mqtt_event_group;

// Local variables

uint8_t status         = 1;
bool who_am_i_sent     = false;
bool signal_emiter     = false;
bool interrupt_flag    = false;
static const char *TAG = "MQTT_TEST";

static esp_mqtt_client_handle_t client;
uint8_t mac[6];
char mac_str[18];

static void publish_who_am_i(void)
{ 
    int msg_id = esp_mqtt_client_publish(
        client,
        "sources/",
        mac_str,
        0,
        0,
        0
    );

    if (msg_id < 0) 
    {
        ESP_LOGI("MQTT", "Error while trying to send who_am_i publish message, msg_id=%d", msg_id);
    } 
    else 
    {
        who_am_i_sent = true;
        ESP_LOGI("MQTT", "Sent who_am_i publish successful, msg_id=%d", msg_id);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    char topic[64];
    
    snprintf(topic, sizeof(topic), "response/%s/", mac_str);

    switch (event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            
            printf("Subscribing to topic: %s\r\n", topic);
            
            esp_mqtt_client_subscribe(client, topic, 0);
            
            publish_who_am_i();

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("Received topic: %.*s\r\n", event->topic_len, event->topic);
            printf("Received payload: %.*s\r\n", event->data_len, event->data);
            break;
        default:
            break;
    }
}

void publish_payload(char* payload_content, char* topic)
{ 
    char buffer[64];
    snprintf(buffer, 64, "%s/%s", topic, mac_str);

    int msg_id = esp_mqtt_client_publish(
        client,
        buffer,
        payload_content,
        0,
        0,
        0
    );

    if (msg_id < 0) 
    {
        ESP_LOGI("MQTT", "Error while trying to publish message, msg_id=%d", msg_id);
    } 
    else 
    {
        ESP_LOGI("MQTT", "Sent publish successful, msg_id=%d", msg_id);
    }
}

void init_mqtt(void)
{
    esp_err_t mac_ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (mac_ret != ESP_OK) 
    {
        ESP_LOGE("MAC Address", "Failed to get MAC address");
        return;
    }
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker = {
            .address.uri    = BROKER_URI,
        },
        .session.keepalive  = 60,
        .network.timeout_ms = 1000,
    };

    xEventGroupWaitBits(
        mqtt_event_group,
        WIFI_PROVISIONED,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}
