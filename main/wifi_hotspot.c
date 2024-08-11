#include <esp_wifi.h>
#include <esp_http_server.h>
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Local variables

#define SOFT_AP_SSID          "ESP32"
#define SOFT_AP_PASS          "12345678"
#define WIFI_TAG              "WIFI_HOTSPOT"
#define RECEIVED_HOTSPOT_DATA BIT2

// Global variables

extern char ssid[32];
extern char pass[64];
extern EventGroupHandle_t sta_wifi_event_group;

// Function declarations

esp_err_t webserver_handler(httpd_req_t *req);
void init_wifi_sta(void);
int min(int x, int y);

httpd_uri_t connect_uri = {
    .uri       = "/connect",
    .method    = HTTP_POST,
    .handler   = webserver_handler,
    .user_ctx  = NULL
};

httpd_uri_t wifi_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = webserver_handler,
    .user_ctx  = NULL
};

esp_err_t webserver_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) 
    {
        const char *response =
            "<!DOCTYPE html>"
            "<html lang=\"en\">"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>Wi-Fi Configuration</title>"
            "</head>"
            "<body>"
            "<h1>Enter Wi-Fi Credentials</h1>"
            "<form action=\"/connect\" method=\"post\">"
            "SSID: <input type=\"text\" name=\"ssid\"><br>"
            "Password: <input type=\"password\" name=\"password\"><br>"
            "<input type=\"submit\" value=\"Connect\">"
            "</form>"
            "</body>"
            "</html>";
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    } 
    else if (req->method == HTTP_POST)
    {
        char buf[100];
        int ret, remaining = req->content_len;
        while (remaining > 0)
        {
            if ((ret = httpd_req_recv(req, buf, min(remaining, sizeof(buf)))) <= 0)
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    continue;
                }
                return ESP_FAIL;
            }
            remaining -= ret;
        }

        buf[req->content_len] = '\0';
        ESP_LOGI(WIFI_TAG, "Received data: %s", buf);

        sscanf(buf, "ssid=%31[^&]&password=%63s", ssid, pass);

        if (strlen(ssid) == 0 || strlen(pass) == 0)
        {
            ESP_LOGI(WIFI_TAG, "SSID or password is empty. Ignoring connection attempt.");
            httpd_resp_sendstr(req, "Invalid Wi-Fi credentials received.");
            return ESP_OK;
        }

        httpd_resp_sendstr(req, "Wi-Fi credentials received. Trying to connect...");

        ESP_LOGI(WIFI_TAG, "Connecting to SSID:%s with password:%s", ssid, pass);
        
        xEventGroupSetBits(sta_wifi_event_group, RECEIVED_HOTSPOT_DATA);

        return ESP_OK;
    }
    return ESP_FAIL;
}

void start_webserver(void) 
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &wifi_uri);
        httpd_register_uri_handler(server, &connect_uri);
    }
}

void init_hotspot(void) 
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid           = SOFT_AP_SSID,
            .password       = SOFT_AP_PASS,
            .ssid_len       = strlen(SOFT_AP_SSID),
            .channel        = 1,
            .max_connection = 2,
            .authmode       = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(SOFT_AP_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());

    start_webserver();
}