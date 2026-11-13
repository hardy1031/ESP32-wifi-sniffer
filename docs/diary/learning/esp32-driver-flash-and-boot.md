# ESP32: driver も一緒に flash される / flash と boot

## 1. 要約
- ESP32 では caller = ESP32 の CPU、callee = Wi-Fi block（radio + MAC）。CPU→Wi-Fi block は register 操作、逆は interrupt → driver → 自分の callback。
- Wi-Fi driver は Espressif の closed な `.a`（libnet80211.a 等）。`.a` = static library（`.o` の archive）で、driver 専用の拡張子ではない（Linux driver は `.ko`）。
- build 時に自分の code + `.a` + FreeRTOS が 1 つの `.bin` に link される → `idf.py flash` のたびに driver も flash される。
- header (`esp_wifi.h`) = 宣言の menu、`.a` = 実体。
- flash = non-volatile memory（電源を切っても消えない）。disk ではないが、ESP32 では disk の役割。
- boot: ROM bootloader → 2nd-stage bootloader(flash 0x0) → IRAM/DRAM は RAM に copy、IROM/DROM は copy せず flash を cache 経由で map して直接実行 → app_main。

## 2. 学んだ situation
「driver は device に載ってる executable?」「flash したら disk に刻まれる?」から、ESP32 と PC の driver の置き場所の違いを整理。

## 3. 再現方法
`ls $IDF_PATH/components/esp_wifi/lib/esp32` で driver の `.a` が見える。`idf.py size-components` で app 内の各 component の size。
