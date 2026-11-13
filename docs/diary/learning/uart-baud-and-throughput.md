# UART・baud rate・capture rate の関係（Mac ⇄ ESP32）

## 1. 要約
```
Wi-Fi air → my_cb（150–300 frames/s）→ frame_queue 32 slot（満杯なら drop）→ uart_tx_task
→ ESP32 UART0 @baud → USB-UART chip（CP2102/CH340…）→ USB → macOS driver（/dev/cu.usbserial-*）→ bridge.py → Wireshark
```
- baud rate = UART が線に出す bit/s。1 byte = 10 bit（start+8+stop）→ 115200 ≒ 11.5 KB/s、921600 ≒ 92 KB/s。
- clock 線が無いので両端で同じ値必須。ずれると drop ではなく文字化け（magic が見つからない）。ESP32 側の値は firmware（`uart_set_baudrate`）が決め、`--baud` は Mac 側を合わせるだけ。
- 決め方: 下限 = frames/s × bytes/record × 10（300 × 272 × 10 ≒ 816k）、上限 = 一番遅い link（ESP32 UART 5 Mbps より USB-UART chip 1–3 Mbps が先に効く）。間の標準値を選び、drop を測って確認。
- bottleneck は「capture が UART より速い」こと。queue（buffer）は burst を吸うだけで、平均で入力 > 出力なら必ず満杯→ drop。先に baud を上げ、次に queue size（1 slot 272 B、RAM と相談）。
- Mac に UART は無い。USB-UART chip の driver が USB で chip に baud を伝え、serial port として見せている。

## 2. 学んだ situation
G3 で 115200 だと届くが drop 多数、921600 は magic が見つからない。「baud を変えるのは何のため？」から整理。

## 3. 再現方法
`system_profiler SPUSBDataType | grep -iE -A2 "cp210|ch34|ft232"` で chip を確認 → datasheet の最大 baud を見る。
