// External inclusions

#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "../../../../../esp/esp-idf/components/spiffs/spiffs/src/spiffs.h"

void spiffs_write_file(const char *path, const char *data) 
{
    spiffs_file file = SPIFFS_open(NULL, path, SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);
    if (file < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for writing");
        return;
    }

    ssize_t bytes_written = SPIFFS_write(NULL, file, (void *)data, strlen(data));
    if (bytes_written < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to write data to file");
    } 
    else 
    {
        ESP_LOGI("SPIFFS", "File written successfully");
    }

    SPIFFS_close(NULL, file);
}

char* spiffs_read_file(const char *path) 
{
    spiffs_file file = SPIFFS_open(NULL, path, SPIFFS_O_RDONLY, 0);
    if (file < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return NULL;
    }

    ssize_t file_size = SPIFFS_lseek(NULL, file, 0, SPIFFS_SEEK_END);
    if (file_size < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to seek to end of file");
        SPIFFS_close(NULL, file);
        return NULL;
    }
    SPIFFS_lseek(NULL, file, 0, SPIFFS_SEEK_SET);

    char* file_content = (char*) malloc(file_size + 1);
    if (!file_content)
    {
        ESP_LOGE("SPIFFS", "Failed to allocate memory for file content");
        SPIFFS_close(NULL, file);
        return NULL;
    }

    ssize_t bytes_read = SPIFFS_read(NULL, file, file_content, file_size);
    if (bytes_read < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to read file content");
        free(file_content);
        SPIFFS_close(NULL, file);
        return NULL;
    }

    file_content[bytes_read] = '\0';

    SPIFFS_close(NULL, file);

    return file_content;
}


void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE("SPIFFS", "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE("SPIFFS", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
    }
}