# ESP-IDF の log は console = UART0 に勝手に流れる

## 1. 要約
- stdout = program が書く stream。console = その stream の行き先 device。ESP-IDF では console = UART0 @115200（sdkconfig: `CONFIG_ESP_CONSOLE_UART_NUM=0`）。起動時（app_main 前）に設定され、その code は自分の app .bin に link されている。
- `esp_wifi_init()` など ESP-IDF の library 自身が `ESP_LOGI` を呼ぶ → stdout → console → UART0。自分は UART に書いていなくても流れる（bootloader も別 .bin から UART0 に出す）。
- 現象 1: 最初のテストで `idf.py monitor` に `I (532) wifi:wifi driver task...` 等が見えた = UART0 → USB → Mac で読んでいた。
- 現象 2: PCAP 実験で file header の直後に Wi-Fi の log が混ざり、Wireshark が `Frame 1 too long (1768319351 bytes)`。1768319351 = little-endian で `77 69 66 69` = "wifi" を incl_len と読んだ。
- 対策: app_main の先頭で `esp_log_level_set("*", ESP_LOG_NONE);`。確実にするなら menuconfig の log verbosity = No output、または console を別 UART / None に。
- Philosophy: 共有 output channel は global resource。binary を流すなら他の書き手を黙らせるか channel を分ける。

## 2. 学んだ situation
G3 を 115200 で動かすと "Frame 1 too long"。G1 にあった log off の行が書き直しで消えていた。

## 3. 再現方法
`python3 -c "import struct;print(struct.pack('<I',1768319351))"` → `b'wifi'`。
