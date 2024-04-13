// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "JPO Brain"
#define MICROPY_HW_FLASH_STORAGE_BYTES          (((14 * 1024) + 1408) * 1024)

// Approved values, see Github issue #39
// Not working as we're not using Micropython's version of TinyUSB
#define MICROPY_HW_USB_MANUFACTURER_STRING "JPO"
#define MICROPY_HW_USB_VID (0x2E8A)
#define MICROPY_HW_USB_PID (0x105C)
