#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/uart.h"

#include <stdint.h>


static const char *TAG = "sniffer";
static volatile int management_frame_count = 0;
static volatile int control_frame_count = 0;
static volatile int data_frame_count = 0;
static volatile int other_frame_count = 0;

typedef struct {
    uint32_t magic;
    uint16_t ver_major;
    uint16_t ver_minor;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t snaplen;
    uint32_t linktype;
} pcap_file_header_t;

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

int uart_send_test() {
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0,
                                    1024,   // rx_buffer_size: must be > hardware FIFO size
                                    0,      // tx_buffer_size: 0 = write blocks until sent
                                    0, NULL, 0));  // no event queue, default interrupt flags

    pcap_file_header_t hdr = {
        .magic     = 0xA1B2C3D4,
        .ver_major = 0x0002,
        .ver_minor = 0x0004,
        .reserved1 = 0,
        .reserved2 = 0,
        .snaplen   = 65535,
        .linktype  = 105,
    };

    uart_write_bytes(UART_NUM_0, &hdr, sizeof(hdr));

    return 1;
}

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_NONE);

    // // Non-Volatile Storage: for calibration data
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_LOGI(TAG, "nvs ok");

    // // loading wifi driver
    // wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    // ESP_LOGI(TAG, "wifi init ok");

    // // driver setting
    // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));    // where the wifi setting is stored
    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));    // what role this radio does, NULL does not host or join a network
    // ESP_ERROR_CHECK(esp_wifi_start());
    // ESP_LOGI(TAG, "wifi start ok");

    // // setting callback upon frame received
    // ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(my_cb));

    // // turning on the promiscous mode
    // ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    uart_send_test();
    while (1) {
        // ESP_LOGI(TAG, "mgmt: %d, ctrl: %d, data: %d, other: %d",
        //         management_frame_count, control_frame_count,
        //         data_frame_count, other_frame_count);
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
    }
}