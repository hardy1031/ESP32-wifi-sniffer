# Drivers, flash memory, and how the ESP32 runs code

## 1. 要約

### What a driver is
- A **driver** is software that runs on the CPU that controls a device. It translates the OS's *standard* requests into operations on *that specific* device (reading and writing the device's registers).
- **The OS defines the interface; the driver implements it.** This is the reverse of a REST API, where the side being called defines the contract. Here the caller (the OS) defines it. This pattern is called an SPI (service provider interface) or dependency inversion.
  - Example: Linux calls the driver's `ndo_set_rx_mode()`. The Broadcom `bgmac` driver then sets the PROM bit in its chip's register, which turns on promiscuous mode.

### Promiscuous mode: the device has it, the Wi-Fi driver switches it
- Promiscuous mode is a **capability of the network device** (its hardware or firmware). It makes the controller pass *every* frame it hears up to the CPU, instead of dropping the frames that aren't addressed to it.
- Not every device supports it. Where it actually lives depends on the chip design (FullMAC vs SoftMAC).
- The **Wi-Fi driver is what turns it on and off**:
```
Linux PC
  tcpdump → OS flag IFF_PROMISC (+ permission check: root / CAP_NET_ADMIN)
          → driver ndo_set_rx_mode() → sets the chip's "promiscuous" bit → device stops filtering
ESP32
  app_main → esp_wifi_set_promiscuous(true)   (a plain function call; no OS gatekeeper)
           → Espressif Wi-Fi driver → Wi-Fi block → frames delivered to my callback
```
- So "who owns promiscuous mode?" has three answers:
  - the **capability** is in the device;
  - the **switch** is in the driver;
  - on a PC, **who is allowed** to flip the switch is the OS's policy.

### Who writes it vs who ships it
- **Write** = who creates the code. **Ship** = who gets the file onto your disk.
- Usually **the device maker writes it and the OS ships it.**
  - Linux: Intel writes `iwlwifi`, and it is part of the Linux kernel.
  - Windows: Intel or Realtek writes the driver; Microsoft signs it and delivers it through Windows Update.

### The mouse exception: one driver for (almost) all mice
- Mice follow the USB **HID** standard. Each mouse sends a *report descriptor* that describes its own data format.
- Because of that, the OS side writes **one generic driver** that works for basically every mouse. On Linux, `hid-generic` matches *any vendor and any product*.
- Example: a boot-protocol report `01 05 FE` means "left button held, X +5, Y −2".

### ESP32: the caller is the ESP32's own CPU, so the driver gets flashed too
```
ESP32 chip = the whole computer
  CPU (caller)          app_main + Espressif Wi-Fi driver (libnet80211.a, libpp.a, ...)
  Wi-Fi block (callee)  radio + baseband + MAC
      CPU → Wi-Fi block : driver reads/writes registers
      Wi-Fi block → CPU : interrupt → driver → my callback
```
- The driver must live next to the CPU that runs it. On the ESP32 that CPU is inside the ESP32, so the Wi-Fi driver `.a` files are **linked into my app image and flashed with it every time**.
- Compare a PC with a mouse: the CPU is in the PC, so the driver lives on the **PC's disk**. The OS loads it from disk into RAM when the mouse appears. The mouse runs only its own firmware.
- `.a` is **not a driver file type**. It is a *static library*: an archive of compiled `.o` files that gets merged into the program at build time. Driver files have their own types: Linux `.ko`, Windows `.sys`, macOS `.dext`.
- The headers are separate from the code. `esp_wifi.h` is the "menu" (declarations), and the `.a` files are the actual code. `esp_log.h` is a different library: logging.

