# UART driver: ESP app dev として最低限知ればいいこと

## 1. 要約
- UART controller = chip 上の hardware block（3 個: UART0/1/2、中身は同じ）。違うのは配線だけで、UART0 が USB-UART bridge → Mac に繋がっている。
- 使うのは 2 call だけ: `uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0)` を 1 回 → `uart_write_bytes(UART_NUM_0, &hdr, sizeof(hdr))`。
- parameter: RX ring buffer（FIFO より大きく必須）、TX ring buffer（0 = 送り終わるまで block、大量送信なら数 KB）、event queue（送信だけなら 0/NULL）、interrupt flags（0 = default）。
- TX FIFO = controller 内の固定サイズ hardware queue。`tx_buffer_size` はその手前の RAM の ring buffer で別物。ISR（driver 持ち）が裏で FIFO に詰める。ISR も ring buffer も自分で書かなくていい。
- log と binary は同じ UART0 を共有 → `esp_log_level_set("*", ESP_LOG_NONE)`。`printf` は改行変換があるので binary には使わない。
- baud rate は両側で一致必須（115200 ≒ 11.5 KB/s）。
- 自分の task は必ず yield（`vTaskDelay`）。`while(1){}` は Task Watchdog に捕まり、backtrace が UART に混ざる。

## 2. 学んだ situation
G1（PCAP header 24 B を UART に出す）で。monitor に `�ò���i` → watchdog backtrace → 修正 → `xxd g1.bin` で 24 B 一致を確認。

## 3. 再現方法
`python tools/bridge.py --port /dev/cu.usbserial-1420 --baud 115200 > g1.bin` → `xxd g1.bin` が `d4c3 b2a1 0200 0400 … ffff 0000 6900 0000`。
