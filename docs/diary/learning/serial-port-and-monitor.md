# serial port / /dev/cu.* / monitor

## 1. 要約
- serial port = データを1 bit ずつ1本の線で送る interface(UART)。対義は parallel(複数線で同時)。高速 interface は skew のため今はほぼ serial(USB/SATA/PCIe)。
- TCP port(番号)とは無関係。同じ "port" の別概念（layer 違い）。
- /dev/cu.* の cu = call-up（こちらから発信する serial device）。dial-in は tty.*。flash は Mac 発信なので cu.* を使う。
- serial は双方向。monitor は passthrough: チップ→画面(log) と 自分の打鍵→チップ(UART RX) の両方。ただし予約キー(Ctrl-], Ctrl-T)は monitor が横取り。入力を読む code が無ければ打鍵は無視される。
- Bluetooth 機器も Serial Port Profile で /dev/cu.* に出る（物理線 vs 無線、同じ抽象の別 transport）。
- monitor 終了は Ctrl-](Ctrl-C ではない。Ctrl-C はチップに渡る)。

## 2. 学んだ situation
`ls /dev/cu.*` で Bluetooth と usbserial が混在していた時。cu の意味、serial とは何か、打鍵がチップに行くか、終了キー、を整理。

## 3. 再現方法
`ls /dev/cu.*` → usbserial-xxxx が実機（抜き差しで消え/現れる）。monitor 終了は Ctrl-]。
