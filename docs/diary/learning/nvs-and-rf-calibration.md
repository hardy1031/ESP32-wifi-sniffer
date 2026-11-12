# NVS and RF calibration data (why app_main calls nvs_flash_init)

## 1. 要約

### NVS
- NVS (Non-Volatile Storage) = a small **key-value store inside the ESP32's flash**. It survives reboot and power-off.
- `nvs_flash_init()` opens the default NVS partition, named `nvs`.
```
Flash (physical memory)
  └─ partition table (regions of flash)
       └─ "nvs" partition (0x9000 in the simple "Single factory app, no OTA" table)
            └─ key-value data   e.g. RF calibration data, Wi-Fi SSID/password (if WIFI_STORAGE_FLASH)
```
- Analogy: flash = the ESP32's "disk"; NVS = a tiny settings file on it.

### Calibration data
- No two chips come out of the factory identical, so their analog radio parts (amplifiers, filters, mixers) differ slightly.
- At boot, the radio measures its own errors and computes **correction values**. Those values are the calibration data. Analogy: the tuning-peg positions of a guitar.
- **It only exists after running on *this* chip.** It can't be baked into the `.bin` at build time, so it needs storage that can be written at run time: NVS.
```
Build time (Mac)            Run time (this one ESP32)
  .bin = code + defaults      radio measures ITSELF → calibration data → save to NVS
  ✗ no cal data possible      next boot: load from NVS → skip full calibration
```
- Full calibration gives the best results but costs a lot of time and current. That is why the result is cached.
- `phy_init` partition = PHY *init data* (the recipe, prepared beforehand). NVS cal data = the measured *result* for this chip. These are different things.

### Why the app (me) has to call nvs_flash_init
- **Calibration itself is not my job.** Espressif's driver measures, computes and saves the data; I write zero calibration code.
- **My job is only to open the storage**, because flash is owned by the app/project:
  1. **The project owns the flash layout.** I choose the partition table in menuconfig. The driver can't know my layout.
  2. **NVS is shared.** Many ESP-IDF and third-party modules store data there, each in its own namespace. One owner (the app) opens it once.
  3. **Fixing NVS problems is a policy decision.** If init fails (e.g. no free pages because the partition was truncated), the fix is to erase it, which wipes *every* module's data. A Wi-Fi library shouldn't decide that.
  4. **It's optional.** `wifi_init_config_t.nvs_enable` can turn it off, at the cost of a full calibration every boot.
- On a PC, the OS mounts disks before apps run. On the ESP32 there's no layer above me, so the app plays the "OS" role for system-wide setup.
- Philosophy: explicit over hidden. Separate what's known at build time (code) from what's only known at run time (per-device measurements).

## 2. 学んだsituation
- 2026-11-11, step D of the frame-count sniffer worked: `frames:` climbed about 150–300 per second.
- The first-boot log showed:
  - `W phy_init: failed to load RF calibration data (0x1102), falling back to full calibration`
  - `0x1102` = `ESP_ERR_NVS_NOT_FOUND`: first boot, nothing saved yet.
  - Then `Saving new calibration data` = the result was written to NVS. Next boot should not warn.
- I asked "isn't calibration the hardware maker's job, not mine?". Answer: yes, the calibration is theirs; owning the storage is mine.
- The same log also showed `E wifi:failed to post WiFi event=43 ret=259`.
  - `259` = `0x103` = `ESP_ERR_INVALID_STATE`.
  - No default event loop had been created. It's harmless for the sniffer.
  - Fix: call `esp_event_loop_create_default()` before `esp_wifi_init()`.

## 3. 再現方法
```bash
cd wifi_sniffer
idf.py -p /dev/cu.<port> flash monitor   # 1st boot after erase: "failed to load RF calibration data (0x1102)" + "Saving new calibration data"
# reset the board (Ctrl-T Ctrl-R in monitor) → 2nd boot: the calibration warning should be gone
idf.py -p /dev/cu.<port> erase-flash     # wipes NVS too → the 1st-boot warning comes back
idf.py partition-table                   # prints the partition table, including the "nvs" and "phy_init" rows
```

## Sources
- ESP-IDF NVS: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
- ESP-IDF RF Calibration: https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/RF_calibration.html
- ESP-IDF Partition Tables: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html
- ESP-FAQ NVS (Wi-Fi config in NVS, flash lifetime): https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/storage/nvs.html
- ESP32 Datasheet (calibration circuitries): https://rapptor.vub.ac.be/Education/2BAIW/CAD/pdf/esp32.pdf
- SourceVu, wifi_init_config_t (nvs_enable): https://sourcevu.sysprogs.com/espressif/esp-idf/symbols/wifi_init_config_t
- ESP-IDF Error Codes: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/error-codes.html
- ESP-IDF Wi-Fi Driver overview (events to default loop): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi-driver/overview.html
