# FreeRTOS の task と queue は何を隠しているか

## 1. 要約
- task が隠す = CPU sharing。register/stack の save・restore（context switch）、tick、ready/blocked list。→ 各 task を「CPU を独占した普通の for(;;)」として書ける。
- queue が隠す = context 間の安全な受け渡し。lock（ESP32 dual-core では spinlock の critical section）、ring buffer の head/tail、item の copy、空なら sleep → 来たら wake。
- `xQueueCreate(32, sizeof(captured_frame_t))` = heap から 32 × 272 B ≈ 8.5 KB を確保して handle を返す。失敗は NULL → `abort()`（fail fast: panic + backtrace + reboot）。
- `xQueueSend` は copy なので、callback の local 変数 `f` は return 後に消えても安全。timeout 0 = full なら即 errQUEUE_FULL（drop を数える）。
- FreeRTOS API は system call ではなく普通の function call（同じ binary・同じ privilege、trap なし）。速いが、bad pointer で kernel ごと壊れる。
- xQueueCreate は ESP-IDF の `components/freertos` に同梱された FreeRTOS kernel の code（source で build）。Wi-Fi driver（closed .a）とは違う。
- Philosophy: task は time-sharing を sequential code に、queue は shared memory を message passing に変える。

## 2. 学んだ situation
G3 で `xQueueCreate` / `xTaskCreate` / `abort()` が出てきて、「これは何？system call？何を隠してる？」を質問。

## 3. 再現方法
`grep -n "define xQueueCreate" $IDF_PATH/components/freertos/FreeRTOS-Kernel/include/freertos/queue.h`
