# NVS と RF calibration data（なぜ nvs_flash_init するか）

## 1. 要約
- NVS (Non-Volatile Storage) = flash の中の小さな key-value store。partition table の `nvs` partition に置かれ、reboot しても残る。
- calibration data = radio が自分の analog 回路の個体差を測った補正値。chip ごとに違い、実行してみないと得られないので build 時に `.bin` に入れられない → run time に NVS へ保存。
- `nvs_flash_init()` が無いと保存も読込もできず、毎 boot full calibration（遅い）。
- calibration 自体は Espressif の driver の仕事。app の仕事は storage を開くことだけ（flash の layout は project が持ち、NVS は複数 module で共有、壊れた時に erase するかは app の判断）。
- `WIFI_STORAGE_FLASH` だと SSID/password も NVS に保存。sniffer は RAM で十分。

## 2. 学んだ situation
step D の初回 boot log の `failed to load RF calibration data (0x1102)` → `Saving new calibration data` から。「calibration は hardware 側の責務では?」を整理。

## 3. 再現方法
`idf.py erase-flash` 後の初回 boot で 0x1102 warning、reset 後の 2 回目は消える。`idf.py partition-table` で `nvs` 行を確認。
