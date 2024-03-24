#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define ESP_CHANNEL 1
#define Unlock_io 14

static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static const char *TAG = "esp_now_init";

static esp_err_t init_wifi()
{
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    esp_netif_init();
    esp_event_loop_create_default();
    nvs_flash_init();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_start();

    ESP_LOGI(TAG, "wifi init complete");
    return ESP_OK;
}

void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    ESP_LOGI(TAG, "Data recived: " MACSTR " %s ", MAC2STR(esp_now_info->src_addr), data);
    if (strcmp((char*)data, "Open") == 0)
    {
        ESP_LOGI(TAG, "Open");
    }
    else if (strcmp((char*)data, "Close") == 0)
    {
        ESP_LOGI(TAG, "Close");
    }
    else if (strcmp((char*)data, "Alarm") == 0)
    {
        ESP_LOGI(TAG, "Alarm");
    }
    else
    {
        ESP_LOGE(TAG, "DATA is not OK");
    }
}

void sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCESS");
    }
    else
    {
        ESP_LOGW(TAG, "ESP_NOW_SEND_FAIL");
    }
}

static esp_err_t init_esp_now()
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);
    esp_now_register_send_cb(sent_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}

static esp_err_t register_peer(uint8_t *peer_addr)
{
    esp_now_peer_info_t esp_now_peer_info = {};
    memcpy(esp_now_peer_info.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    esp_now_peer_info.channel = ESP_CHANNEL;
    esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;

    esp_now_add_peer(&esp_now_peer_info);
    return ESP_OK;
}


void app_main(void)
{

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL<<Unlock_io;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_peer(peer_mac));

    uint8_t data [] = "Open";

    while (true)
    {
        if (!gpio_get_level(Unlock_io)){
            ESP_ERROR_CHECK(esp_now_send(peer_mac, data, sizeof(data)));
            vTaskDelay(200/portTICK_PERIOD_MS);
        }
    }
    
    
}