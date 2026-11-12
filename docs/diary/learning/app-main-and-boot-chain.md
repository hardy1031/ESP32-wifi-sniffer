# app_main と boot chain

## 1. 要約
自分の file に main() は無い。本物の main は ESP-IDF 内にあり、app_main() はそれが呼ぶ hook。
boot 順: 電源/reset → ROM bootloader → 2nd-stage bootloader(flash) → ESP-IDF startup(call_start_cpu0) → FreeRTOS 起動 → "main task" が app_main() を呼ぶ。
entry point は linker script が決める（compiler が main を探すのではない）。実行は call graph 順（file の上から順ではない）。
app_main が return すると、その task は削除されるがチップは止まらない（idle task 等が回り続ける）。

## 2. 学んだ situation
「main 関数が無いのに何処から始まる?」から。実際の flash log で `main_task: Calling app_main()` → Hello → `Returned from app_main()` を観測して確認。

## 3. 再現方法
flash して monitor。log の `call_start_cpu0` → `main_task: Calling app_main()` → 自分の ESP_LOGI → `Returned from app_main()` の流れを読む。
