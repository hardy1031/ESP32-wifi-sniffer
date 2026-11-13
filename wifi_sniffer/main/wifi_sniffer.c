#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/uart.h"
#include "esp_timer.h"

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
_Static_assert(sizeof(pcap_file_header_t) == 24, "PCAP file header must be 24 bytes");

// PCAP packet record header: one per packet, followed by the packet bytes
typedef struct {
    uint32_t ts_sec;    // timestamp: seconds
    uint32_t ts_usec;   // timestamp: microseconds
    uint32_t captured_len;  // number of packet bytes that follow in the stream
    uint32_t original_len;
} pcap_record_header_t;
_Static_assert(sizeof(pcap_record_header_t) == 16, "PCAP record header must be 16 bytes");

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

static void send_pcap_file_header(void) {
    pcap_file_header_t hdr = {
        .magic     = 0xA1B2C3D4,
        .ver_major = 2,
        .ver_minor = 4,
        .reserved1 = 0,
        .reserved2 = 0,
        .snaplen   = 65535,
        .linktype  = 105,   // LINKTYPE_IEEE802_11: raw 802.11 frames
    };
    uart_write_bytes(UART_NUM_0, &hdr, sizeof(hdr));
}

// G2: send one packet = 16-byte record header + the packet bytes
static void send_pcap_packet(const uint8_t *data, uint32_t len) {
    int64_t time = esp_timer_get_time();   // microseconds since boot
    pcap_record_header_t record = {
        .ts_sec   = (uint32_t)(time / 1000000),
        .ts_usec  = (uint32_t)(time % 1000000),
        .captured_len = len,
        .original_len = len,
    };
    uart_write_bytes(UART_NUM_0, &record, sizeof(record));
    uart_write_bytes(UART_NUM_0, data, len);
}

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_NONE);   // logs share UART0 with the binary stream
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0,
                                        1024,   // rx_buffer_size: must be > hardware FIFO size
                                        0,      // tx_buffer_size: 0 = write blocks until sent
                                        0, NULL, 0));  // no event queue, default interrupt flags

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
    send_pcap_file_header();   // G1: once

    // G2: a hand-made 802.11 ACK frame (control frame, 10 bytes)
    //   Frame Control 0xD4 0x00 = type control, subtype ACK
    //   Duration      0x00 0x00
    //   Receiver addr 02:00:00:00:00:01 (made-up, locally administered)
    const uint8_t fake_ack[10] = {
        0xD4, 0x00,
        0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x01,
    };

    while (1) {
        // ESP_LOGI(TAG, "mgmt: %d, ctrl: %d, data: %d, other: %d",
        //         management_frame_count, control_frame_count,
        //         data_frame_count, other_frame_count);
        send_pcap_packet(fake_ack, sizeof(fake_ack));   // G2: one packet per second
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
    }
}
