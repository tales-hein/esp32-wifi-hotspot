#include <string.h>
#include "esp_spiffs.h"
#include "esp_log.h"

void spiffs_append_file(const char *path, const char *data) 
{
    FILE *file = fopen(path, "a");
    if (!file)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for appending");
        return;
    }

    if (fwrite(data, sizeof(char), strlen(data), file) == 0)
    {
        ESP_LOGE("SPIFFS", "Failed to write data to file");
    } 
    else
    {
        ESP_LOGI("SPIFFS", "Appended data to file successfully");
    }

    fclose(file);
}

void spiffs_erase_content(const char *path)
{
    FILE *file = fopen(path, "w");
    if (!file)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for erasing");
        return;
    }
    fclose(file);
}

char* spiffs_read_file(const char *path) 
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    ssize_t file_size = ftell(file);
    rewind(file);

    char* file_content = (char*) malloc(file_size + 1);
    if (!file_content)
    {
        ESP_LOGE("SPIFFS", "Failed to allocate memory for file content");
        fclose(file);
        return NULL;
    }

    ssize_t bytes_read = fread(file_content, sizeof(char), file_size, file);
    if (bytes_read < 0)
    {
        ESP_LOGE("SPIFFS", "Failed to read file content");
        free(file_content);
        fclose(file);
        return NULL;
    }

    file_content[bytes_read] = '\0';

    fclose(file);

    return file_content;
}

void init_storage() 
{
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
