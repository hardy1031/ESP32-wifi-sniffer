# ESP32 Wi-Fi Protocol Analyzer & Security Traffic Inspector

ESP32マイコンとPython/Wiresharkを組み合わせた、2フェーズ構成のWi-Fiパケット解析・トラフィックインスペクション・プロジェクトです。
組み込み層（C言語/レジスタ・プロトコル制御）からネットワーク層（TCP/IP、ARP、セキュリティ解析）までを包括的にカバーします。


## 🛠️ 必要資材・動作環境
- **ハードウェア:** ESP32 開発ボード（ESP32-DevKitC 等）、USB ケーブル
- **開発環境 (ESP32):** ESP-IDF (C言語 / FreeRTOS)
- **開発環境 (PC):** Python 3.x (`pyserial` ライブラリ), Wireshark
- **評価用ターゲット機器:** 自宅ローカルLAN内のPC、スマートフォン等（※必ず自所有の環境で実施すること）

---

## 🏗️ システム全体アーキテクチャ
```
+---------------------------------------------------------------+
|                        Phase 1 Architecture                   |
|  [Air Waves] ---> (802.11 Frame)                              |
|                          │                                    |
|                          ▼                                    |
|                 [ESP32 Promiscuous Mode]                      |
|                          │ (C-Lang: Parse 802.11 & PCAP Encapsulation)
|                          ▼                                    |
|                   [USB / UART Serial]                         |
|                          │                                    |
|                          ▼                                    |
|                   [PC / Python Bridge]                        |
|                          │ (Stdio Pipe / Named Pipe)          |
|                          ▼                                    |
|                    [Wireshark GUI]                            |
+---------------------------------------------------------------+
```

---

## 📅 開発ロードマップ（推奨スケジュール）

```
[Week 1-2: Phase 1 基礎]
├─ ESP-IDF環境構築 ＆ UART通信テスト
└─ Promiscuousモード起動 ＆ Rawパケット受信用C言語コード記述

[Week 3-4: Phase 1 完成]
├─ PCAPヘッダの付与ロジック実装
├─ Pythonブリッジ作成 ＆ Wiresharkでの802.11リアルタイム解析成功
└─ 【成果物1】Phase 1 動作デモ動画・GitHubコミット

[Week 5-6: Phase 2 基礎]
├─ Stationモード接続 ＆ 生フレーム送信機能の実装
└─ 構造体を用いたARP Replyパケット生成コード作成

[Week 7-8: Phase 2 完成]
├─ ARP Poisoning動作確認（Target端末の通信リダイレクト）
├─ IP Forwardingロジック実装（通信断絶の防止）
└─ 【成果物2】Phase 2 解析レポート・GitHub README最終更新
```

---

### 獲得・アピールできるスキル
- C言語による構造体キャスト、ビット演算、メモリ（ポインタ）管理
- FreeRTOS環境におけるタスク設計・割り込みハンドラの非同期処理
- IEEE 802.11 MACフレームフォーマット（Management, Control, Dataフレーム）の理解
- PCAPフォーマット構造の理解とWiresharkパイプ連携

