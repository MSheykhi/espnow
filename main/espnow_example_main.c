#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "string.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"

static const char *TAG = "Main";

#define LED_GPIO 18               // Change this to your specific GPIO pin used for power control
#define GPIO_POWER_PIN GPIO_NUM_1 // Change this to your specific GPIO pin used for power control
// Assuming ADC channels can be mapped directly to GPIO numbers
int gpio_pins_for_adc_channels[] = {GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3}; // Example GPIO numbers, replace with actual mappings

#define GPIO_POWER_PIN_SEL ((1ULL << GPIO_NUM_1))
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_NUM_0) | (1ULL<<GPIO_NUM_1) | (1ULL<<GPIO_NUM_2) | (1ULL<<GPIO_NUM_3))

#define ESP_CHANNEL 1
#define Unlock_io 14

static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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
    if (strcmp((char *)data, "Open") == 0)
    {
        ESP_LOGI(TAG, "Open");
    }
    else if (strcmp((char *)data, "Close") == 0)
    {
        ESP_LOGI(TAG, "Close");
    }
    else if (strcmp((char *)data, "Alarm") == 0)
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

void set_power(bool power)
{
    if (power)
    {
        gpio_reset_pin(GPIO_POWER_PIN);
        gpio_set_direction(GPIO_POWER_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_POWER_PIN, 0);
    }
    else
    {
        gpio_reset_pin(GPIO_POWER_PIN);
        gpio_set_direction(GPIO_POWER_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_POWER_PIN, 1);
    }
}

void app_main(void)
{

    // Set GPIO for LED control

    // Initialize ADC for channel 0, 1, 2, 3 (Assuming ADC1)
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t adc1_init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&adc1_init_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_WIDTH_BIT_12,
    };

    adc_channel_t channel[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3};
    for (int i = 0; i < 4; i++)
    {
        adc_oneshot_config_channel(adc1_handle, channel[i], &chan_cfg);
    }

    // Read ADC values and calculate voltages
    int adc_values[4];
    float adc_voltages[4];
    float reference_voltage = 3.9; // Reference voltage for ADC_ATTEN_DB_11
    int resolution = 4096;         // Resolution for ADC_WIDTH_BIT_12
    for (int i = 0; i < 4; i++)
    {
        adc_oneshot_read(adc1_handle, channel[i], &adc_values[i]);
        adc_voltages[i] = (adc_values[i] * reference_voltage) / resolution;
        ESP_LOGI(TAG, "ADC Channel %d: %d, Voltage: %.2fV", i, adc_values[i], adc_voltages[i]);
    }
    // printf("adc Done, ShutDown!");
    // vTaskDelay(pdMS_TO_TICKS(100));
    set_power(true);

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0); // Set GPIO high to shut down

    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_peer(peer_mac));

    uint8_t data[] = "Open";

    ESP_ERROR_CHECK(esp_now_send(peer_mac, data, sizeof(data)));
    
    ESP_LOGI(TAG, "Power Off.");
    set_power(false);
}
