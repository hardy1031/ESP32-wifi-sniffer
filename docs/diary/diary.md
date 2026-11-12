# wifi_sniffer 開発日記

（Aug–Sep の座学・計画は `0825` / `0826` / `0905` サブフォルダ、key-takeaways は `../docs/key-takeaways.md`。ここは手を動かした記録。学んだ細かい知識は `learning/` に分割。）

## 2026-11-11 — 初めての flash → log 成功

**目標:** Phase 1 の入口。「めちゃ小さく flash して log を見る」ループを一周させる。

**やったこと（時系列）:**
1. ボード接続確認。`ls /dev/cu.*` → `/dev/cu.usbserial-1420`（USB-UART。Bluetooth 群に混ざって出るが usbserial が実機）。
2. ESP-IDF v5.5.6 を macOS にインストール。
   - prereq: `brew install cmake ninja dfu-util python3`（ccache は z3/llvm を source build し始めて地獄 → 外した）
   - `git clone -b v5.5.6 --recursive`（esp_wifi/lib が ~2GB）
   - `./install.sh esp32` → `. $HOME/esp/esp-idf/export.sh`
3. `wifi_sniffer` を手動で ESP-IDF project 化（`CMakeLists.txt` + `main/CMakeLists.txt` + `main/main.c`）。
4. `main.c` に `app_main` + `ESP_LOGI(TAG, "Hello, I am alive")`。
5. `idf.py -p /dev/cu.usbserial-1420 flash monitor` → `I (247) HELLO: Hello, I am alive` ✅

**到達点:** write C → flash → read log のループが通った。実機は ESP32-D0WD-V3 (dual core, rev 3.1)。

**次:** Phase 1 本番 — promiscuous mode を有効化して最初の 802.11 frame を出力。

**メモ:** flash size warning（4MB 実機 vs 2MB config）は無害、後で `idf.py menuconfig` で調整可。


```
User space   app (screen / tcpdump)
   │  ── open("/dev/ttyUSB0")            ── socket(AF_PACKET) + bind(ifindex of wlan0)
   │     found by filesystem path            found by interface name/index (no path)
   │     → returns a file descriptor         → returns a file descriptor (sockfd)
Kernel       char-device layer → driver        network stack → interface → network driver
Hardware     serial chip                       Wi-Fi card