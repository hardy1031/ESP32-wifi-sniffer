# /dev の device file・socket・fd table

## 1. 要約
- `/dev/xxx` は folder ではなく device file。char/block device は `open/read/write/ioctl` で使い、driver の file operations が処理する（例: `/dev/cu.usbserial-1420`）。
- network interface（`wlan0`, `eth0`）は `/dev` に出ない。名前と index 番号だけを持ち、program は socket で使う（packet は宛先・protocol 付きなので byte stream の file に合わない）。
- sniffer の例（Linux）: `socket(AF_PACKET)` → `bind()` で wlan0 の index に紐付け → `PACKET_MR_PROMISC` で promiscuous。
- `open()` も `socket()` も fd を返す = 両方 fd table の entry → kernel の `struct file`。違うのは `f_op`（function pointer の table）。socket は path の無い file（`socket:[12345]`）。

## 2. 学んだ situation
「wlan0 は /dev に無くて socket で使う」の意味から、socket と file の関係、fd table の中身まで掘った。

## 3. 再現方法
Linux で `ip link`（interface と index）、`ls -l /proc/self/fd`（file は path、socket は `socket:[inode]`）。
