# promiscuous callback に届くもの: rx_ctrl と payload

## 1. 要約
- frame を 1 つ受け取るたびに、driver が `my_callback` に 2 つ渡してくれる:
```
pkt->rx_ctrl   … ESP32 が受信したときに書いたメモ（frame の一部ではない）
pkt->payload   … 電波で届いた frame そのもの = [ header | 中身 | FCS(4 bytes) ]
```
- `rx_ctrl` でよく使うのは 2 つ: `sig_len` = payload の長さ（FCS 込み）、`rx_state` = ちゃんと受信できたか（0 なら OK）。他に `rssi`, `channel`, `rate` も入っている。
- FCS = 送る側が最後につける 4 bytes の check 用の数字。中身ではない。ESP32 の hardware が受信時にこれで確かめて、結果を `rx_state` に書く。
- つまり FCS は「確かめるための 4 bytes」、`rx_state` は「その確かめた結果」。

## 2. 学んだ situation
Wireshark の Malformed / Bogus IP version の原因を探していて、callback が受け取っている値を整理した。

## 3. 再現方法
`~/esp/esp-idf/components/esp_wifi/include/local/esp_wifi_types_native.h` の `wifi_pkt_rx_ctrl_t`: `sig_len` = "length of packet including FCS"、`rx_state` = "0: no error"。
