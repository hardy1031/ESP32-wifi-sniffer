# PCAP format（file header と record）

## 1. 要約
- 構造: file header 24 B を 1 回 → その後 record（record header 16 B + frame bytes）が並ぶだけ。index も footer も無いので前から streaming できる（UART 向き）。
- file header: magic `0xA1B2C3D4`（little-endian だと `d4 c3 b2 a1` で届き、byte order も伝える）・version 2.4・reserved 0,0・snaplen・linktype 105（raw 802.11）。値は全部 spec で決まる定数。
- record = PCAP の「1 packet 分の封筒」。frame の type（mgmt/ctrl/data）ではない。record header（PCAP: いつ・何 byte）の中に 802.11 frame（frame header: 種類・宛先）が入る。
- record header: `ts_sec`, `ts_usec`, `incl_len`（この packet で実際に保存した byte 数）, `orig_len`（本来の長さ）。
- snaplen = 1 packet で保存する最大 byte 数（file 全体の limit）。超えた分は capture tool が保存 copy から後ろを切る → 上位 layer の data から消える。incl_len ≤ snaplen。名前は snaplen と混ざらない `captured_len` も可。
- 切る理由: 速度（drop 防止）・storage・header だけで足りる分析。TCP の MSS / IP fragmentation は「分割」で data は消えない、snaplen は「切り捨て」。
- file header を 2 回送ると、2 回目を record と読まれ incl_len ≈ 2.7 GB > snaplen で Wireshark が error。

## 2. 学んだ situation
G1（header だけ送る → Wireshark が 0 packets で Capture started）と G2（record + 偽 ACK frame を毎秒）を作る中で、incl_len / snaplen / record の意味を質問して整理。

## 3. 再現方法
`python tools/bridge.py --port /dev/cu.usbserial-1420 --baud 115200 > g1.bin` → `xxd g1.bin` で header 24 B、G2 なら続く 16 B の record header と frame が見える。
