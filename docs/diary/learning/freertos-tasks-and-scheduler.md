# FreeRTOS: task / scheduler / yield

## 1. 要約
FreeRTOS = 小さな preemptive RTOS（task scheduler）。MCU に OS が無いので、複数の処理を task として並行実行させる。
- task = kernel object。xTaskCreate で TCB(管理構造体) + 専用 stack が作られる。実質 thread（uthread に近いが preemptive）。
- yield = CPU を scheduler に返す（vTaskDelay 等）。busy loop は yield せず他を starve させる。
- scheduler = priority の高い ready task を選ぶ。blocked(delay/queue 待ち)は skip。同 priority は round-robin。
- task 終了 = return でなく削除（vTaskDelete(NULL) か他 task が vTaskDelete）。for(;;) は「永久 task」の形。
app 目線で要るのは「app_main も1つの task」「own stack size に注意(overflow)」「priority で順序」の3点。内部実装は無視でよい。

## 2. 学んだ situation
flash log の main_task から FreeRTOS に波及。yield を async/uthread と比較、task の終わり方、scheduler をどこまで知るべきか、を整理。

## 3. 再現方法
phase1.md の設計: RX callback → FreeRTOS Queue → 別 task が UART 送信、が典型パターン。
task 追加: `xTaskCreate(fn, "name", stack, arg, priority, handle)`。
