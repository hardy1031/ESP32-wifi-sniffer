# Abstraction は「何を隠すか」で layer が決まる（thread / syscall / process）

## 1. 要約
- What a thread abstracts is the CPU: each thread behaves as if it had its own processor, while the OS time-shares the real cores.
- thread ≈ FreeRTOS task。両方 scheduler が切り替える単位で、同じ layer。interrupt を隠すのは thread ではない。
- Linux kernel 内では process も thread も `task_struct`（= task）。thread は `clone(CLONE_VM|CLONE_THREAD)` で memory を共有する task。
```
thread / task  → CPU sharing を隠す          (scheduler)
syscall        → hardware event を隠す        (kernel: IRQ → deferral → buffer → recv() が返る)
process        → 他の program の memory を隠す (kernel + MMU)
```
- Philosophy: Each abstraction hides one thing. 「この abstraction は何を隠す？」の答えでどの layer か分かる。
- ESP32 では syscall 層が無いので、interrupt → 「data ready」の層（callback + queue）は自分で組む。

## 2. 学んだ situation
xTaskCreate から「Linux に task は無いの？thread？」「thread は task と interrupt を隠したもの？」と質問して整理。

## 3. 再現方法
Linux で `ls /proc/<pid>/task/` → thread ごとに 1 entry（kernel は task として扱っている）。
