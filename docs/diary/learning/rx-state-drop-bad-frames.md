# rx_state で壊れた frame を捨てる

## 1. 要約
- 電波は noise で乱れるので、ときどき途中の bytes が化けた frame が届く。ESP32 の hardware は受信するたびに FCS で確かめ、結果を `rx_state` に書く（`0` = ちゃんと届いた、`!= 0` = 途中で壊れた）。
- 問題: `rx_state` を見ずに、壊れた frame も全部 pcap に送っていた（壊れた frame だけに関係）。
- 壊れた frame は中身がでたらめ → Wireshark で **Bogus IP version** などになる。しかも遅い UART の時間をそのぶん無駄に使う。
- 直し方: callback の最初で確認し、壊れていたら送らずに捨てる。捨てた数は数えておく。
```c
if (pkt->rx_ctrl.rx_state != 0) { rx_error_frames++; return; }   // 壊れているので捨てる
```
- FCS 問題との違い: FCS = `sig_len`（長さ）を直す / こっち = `rx_state`（OK か）で捨てる。直す場所は別。

## 2. 学んだ situation
Bogus IP version の原因を探していて、callback が壊れた frame も queue に入れていたことに気づいた。

## 3. 再現方法
`rx_error_frames` が 0 のままなら、promiscuous mode では壊れた frame は届いていない → Bogus IP version の原因は別。
