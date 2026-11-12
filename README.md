# ESP32 Wi-Fi Sniffer

Puts the ESP32 into promiscuous mode and logs the number of 802.11 frames it sees, by type (mgmt / ctrl / data / other), every second.

## Usage

```sh
cd wifi_sniffer
idf.py build
idf.py -p /dev/cu.usbserial-1420 flash monitor
```
