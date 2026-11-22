# FCS を pcap に入れると Malformed Packet になる

## 1. 要約
- 問題: `payload` を最後の FCS までそのまま pcap に書いていた → 正常な frame にも、いらない 4 bytes がついている（**全部**の frame に関係）。
- Wireshark は「pcap（linktype 105）には FCS が入っていない」と思って読む → 最後の 4 bytes を中身の続きだと思って parse し、失敗 → **Malformed Packet**（特に beacon などの mgmt frame）。
- 直し方: pcap に書く長さを 4 減らして FCS を入れない。例: `sig_len` = 300 → 前から 296 bytes だけ書く。
```c
if (pkt->rx_ctrl.sig_len < FCS_LEN) return;          // unsigned: 4 未満で引くと約 42 億になる
frame.orig_len = pkt->rx_ctrl.sig_len - FCS_LEN;     // FCS_LEN = 4
```

## 2. 学んだ situation
自分の capture の beacon が Malformed になっていて、原因が `sig_len` に FCS が入っていることだと分かった。

## 3. 再現方法
直す前と後で Wireshark の `_ws.malformed` filter をかけて、件数が減るか見る。
