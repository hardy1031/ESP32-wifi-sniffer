# Wireshark filter と layer

## 1. 要約
- filter 名の prefix = どの layer の header の field か。`wlan.` は全部同じ 802.11 MAC header（L2）。
- frame type（`wlan.fc.type`）と MAC address（`wlan.addr`）は**同じ L2**。layer が違うのは `ip.`（L3）、`tcp./udp.`（L4）、`dns/http`（L7）。
```
radiotap.*  電波情報（signal, channel）
wlan.*      L2: type, MAC addr, SSID
ip.*        L3   tcp.* / udp.*  L4   dns, http  L7
```
- 違う layer も `&&` で組み合わせられる: `wlan.fc.type == 2 && ip.addr == 192.168.1.5`。
- 上の layer ほど当たる frame が減る。`ip` は暗号化されていない（or 復号済み）data frame だけ。mgmt/ctrl は IP を持たないので絶対に当たらない。`wlan.` は全 frame に効く。

## 2. 学んだ situation
frame type の filter と `ip == ...` / `mac == ...` は別 layer か？と考えたときに、MAC address も frame type と同じ L2 だと整理した。

## 3. 再現方法
packet を 1 つ選び、詳細 pane の tree（`IEEE 802.11` → `Logical-Link Control` → `Internet Protocol`）の順番 = layer の順番。field を右クリック → Apply as Filter で filter 名の prefix を確認。