### `/dev/...` (Linux): correcting my first idea
- `/dev/xxx` is a **device file (a special file), not a folder**.
- For **char and block devices** (`/dev/tty1`, disks, ...), a program does `open/read/write/ioctl` on that file. The driver's file operations handle the request and turn it into device operations.
- **Network interfaces (Ethernet, Wi-Fi) do *not* appear in `/dev`.** They get a name like `eth0` or `wlan0`, and that name has no entry in the filesystem. Programs reach them through sockets.
  - Why: a network interface deals in *addressed packets*, not a plain byte stream. "Write bytes into `/dev/wlan0`" wouldn't say which protocol or destination; a socket carries that.
  - Each interface has a **name and an index number** (`ip link` shows both). A packet socket is tied to one interface with `bind()`, passing that index (`sll_ifindex`).
  - Example, how a sniffer (tcpdump) works on Linux:
    1. `socket(AF_PACKET, ...)` creates a raw packet socket at the driver level (layer 2).
    2. `bind()` ties it to `wlan0`'s index.
    3. The socket option `PACKET_MR_PROMISC` turns on promiscuous mode.

### fd table: files and sockets are the same kind of object
- It's the **app** that talks to the **kernel** through a file or a socket. The kernel then talks to the hardware through the driver.
- `open()` and `socket()` both return an **fd**. An fd is an index into the process's fd table. Each entry points to a kernel `struct file` (an "open file description").
- What differs is the **operations table** (`f_op`) that the `struct file` carries:
```
Process     fd table:  3                                  4
                       ▼                                  ▼
Kernel      struct file                         struct file
              f_op = driver's file_operations     f_op = socket_file_ops
              path = /dev/ttyUSB0                 private_data → struct socket  (in "sockfs", no path)
              → driver read/write → serial chip   → network stack → wlan0 → Wi-Fi driver
```
- So a socket *is* a file inside the kernel. It just has **no path**. Its name looks like `socket:[12345]`.
- **Same handle, different naming:**
  - devices are found by a filesystem path;
  - network interfaces are found by an interface name or index.
- `send(fd, buf, n, 0)` behaves exactly like `write(fd, buf, n)`.
- Philosophy: polymorphism in C. `f_op` is a table of function pointers, like a vtable. The calls look the same; the behavior depends on what's behind the fd.

### Flash memory
- Flash is **non-volatile memory**: it keeps its data with the power off. RAM is volatile and loses everything.
- Flash is a *memory technology*, not a disk. A PC's SSD, however, is a storage device *built from* flash chips.
- On the ESP32, flash plays the role a disk plays on a PC: it is where the bootloader, the partition table, and my app (including the driver) are stored permanently.

### How the ESP32 runs my `.bin`
```
idf.py build   my code + ESP-IDF libraries (.a, incl. Wi-Fi driver) + FreeRTOS → one app .bin
               (built on the Mac with the xtensa-esp32-elf cross-compiler)
idf.py flash   .bin → USB serial → ESP32 flash
power on
 ① Mask ROM: 1st-stage bootloader (fixed) loads the 2nd-stage bootloader from flash 0x0 into RAM
 ② 2nd-stage bootloader reads the partition table (0x8000), then sets up my app:
      IRAM/DRAM segments → COPIED from flash into RAM
      IROM/DROM segments → MAPPED (flash MMU + cache): the CPU runs them straight from flash
 ③ jumps to the app → FreeRTOS starts → app_main()
```
- Most code stays in flash and runs through the cache, because IRAM is small. Timing-critical code and some interrupt handlers go into IRAM.
- A PC is different: it copies a program into RAM before running it.

## 2. 学んだsituation
- 2026-11-11, start of Phase 1 (promiscuous-mode capture). I asked "who owns promiscuous mode? the kernel? but the ESP32 has no OS...".
- That opened a chain of questions:
  1. what a driver is
  2. who writes it vs who ships it
  3. why one driver works for all mice
  4. whether the ESP32 is a device, and what its caller and callee are
  5. what `.a` is
  6. what flash is, and how the ESP32 boots
  7. why network interfaces aren't in `/dev`, and how files and sockets share the fd table
- Side facts:
  - The ESP32 actually *does* have an OS: FreeRTOS, built into ESP-IDF.
  - The ESP32 Wi-Fi is **2.4 GHz only**, so my sniffer cannot hear 5 GHz networks.

