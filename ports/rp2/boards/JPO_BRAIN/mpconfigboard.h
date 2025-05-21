// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "JPO Brain"

// VFS (LittleFS) flash storage size after the binary. 
// Availaable space: 16Mb - (bootloader + header_page + mpy-dbgr), 
// with some extra for safety (if mpy/bootloader binaries grow). 4kb aligned
// Required on 2025-05-21: 60+4+438 = 502 kb
#define MICROPY_HW_FLASH_STORAGE_BYTES          (((16 * 1024) - 800) * 1024)

// Approved values, see Github issue #39
// Not working as we're not using Micropython's version of TinyUSB
#define MICROPY_HW_USB_MANUFACTURER_STRING "JPO"
#define MICROPY_HW_USB_VID (0x2E8A)
#define MICROPY_HW_USB_PID (0x105C)
