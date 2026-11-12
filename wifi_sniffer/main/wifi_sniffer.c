#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "sniffer";
static volatile int management_frame_count = 0;
static volatile int control_frame_count = 0;
static volatile int data_frame_count = 0;
static volatile int other_frame_count = 0;

static void my_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT) {
        management_frame_count++;
    }
    if (type == WIFI_PKT_CTRL) {
        control_frame_count++;
    }
    if (type == WIFI_PKT_DATA) {
        data_frame_count++;
    }
    if (type == WIFI_PKT_MISC) {
        other_frame_count++;
    }
}

void app_main(void) {
  // Non-Volatile Storage: for calibration data
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_LOGI(TAG, "nvs ok");

  // loading wifi driver
  wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
  ESP_LOGI(TAG, "wifi init ok");

  // driver setting
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));    // where the wifi setting is stored
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));    // what role this radio does, NULL does not host or join a network
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "wifi start ok");

  // setting callback upon frame received
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(my_cb));

  // turning on the promiscous mode
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

  while (1) {
      ESP_LOGI(TAG, "mgmt: %d, ctrl: %d, data: %d, other: %d",
               management_frame_count, control_frame_count,
               data_frame_count, other_frame_count);
      vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
  }
}