## 3. 再現方法
```bash
# ESP32 side (Mac)
ls /dev/cu.*                                   # ESP32 board appears as a USB-serial port (USB-to-UART bridge driver)
ls $IDF_PATH/components/esp_wifi/lib/esp32     # Espressif Wi-Fi driver = prebuilt .a files
idf.py size-components                         # how much space each component takes in the app image;
                                               # compare hello_world vs a project that uses Wi-Fi
idf.py -p /dev/cu.<port> flash monitor         # boot log shows the "2nd stage bootloader" lines before app_main

# Linux PC side (for comparison)
modinfo iwlwifi            # "filename:" line = where the driver .ko sits on disk (/lib/modules/...)
ls -l /dev | head          # char/block device files ("c"/"b" in the first column)
ip link                    # network interfaces with their index numbers (1: lo, 2: wlan0 ...): NOT in /dev
ls -l /proc/self/fd        # fd table of this process: files show a path, sockets show socket:[inode]
```

## Sources
- Linux Device Drivers 2nd ed., Ch.1, "Classes of Devices and Modules": https://oreilly.com/library/view/linux-device-drivers/0596000081/ch01s03.html
- Linux Network Administrator's Guide, ch.3: https://linuxtopia.org/online_books/linux_network_administrators_guide/x-087-2-hardware.html
- KernelNewbies: Module (drivers provide /dev files): https://backiee.wasmer.app/http_kernelnewbies_org/Module
- bgmac `ndo_set_rx_mode` patch: https://lists.openwall.net/netdev/2013/02/06/61
- Wikipedia, Service provider interface: https://www.wikipedia.org/wiki/Service_Provider_Interface
- Intel, Linux support for Intel wireless (drivers in upstream kernel): https://ark.intel.com/content/www/us/en/support/articles/000005511.html
- Microsoft, drivers built by IHVs and published to Windows Update: https://techcommunity.microsoft.com/t5/windows-it-pro-blog/deployment-service-for-driver-updates-public-preview-coming-soon/ba-p/2914212
- Linux `hid-generic.c`: https://tangled.org/tjh.dev/kernel/blob/for-next/drivers/hid/hid-generic.c
- Linux kernel docs, HID report descriptors: https://docs.kernel.org/6.8/hid/hidintro.html
- OSDev wiki, USB HID boot protocol: https://osdev.wiki/wiki/USB_Human_Interface_Devices?oldid=7556
- ESP-IDF commit listing the esp_wifi `.a` libs: https://github.com/espressif/esp-idf/commit/63af85f7e79fe734ea6656088f9666ad4a002298
- ESP-IDF build system (components → `.a` → linked into app): https://github.com/espressif/esp-idf/blob/129286d/docs/en/api-guides/build-system.rst
- Wikipedia, ar (Unix): https://kiwix.lema.org/wikipedia_en_all_mini_2024-02/A/Ar_(Unix)
- ESP-IDF Application Startup Flow: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/startup.html
- ESP-IDF Memory Types: https://github.com/espressif/esp-idf/blob/636ff35b/docs/en/api-guides/memory-types.rst
- ESP-IDF FreeRTOS overview: https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/system/freertos.html
- ESP32 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- IBM, Flash vs SSD: https://www.ibm.com/think/topics/flash-vs-ssd-storage
- Lenovo, What is flash memory: https://lenovo.com/us/en/glossary/flash-memory
- modprobe(8): https://linux.die.net/man/8/modprobe
- Linux Device Drivers, Ch.14 Network Drivers (interfaces not in the filesystem): https://www.oreilly.com/library/view/linux-device-drivers/1565922921/ch14.html
- packet(7) (AF_PACKET, sll_ifindex, PACKET_MR_PROMISC): https://man7.org/linux/man-pages/man7/packet.7.html
- open(2) (fd → open file description / struct file): https://man.voidlinux.org/open.2
- send(2) (send with flags 0 = write): https://www.man7.org/linux//man-pages/man2/send.2.html
- Linux Device Drivers 2nd ed., File Operations (f_op): https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s03.html
- Linux net/socket.c (socket_file_ops, sockfs): https://mirrors.lzu.edu.cn/git/linux-nf/src/branch/main/net/socket.c
