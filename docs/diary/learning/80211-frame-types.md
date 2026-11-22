# 802.11 frame types: management / control / data

## 1. 要約
- 全 frame は MAC header の Frame Control `type` で 3 種類に分かれる。ESP32 の `WIFI_PKT_MGMT/CTRL/DATA` はこれと同じ。`WIFI_PKT_MISC` は ESP-IDF 独自（MIMO など）。
- **Management (0)**: network を見つける・つなぐ・切る。beacon, probe req/resp, auth, assoc, deauth。
- **Control (1)**: 電波上の送受信の調整。ACK, RTS/CTS, Block Ack。中身の data はない。
- **Data (2)**: IP packet を運ぶ。Data, QoS Data, Null（省電力の合図）。4-way handshake（EAPOL）も data で運ばれる。
- beacon = AP が「ここに SSID X がある」と約 100ms ごとに broadcast する mgmt frame → 1 AP で約 10 個/s。sniffer で mgmt ≫ data になる主な理由。
- 接続の流れ: probe（mgmt）→ auth/assoc（mgmt）→ EAPOL（data）→ IP 通信（data）+ 毎回 ACK（ctrl）。

## 2. 学んだ situation
Step E で `mgmt: N, ctrl: 0, data: N` と数えた後、Wireshark で見るときに「3 種類が何なのか」を整理し直した。

## 3. 再現方法
Wireshark で `wlan.fc.type == 0 / 1 / 2` で絞る。beacon は `wlan.fc.type_subtype == 0x08` → `Wireless Management` の下に SSID が見える。
