# ESP-IDF と idf.py の関係

## 1. 要約
ESP-IDF = ESP32 用の公式 C フレームワーク。2つの顔を持つ: (a) Mac 上で動く build toolchain（compiler + CMake/Ninja + idf.py）、(b) チップ内で動く runtime framework（FreeRTOS + driver API）。
idf.py 自体は compiler でも flasher でもなく、裏の本物のツールを呼ぶ front-end（task runner のようなもの）: build→gcc, flash→esptool.py, monitor→serial monitor。

## 2. 学んだ situation
「idf.py は Python だからチップのコードも Python?」という疑問から。Python なのは Mac 側のツールだけで、ESP32 上は C（Xtensa）という layer の分離を理解。

## 3. 再現方法
`. $HOME/esp/esp-idf/export.sh` 後に `idf.py --version` → version 表示。
`idf.py build` の出力で CMake → ninja → xtensa-esp32-elf-gcc が走るのが見える。
