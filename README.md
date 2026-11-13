# ESP32 Wi-Fi Sniffer

Puts the ESP32 into promiscuous mode and streams every captured 802.11 frame as PCAP over UART0 to the PC, where `tools/bridge.py` forwards it to Wireshark.

```
ESP32: Wi-Fi callback → queue → uart_tx_task → UART0 (300000 baud)
  → USB-UART chip (CH340) → Mac /dev/cu.usbserial-* → tools/bridge.py → Wireshark
```

## Usage

```sh
. $HOME/esp/esp-idf/export.sh
cd wifi_sniffer
idf.py build flash
python ../tools/bridge.py --port /dev/cu.usbserial-1420 --baud 300000 \
  | /Applications/Wireshark.app/Contents/MacOS/Wireshark -k -i -
```

- `--baud` must match `PCAP_BAUD` in `wifi_sniffer/main/wifi_sniffer.c`.
- Stop `bridge.py` (Ctrl+C) before running `idf.py flash`; both use the same serial port.
- To save instead of viewing: `python ../tools/bridge.py --port ... --baud 300000 > cap.bin`

## Known issue: baud rate above 300000 breaks the stream (2026-11-13)

### What was observed
| Baud | Result |
|---|---|
| 115200 | works (many frames dropped on the ESP32: the link carries only ~40–75 frames/s vs ~150–300 captured/s) |
| 300000 | works |
| 400000, 500000, 1000000 | works for a few seconds, then Wireshark: `Frame N too long (...)` |
| 921600 | nothing arrives; `bridge.py` never finds the PCAP magic `d4 c3 b2 a1` |

- Board's USB-UART chip: WCH CH340 (`ioreg`: idVendor 6790 = 0x1A86, idProduct 29987 = 0x7523), macOS built-in driver (port name `usbserial-*`).
- Not a crash: no boot/panic text in the saved stream. Not the disk: plenty of free space (Wireshark error -14 = `WTAP_ERR_SHORT_WRITE` was a side effect).
- Parsing a saved `cap.bin` (500000 and 1000000) showed exactly **3 bytes missing from the middle of one frame**; every record after that is read at the wrong offset. The same beacon arrived intact 185 times, so the loss is random, not content-dependent.

### Possible causes (not confirmed yet)
- 400000 and up: **receive overrun**: UART has no flow control here, so if the CH340 / USB / macOS driver can't take bytes fast enough, bytes are silently lost. PCAP has no sync marker or checksum, so one lost byte breaks the rest of the capture.
- 921600 only: the **macOS CH340 driver may program the chip to a wrong rate** for this value (both chips can generate 921600 within ~0.2% on paper).

### What to learn to understand it
- UART framing, baud rate generation (clock ÷ divisor), and error types (overrun, framing error)
- Flow control: RTS/CTS (hardware) and XON/XOFF (software), and why dev boards usually don't wire it
- How a USB-UART bridge works: USB bulk transfers, chip RX buffer, how the host driver sets the baud rate
- Why stream formats need framing / resync (sync markers, length + CRC, COBS)

### Ideas to try later
- Install WCH's official macOS driver and retest 921600 / 1000000
- Make `bridge.py` resync after a broken record, or add framing + CRC on the ESP32 side
- Lower `CAPTURE_MAX_LEN` (e.g. 128) to fit more frames per second at 300000
