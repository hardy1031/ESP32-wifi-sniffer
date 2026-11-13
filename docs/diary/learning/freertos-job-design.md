# FreeRTOS app の設計: job に分けてから primitive に map する

## 1. 要約
- job = 設計の単位（1 つの責務 + 自分の trigger + 自分の pace）。task = それを動かす FreeRTOS object の 1 つ。job ≠ task。
- 考え方: ① verb を列挙 ② 各 verb の trigger ③ pace と block するか ④ 同じ trigger・pace は 1 job、遅い方が速い方を止めるなら分けて queue でつなぐ ⑤ どこで動かすか決める。
- map: block が必要 → task / data の矢印 → queue / signal だけ → notification・semaphore / 共有 resource → mutex / 短い周期処理 → esp_timer・app_main loop / driver の event → callback。
- task は stack 分 RAM を食う → 「待ち（wait）を持つ job だけ task にする」。block しない短い job は既存の context に乗せる。
- sniffer: capture（bursty）→ Wi-Fi callback / data → queue / UART 送信（block する）→ uart_tx_task / channel hop → app_main loop か esp_timer。
```
app_main: ① driver init ② queue 作成 ③ task 作成 ④ event source 開始 ⑤ idle or return
task:     for(;;){ block して待つ → 処理 → 出力 }
```
- queue/task は起動時に 1 回作って使い続ける（毎回作らない）。Philosophy: set up once, then react forever。

## 2. 学んだ situation
「毎回 queue と task を作るの？」「job の定義は？どう分ければいい？」と質問して、自分で分解できる手順を整理。

## 3. 再現方法
`wifi_sniffer.c` の `app_main` が ①→⑤ の順になっているか確認。
