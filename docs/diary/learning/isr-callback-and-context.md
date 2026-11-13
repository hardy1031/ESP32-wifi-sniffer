# ISR・callback・task: 「何の code か」と「どこで動くか」は別軸

## 1. 要約
- ISR = Interrupt Service Routine。hardware が interrupt を上げると CPU が今の処理を止めて飛ぶ関数。短く、block 禁止。driver が `esp_intr_alloc` で登録する。
- callback = 他人に渡して、他人が呼ぶ自分の関数。task ではない。呼んだ側の context で動く。
- 軸が 2 つ: code（what: my_cb, uart_tx_task）と execution context（where: task か ISR context）。
```
Wi-Fi block → interrupt → Wi-Fi driver の ISR（最小限）→ Wi-Fi task を起こす
Wi-Fi task  → my_cb(buf, type) を呼ぶ ← my_cb はここ（driver の task）で動く
my_cb → xQueueSend → uart_tx_task が起きて UART へ
```
- my_cb が遅いと Wi-Fi driver 自体が遅れる → copy して queue に渡して即 return。
- Linux でも同じ概念はある（hard IRQ, softirq/workqueue, thread, event loop callback）が、kernel が syscall の下に隠すので app dev は普段見ない。
- Philosophy: inversion of control。自分は「何をするか」だけ渡し、「いつ・どこで」は driver が決める。

## 2. 学んだ situation
「callback は task なの？」「ISR って何？」「Linux ならこれ考えないの？」と質問して整理。

## 3. 再現方法
`esp_wifi_set_promiscuous_rx_cb(my_cb)` が callback 登録の 1 行。
