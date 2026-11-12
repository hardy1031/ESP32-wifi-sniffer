# デバッグガイド
C言語による実装例、およびUSBケーブル1本で行えるデバッグ手法をまとめています。


## ESP32のデバッグ・動作確認手法
専用のデバッガ（JTAG等）を使わずとも、PCとESP32を接続したUSBケーブル1本 でログ確認からクラッシュ解析までを行えます。

### 1. 基本的なデバッグコマンド
PCのターミナル（CLI）から以下の1行を実行することで、「ビルド ➔ 書き込み ➔ シリアルモニタ起動」が連続実行されます。

> idf.py flash monitor

### 2. ログレベルを活用したプリントデバッグ (ESP_LOG)
C言語の printf に相当する機能として、ESP-IDFにはタグ付け・色分け表示が可能な ESP_LOG マクロが用意されています。

```
C
#include "esp_log.h"

static const char *TAG = "MY_APP";

void debug_example(void)
{
    int status_code = 200;
    
    // 重要度（ログレベル）に応じた使い分け
    ESP_LOGI(TAG, "正常動作ログ: ステータス = %d", status_code); // 緑色 (Info)
    ESP_LOGW(TAG, "警告ログ: パケット処理遅延が発生しています");     // 黄色 (Warn)
    ESP_LOGE(TAG, "エラーログ: Wi-Fi初期化に失敗しました");         // 赤色 (Error)
}
```

### 3. バイナリパケット（生データ）のデバッグ表示
802.11フレームなどの生のバイナリデータ（16進数）の中身を確認したい場合は、ESP_LOG_BUFFER_HEX を使用します。
```
C
uint8_t dummy_packet[] = {0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

// メモリの内容を16進数フォーマットでダンプ表示
ESP_LOG_BUFFER_HEX("RAW_DATA", dummy_packet, sizeof(dummy_packet));
```

シリアルモニタ出力例:
```
Plaintext
I (1234) RAW_DATA: 80 00 00 00 ff ff ff ff
```

### 4. クラッシュ時の自動バックトレース（スタック解析）
メモリ割り当てエラーやヌルポインタアクセスなどでプログラムが異常終了（Panic）した場合、idf.py monitor はPC上のソースコード（C言語）の何行目でクラッシュしたかを自動追跡して表示します。

クラッシュ発生時のシリアルモニタ出力例:
```
Plaintext
Guru Meditation Error: Core 0 panic'ed (LoadProhibited). Exception was unhandled.

Backtrace: 0x400d1d15:0x3ffb5030 0x400d1d84:0x3ffb5050
  #0  0x400d1d15 in wifi_promiscuous_cb at main/main.c:12  <-- 12行目でヌルポインタ参照が発生！
  #1  0x400d1d84 in app_main at main/main.c:28
```