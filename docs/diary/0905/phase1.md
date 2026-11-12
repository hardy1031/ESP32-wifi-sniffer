## 🎯 Phase 1: L2 802.11 パケットキャプチャ & リアルタイム解析
**目的:** Wi-Fi接続を行わないPromiscuous（プロミスキャス）モードで周囲の802.11生フレームを直接キャプチャし、PC上のWiresharkへリアルタイムにストリーミング配信する。

### 1-1. 開発タスク（ESP32 / C言語）
- [ ] **ESP-IDF Wi-Fi初期化 & モード制御**
  - `esp_wifi_set_mode(WIFI_MODE_NULL)` で未接続状態で起動。
  - `esp_wifi_set_promiscuous(true)` によりRAWフレーム受信を有効化。
  - チャネルホッピング（1ch〜13chの自動切り替え機能）の実装。
- [ ] **パケット受信コールバック（`promiscuous_rx_cb`）の実装**
  - 高速処理のための割り込みハンドラ/タスク分離（FreeRTOS Queueの活用）。
  - 受信データ（`wifi_promiscuous_pkt_t`）から802.11 MACヘッダ（Frame Control, Addr1-3, RSSI等）の抽出。
- [ ] **PCAP（DLT_IEEE802_11: 105）カプセル化**
  - 起動時に「PCAP Global Header（24バイト）」を出力する処理を記述。
  - 受信パケットごとに「PCAP Packet Header（16バイト）」を付与し、バイナリ化。
- [ ] **UART高速バイナリ通信**
  - ボーレートを `1152000 bps` 以上に設定し、パケットをシリアル出力。

### 1-2. 開発タスク（PC側 / Python & Wireshark）
- [ ] **Pythonシリアルブリッジ（`bridge.py`）作成**
  - `pyserial` を使用し、ESP32からのバイナリデータを標準出力（`sys.stdout`）にフラッシュ出力。
- [ ] **パイプライン接続の実装**
  - `python bridge.py | wireshark -k -i -` コマンドによるリアルタイム描画の検証。

---