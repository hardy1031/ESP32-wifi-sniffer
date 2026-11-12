# ESP-IDF 概要
ESP32の公式開発環境である **ESP-IDF** の役割をまとめています。

## 📘 ESP-IDF とは？

**ESP-IDF**（Espressif IoT Development Framework）は、ESP32シリーズの製造元（Espressif Systems社）が提供している **公式のC言語開発環境（SDK / フレームワーク）** です。

* **役割:** C言語のコードをESP32用バイナリへコンパイルする「ビルドツール」に加え、Wi-Fi/Bluetooth制御、メモリ管理、FreeRTOS（リアルタイムOS）などの各種APIライブラリが一式セットになっています。
* **位置付け:** iOSにおける `Xcode / Swift SDK` や、Androidにおける `Android Studio / Android SDK` と同等の存在です。


## 💻 ESP-IDF を使用した具体的なコード例

ESP-IDFのAPI（`esp_wifi.h` 等）を呼び出すことで、Wi-Fiチップのレジスタ操作やプロトコル制御をC言語から直接行えます。

### 例1: 接続を行わずに周囲のWi-Fiパケットを盗聴（Promiscuous Mode）するコード

```
c
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_SNIFFER";

// Wi-Fiパケットを受信した際に呼び出されるコールバック関数
void wifi_promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    
    // 受信パケットのメタデータ（電波強度: RSSI, パケット長）を取得
    int rssi = pkt->rx_ctrl.rssi;
    int len = pkt->rx_ctrl.sig_len;

    // 生データ（802.11 MACヘッダ）の先頭ポインタ
    uint8_t *payload = pkt->payload;

    // ログ出力（RSSIとパケット長を表示）
    ESP_LOGI(TAG, "Packet Captured | RSSI: %d dBm | Length: %d bytes", rssi, len);
}

void app_main(void)
{
    // 1. NVS（非揮発性ストレージ）の初期化（ESP-IDFのWi-Fi駆動に必須）
    nvs_flash_init();

    // 2. Wi-Fiスタックの初期化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 3. モード設定（ルーターに接続しないNULLモード）
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();

    // 4. プロミスキャス（盗聴）モードの有効化 ＆ コールバック関数の登録
    esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb);
    esp_wifi_set_promiscuous(true);

    ESP_LOGI(TAG, "Promiscuous mode started. Sniffing packets...");
}
```