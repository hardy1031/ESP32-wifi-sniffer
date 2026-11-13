# Decision: stream captured frames over UART to the PC (not save to a file on the ESP32)

- Date: 2026-11-11
- Status: decided
- Context: Phase 1, after step E (counting frames by type worked). Next is wrapping frames in PCAP for Wireshark.

## Question
Where do captured 802.11 frames go?
- (a) Saved as a `.pcap` file on the ESP32, then read later in Wireshark on the PC
- (b) Saved to an SD card
- (c) Streamed live over USB serial (UART) to the PC → `bridge.py` → Wireshark

## Decision
**(c) UART stream → PC.** This matches the original plan in `../0905/phase1.md` and `../0905/overview.md`.

## Why
1. **The ESP32 has no disk.**
   - Its storage is the flash (4 MB on my board), and the bootloader, partition table, NVS, phy_init and the app (~1 MB) already live there.
   - RAM (520 KB SRAM) is far too small for a capture.
2. **A file in flash fills up in seconds.**
   - Rough estimate: ~300 frames/s × ~200 B ≈ 60 KB/s, so about 2 MB of free flash lasts ~30 s.
3. **Writing captures to flash wears it out.**
   - NOR flash survives ~100,000 erase cycles per sector.
   - Espressif cites a real case where writes about every 500 ms wore out an ESP32's flash in a few days.
4. **Live view.** Streaming shows frames in Wireshark in real time. With a file, I only see results after copying it off.
5. **No extra hardware.** It uses the USB cable I already flash with. An SD card would need an extra module.
6. **Puts each job where it's cheapest.**
   - ESP32 = capture + wrap in PCAP;
   - PC = storage (its disk, no size limit) + decoding (Wireshark).

## Consequences / things I now have to handle
- **UART is shared with the text log.** `ESP_LOGI` output would corrupt the binary PCAP stream, so logs must be turned off or moved elsewhere while streaming.
- **Baud rate** must go up: at 115200, frames per second × size won't fit. The plan says ≥ 1152000.
- **Callback → FreeRTOS Queue → UART task.** The promiscuous callback runs in the Wi-Fi driver task, so UART writes must happen in my own task.
- Needs `bridge.py` (pyserial → stdout) on the PC: `python bridge.py | wireshark -k -i -`.

## Rejected for now (possible later)
- **SD card (b):** GB-scale and works without a PC, but needs extra hardware and isn't live. Good extension later for standalone capture.
- **File in flash (a):** too small, wears the flash out, not live.

## Next steps (small, each with feedback)
- G1: send only the 24-byte PCAP file header (magic `0xA1B2C3D4`, v2.4, LinkType 105 = raw 802.11), with logs off → Wireshark opens it with no error, 0 packets.
- G2: + one hard-coded fake frame → Wireshark shows 1 packet.
- G3: + real frames via queue → task → live 802.11 frames in Wireshark.

## Sources
- ESP-IDF File System Considerations (flash wear, 500 ms example): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/file-system-considerations.html
- ESP-IDF Partition Tables: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html
- ESP-IDF Memory Types (520 KB SRAM): https://github.com/espressif/esp-idf/blob/abf8ffa/docs/en/api-guides/memory-types.rst
- ESP-IDF Wi-Fi Modes (callback runs in Wi-Fi driver task): https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/wifi-driver/wifi-modes.html
- PCAP format (IETF draft): https://www.ietf.org/archive/id/draft-ietf-opsawg-pcap-03.html
- Link-layer types (105 = IEEE 802.11): https://www.tcpdump.org/linktypes.html
