#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdint.h>
#include <stdlib.h>   // abort
#include <string.h>

#define PCAP_UART        UART_NUM_0
#define PCAP_BAUD        300000   // esp32 can speeds up to 5Mbps
#define CAPTURE_MAX_LEN  256      // bytes copied per frame (longer frames are truncated)
#define QUEUE_LEN        32       // frames waiting between callback and UART task

// ---------- PCAP format ----------
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

typedef struct {
    uint32_t ts_sec;        // timestamp: seconds
    uint32_t ts_usec;       // timestamp: microseconds
    uint32_t captured_len;  // number of packet bytes that follow in the stream
    uint32_t orig_len;      // original length of the packet
} pcap_record_header_t;
_Static_assert(sizeof(pcap_record_header_t) == 16, "PCAP record header must be 16 bytes");

// ---------- one captured frame, as stored in the queue ----------
typedef struct {
    int64_t  ts_us;                   // when the callback saw it (us since boot)
    uint32_t orig_len;                // real frame length
    uint32_t captured_len;            // bytes actually copied (<= CAPTURE_MAX_LEN)
    uint8_t  data[CAPTURE_MAX_LEN];
} captured_frame_t;

static QueueHandle_t frame_queue;
static volatile uint32_t dropped_frames = 0;   // queue was full -> frame thrown away

// ---------- pcap functions ----------
static void send_pcap_file_header(void) {
    pcap_file_header_t hdr = {
        .magic     = 0xA1B2C3D4,
        .ver_major = 2,
        .ver_minor = 4,
        .reserved1 = 0,
        .reserved2 = 0,
        .snaplen   = CAPTURE_MAX_LEN,
        .linktype  = 105,   // LINKTYPE_IEEE802_11: raw 802.11 frames
    };
    uart_write_bytes(PCAP_UART, &hdr, sizeof(hdr));
}

static void send_pcap_packet(const captured_frame_t *frame) {
    pcap_record_header_t rec = {
        .ts_sec       = (uint32_t)(frame->ts_us / 1000000),
        .ts_usec      = (uint32_t)(frame->ts_us % 1000000),
        .captured_len = frame->captured_len,
        .orig_len     = frame->orig_len,
    };
    uart_write_bytes(PCAP_UART, &rec, sizeof(rec));
    uart_write_bytes(PCAP_UART, frame->data, frame->captured_len);
}

// ---------- Wi-Fi callback: runs in the Wi-Fi driver task ----------
static void my_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MISC) {
        return;   // MISC (e.g. MIMO) has no payload bytes
    }
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;

    captured_frame_t frame;
    frame.ts_us        = esp_timer_get_time();
    frame.orig_len     = pkt->rx_ctrl.sig_len;
    frame.captured_len = (frame.orig_len < CAPTURE_MAX_LEN) ? frame.orig_len : CAPTURE_MAX_LEN;
    memcpy(frame.data, pkt->payload, frame.captured_len);   // copy: buf belongs to the driver

    if (xQueueSend(frame_queue, &frame, 0) != pdTRUE) {  // 0 = never wait in the driver task
        dropped_frames++;
    }
}

// ---------- UART task: takes frames from the queue and sends them ----------
static void uart_tx_task(void *arg) {
    captured_frame_t frame;
    while (1) {
        // sleep until a frame arrives
        if (xQueueReceive(frame_queue, &frame, portMAX_DELAY) == pdTRUE) {
            send_pcap_packet(&frame);
        }
    }
}

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_NONE);
    // 1. UART driver part
    ESP_ERROR_CHECK(uart_driver_install(PCAP_UART,
                                        1024,   // rx_buffer_size: must be > hardware FIFO size
                                        0,      // tx_buffer_size: 0 = write blocks until sent (fine in our own task)
                                        0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_baudrate(PCAP_UART, PCAP_BAUD));

    // 2. PCAP file header needs to be sent only once
    send_pcap_file_header();

    // 3. queue + UART task (before Wi-Fi starts delivering frames)
    frame_queue = xQueueCreate(QUEUE_LEN, sizeof(captured_frame_t));
    if (frame_queue == NULL) {
        abort();
    }
    xTaskCreate(uart_tx_task,       // task function
                "uart_tx",          // task name
                4096,               // stack size
                NULL,               // argument
                5,                  // priority
                NULL);

    // 4. Wi-Fi driver part
    // need to allocate memory for the calibration data
    ESP_ERROR_CHECK(nvs_flash_init());

    // Wi-Fi driver posts events here
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(my_callback));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    // 5. nothing else to do here; app_main's task just idles
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
