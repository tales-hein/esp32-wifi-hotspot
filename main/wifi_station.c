// External inclusions

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Internal inclusions

#include "common.h"

// Definitions

#define MAX_CONN_RETRIES      10
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1
#define RECEIVED_HOTSPOT_DATA BIT2

// Local variables

static const char *TAG = "WIFI STATION";
static int retry_count = 0;
static const char* WIFI_FILE  = "/spiffs/wifi.txt";
bool in_ap_mode = false;

// Global variables

extern char ssid[32];
extern char pass[64];
extern bool has_wifi;
extern EventGroupHandle_t sta_wifi_event_group;

// Function declarations

void spiffs_append_file(const char* path, const char* data);
void spiffs_erase_content(const char* path);
char* spiffs_read_file(const char *path);
void init_hotspot(void);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (retry_count < MAX_CONN_RETRIES) 
        {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } 
        else 
        {
            xEventGroupSetBits(sta_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(sta_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id
        )
    );
    
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &instance_got_ip
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e        = WPA3_SAE_PWE_HUNT_AND_PECK,
            .sae_h2e_identifier = "",
        },
    };

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(
        sta_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ssid, pass);
        char content[100];
        sniprintf(content, sizeof(content), "ssid=%s&pass=%s", ssid, pass);
        spiffs_erase_content(WIFI_FILE);
        spiffs_append_file(WIFI_FILE, content);
        has_wifi = true;
        in_ap_mode = false;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",ssid, pass);
        ESP_LOGI(TAG, "Initializing hotspot for wifi configuration...");
        has_wifi = false;
        in_ap_mode = true;
        init_hotspot();
    } 
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void check_last_succesful_connection(void)
{
    char* store_wifi_data = spiffs_read_file(WIFI_FILE);

    ESP_LOGI(TAG, "dados:%s", store_wifi_data);

    if (strlen(store_wifi_data) == 0) 
    {
        ESP_LOGI(TAG, "No last successful connection data found... Initializing hotspot for wifi configuration.");
        in_ap_mode = true;
        init_hotspot();
        return;
    }

    char stored_ssid[32] = {0};
    char stored_pass[64] = {0};

    sscanf(store_wifi_data, "ssid=%31[^&]&pass=%63s", stored_ssid, stored_pass);

    if (strcmp(stored_ssid, "") == 0 || strcmp(stored_pass, "") == 0)
    {
        ESP_LOGI(TAG, "No last successful connection data found... Initializing hotspot for wifi configuration.");
        in_ap_mode = true;
        init_hotspot();
        return;
    }

    ESP_LOGI(TAG, "Content retrieved from last successful connection. Using it...");

    strncpy((char *)ssid, stored_ssid, 32);
    strncpy((char *)pass, stored_pass, 64);
}

void init_wifi_sta(void)
{
    check_last_succesful_connection();

    if (in_ap_mode)
    {
        xEventGroupWaitBits(
            sta_wifi_event_group,
            RECEIVED_HOTSPOT_DATA,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta();
}
