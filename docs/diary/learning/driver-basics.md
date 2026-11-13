# driver とは / 誰が書いて誰が配るか

## 1. 要約
- driver = device(hardware) を操作する software。OS が決めた interface を実装し、OS の標準の命令を「その chip の register 操作」に translate する adapter。REST API と逆で、呼ぶ側(OS)が約束を決める（SPI / dependency inversion）。
- 例: OS が `ndo_set_rx_mode()` を呼ぶ → bgmac driver が chip の PROM bit を立てる → promiscuous mode on。
- promiscuous mode は device の capability、on/off は driver、誰が触れるか（root 権限）は OS の policy。
- 普通は device maker が write、OS が ship（Intel が iwlwifi を書き、Linux kernel に入って届く / Windows は Windows Update）。
- mouse は例外: HID standard で report descriptor が自分の format を説明するので、OS 側の generic driver 1 つでほぼ全 mouse が動く（`01 05 FE` = 左 click, X+5, Y-2）。
- PC: driver は disk にあり、device が刺さると ID で match して RAM に load。

## 2. 学んだ situation
「promiscuous mode は誰が持ってる? kernel?」から driver の正体、write と ship の違い、mouse がなぜ driver 不要か、へ波及。

## 3. 再現方法
Linux で `modinfo iwlwifi` → `filename:` 行が disk 上の driver の場所（/lib/modules/...）。
