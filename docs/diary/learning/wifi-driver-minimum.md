# Wi-Fi driver: ESP app dev として最低限知ればいいこと

## 1. 要約
- driver の内部（closed な `.a`）は知らなくていい。知るべきは「API・呼ぶ順番・callback の rules」だけで開発できる。
- 呼ぶ順番: `nvs_flash_init` → `esp_wifi_init(WIFI_INIT_CONFIG_DEFAULT)` → `set_storage(RAM)` → `set_mode(NULL)` → `esp_wifi_start` → `set_promiscuous_rx_cb(my_cb)` → `set_promiscuous(true)`。
- config は 3 種類: init の `cfg`（driver の resource）、mode（radio の役割: NULL/STA/AP）、`set_config`（接続先 SSID。sniffer は不要）。
- callback は driver が呼ぶ（自分では呼ばない）。`buf` と `type` は driver が入れてくれる。登録は名前だけ渡す: `set_promiscuous_rx_cb(my_cb)`（`my_cb()` ではない）。
- callback は driver の task で動く → 重い処理は禁止、queue で自分の task に渡す。
- `type` の仕分けも driver がやる（MGMT/CTRL/DATA/MISC）。default filter は MGMT+DATA だけ → CTRL が欲しければ `set_promiscuous_filter`。
- 全 API の戻り値を `ESP_ERROR_CHECK` で見る。error 名でどの step で壊れたか分かる。

## 2. 学んだ situation
step A–E で frame count sniffer を作る中で。driver の正体を掘りすぎた後、「app dev に必要なのは API の使い方と rules」と整理。

## 3. 再現方法
step D/E の log: `wifi:ic_enable_sniffer` → `mgmt: N, ctrl: 0, data: N, other: N` が増えていけば OK。
