/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "jpo_version.h"
#include "py/compile.h"
#include "py/cstack.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/modbluetooth.h"
#include "extmod/modnetwork.h"
#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/softtimer.h"
// #include "shared/tinyusb/mp_usbd.h"
#include "uart.h"
#include "modmachine.h"
#include "modrp2.h"
#include "mpbthciport.h"
#include "mpnetworkport.h"
#include "genhdr/mpversion.h"
#include "mp_usbd.h"
#include "mpconfigport.h" // for JPO_JCOMP

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/unique_id.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/watchdog.h"

#if MICROPY_PY_LWIP
#include "lwip/init.h"
#include "lwip/apps/mdns.h"
#endif
#if MICROPY_PY_NETWORK_CYW43
#include "lib/cyw43-driver/src/cyw43.h"
#endif
#if PICO_RP2040
#include "RP2040.h" // cmsis, for PendSV_IRQn and SCB/SCB_SCR_SEVONPEND_Msk
#elif PICO_RP2350 && PICO_ARM
#include "RP2350.h" // cmsis, for PendSV_IRQn and SCB/SCB_SCR_SEVONPEND_Msk
#endif
#include "pico/aon_timer.h"
#include "shared/timeutils/timeutils.h"

#ifdef JPO_JCOMP
    #include "jpo/hal.h"
    #include "jpo/jcomp/debug.h"
    #include "jpo/jcomp/jcomp_protocol.h"
#else
    #include "tusb.h"
#endif //JPO_JCOMP

#include "jpo_debugger.h"

extern uint8_t __StackTop, __StackBottom;
extern uint8_t __StackOneTop, __StackOneBottom;
extern uint8_t __GcHeapStart, __GcHeapEnd;

// Embed version info in the binary in machine readable form
bi_decl(bi_program_version_string(MICROPY_GIT_TAG));

// Add a section to the picotool output similar to program features, but for frozen modules
// (it will aggregate BINARY_INFO_ID_MP_FROZEN binary info)
bi_decl(bi_program_feature_group_with_flags(BINARY_INFO_TAG_MICROPYTHON,
    BINARY_INFO_ID_MP_FROZEN, "frozen modules",
    BI_NAMED_GROUP_SEPARATE_COMMAS | BI_NAMED_GROUP_SORT_ALPHA));

int main(int argc, char **argv) {
    // This is a tickless port, interrupts should always trigger SEV.
    #if PICO_ARM
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;
    #endif

    pendsv_init();
    soft_timer_init();

    // Set the MCU frequency and as a side effect the peripheral clock to 48 MHz.
    set_sys_clock_khz(SYS_CLK_KHZ, false);

    // Hook for setting up anything that needs to be super early in the bootup process.
    MICROPY_BOARD_STARTUP();

    #if MICROPY_HW_ENABLE_UART_REPL
    bi_decl(bi_program_feature("UART REPL"))
    setup_default_uart();
    mp_uart_init();
#define BOOT_FLAG_REMOVE_MAIN_PY 0x1
static bool _remove_user_scripts = false;

void check_watchdog_flags() {
    uint32_t flags = watchdog_hw->scratch[7];
    // reset the flags
    watchdog_hw->scratch[7] = 0;

    //DBG_OLED("wd flags %d", flags);

    if (flags & BOOT_FLAG_REMOVE_MAIN_PY) {
        _remove_user_scripts = true;
    }
}

int main(int argc, char **argv) {   
    #ifdef JPO_JCOMP
        // JCOMP i/o replaces any other; UART as well as USB
    #else
        #if MICROPY_HW_ENABLE_UART_REPL
        bi_decl(bi_program_feature("UART REPL"))
        setup_default_uart();
        mp_uart_init();
        #else
        #ifndef NDEBUG
        stdio_init_all();
        #endif
        #endif

    #if MICROPY_HW_ENABLE_USBDEV && MICROPY_HW_USB_CDC
    bi_decl(bi_program_feature("USB REPL"))
    #endif
        #if MICROPY_HW_ENABLE_USBDEV
        #if MICROPY_HW_USB_CDC
        bi_decl(bi_program_feature("USB REPL"))
        #endif
        tusb_init();
        #endif
    #endif //JPO_JCOMP

    #if MICROPY_PY_THREAD
    bi_decl(bi_program_feature("thread support"))
    mp_thread_init();
    #endif

    // Start and initialise the RTC
    struct timespec ts = { 0, 0 };
    ts.tv_sec = timeutils_seconds_since_epoch(2021, 1, 1, 0, 0, 0);
    aon_timer_start(&ts);
    mp_hal_time_ns_set_from_rtc();

    // Initialise stack extents and GC heap.
    mp_cstack_init_with_top(&__StackTop, &__StackTop - &__StackBottom);
    mp_stack_set_top(&__StackTop);
    #ifdef JPO_JCOMP
        // Using &__StackOneTop for safety, since &__StackBottom
        // can overlap with core0 stack (with the default linker config)
        // Add a safety margin for 128 bytes plus two JCOMP messages (a request and a response)
        mp_stack_set_limit(&__StackTop - &__StackOneTop - (128 + 2*JCOMP_MSG_BUF_SIZE_MAX));
    #else
        mp_stack_set_limit(&__StackTop - &__StackBottom - 256);    
    #endif
    gc_init(&__GcHeapStart, &__GcHeapEnd);

    #ifdef JPO_JCOMP
    // Initialize JPO HAL library (including JCOMP)
    // TODO-P2: add the Micropython version (MICROPY_BANNER_*), so PC knows to upgrade it
    #if JPO_DBGR_BUILD
    jcomp_set_env_type("MPYT-DBGR:" MICROPY_BANNER_NAME_AND_VERSION ":" VERSION_TIMESTAMP);
    #else
    jcomp_set_env_type("MPYT-FAST:" MICROPY_BANNER_NAME_AND_VERSION ":" VERSION_TIMESTAMP);
    #endif

    // TODO: change once radio is fixed
    //hal_init_no_radio();
    hal_init();
    //DBG_OLED("hal_init done");

    check_watchdog_flags();

    #endif //JPO_JCOMP

    #ifdef JPO_DBGR
    jpo_dbgr_init();
    #endif //JPO_DBGR

    #if MICROPY_PY_LWIP
    // lwIP doesn't allow to reinitialise itself by subsequent calls to this function
    // because the system timeout list (next_timeout) is only ever reset by BSS clearing.
    // So for now we only init the lwIP stack once on power-up.
    lwip_init();
    #if LWIP_MDNS_RESPONDER
    mdns_resp_init();
    #endif
    #endif

    #if MICROPY_PY_NETWORK_CYW43 || MICROPY_PY_BLUETOOTH_CYW43
    {
        cyw43_init(&cyw43_state);
        cyw43_irq_init();
        cyw43_post_poll_hook(); // enable the irq
        uint8_t buf[8];
        memcpy(&buf[0], "PICO", 4);

        // MAC isn't loaded from OTP yet, so use unique id to generate the default AP ssid.
        const char hexchr[16] = "0123456789ABCDEF";
        pico_unique_board_id_t pid;
        pico_get_unique_board_id(&pid);
        buf[4] = hexchr[pid.id[7] >> 4];
        buf[5] = hexchr[pid.id[6] & 0xf];
        buf[6] = hexchr[pid.id[5] >> 4];
        buf[7] = hexchr[pid.id[4] & 0xf];
        cyw43_wifi_ap_set_ssid(&cyw43_state, 8, buf);
        cyw43_wifi_ap_set_auth(&cyw43_state, CYW43_AUTH_WPA2_AES_PSK);
        cyw43_wifi_ap_set_password(&cyw43_state, 8, (const uint8_t *)"picoW123");
    }
    #endif

    // Hook for setting up anything that can wait until after other hardware features are initialised.
    MICROPY_BOARD_EARLY_INIT();

    for (;;) {

        // Initialise MicroPython runtime.
        mp_init();
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));

        // Initialise sub-systems.
        readline_init0();
        machine_pin_init();
        rp2_pio_init();
        rp2_dma_init();
        machine_i2s_init0();

        #if MICROPY_PY_BLUETOOTH
        mp_bluetooth_hci_init();
        #endif
        #if MICROPY_PY_NETWORK
        mod_network_init();
        #endif
        #if MICROPY_PY_LWIP
        mod_network_lwip_init();
        #endif

        // Execute _boot.py to set up the filesystem.
        #if MICROPY_VFS_FAT && MICROPY_HW_USB_MSC
        pyexec_frozen_module("_boot_fat.py", false);
        #else
        pyexec_frozen_module("_boot.py", false);
        #endif

        // Delete user scripts if requested
        if (_remove_user_scripts) {
            _remove_user_scripts = false;
            pyexec_frozen_module("_remove_user_scripts.py", false);
            //DBG_OLED("rmv main done");
        }

        // Execute user scripts.
        int ret = pyexec_file_if_exists("boot.py");

        #if MICROPY_HW_ENABLE_USBDEV
        mp_usbd_init();
        #endif

        if (ret & PYEXEC_FORCED_EXIT) {
            goto soft_reset_exit;
        }
        if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL && ret != 0) {
            ret = pyexec_file_if_exists("main.py");
            if (ret & PYEXEC_FORCED_EXIT) {
                goto soft_reset_exit;
            }
        }

        for (;;) {
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                if (pyexec_raw_repl() != 0) {
                    break;
                }
            } else {
                if (pyexec_friendly_repl() != 0) {
                    break;
                }
            }
        }

    soft_reset_exit:
        mp_printf(MP_PYTHON_PRINTER, "MPY: soft reboot\n");

        // Hook for resetting anything immediately following a soft reset command.
        MICROPY_BOARD_START_SOFT_RESET();

        #if MICROPY_PY_NETWORK
        mod_network_deinit();
        #endif
        machine_i2s_deinit_all();
        rp2_dma_deinit();
        rp2_pio_deinit();
        #if MICROPY_PY_BLUETOOTH
        mp_bluetooth_deinit();
        #endif
        machine_pwm_deinit_all();
        machine_pin_deinit();
        machine_uart_deinit_all();
        #if MICROPY_PY_THREAD
        mp_thread_deinit();
        #endif
        soft_timer_deinit();
        #if MICROPY_HW_ENABLE_USB_RUNTIME_DEVICE
        mp_usbd_deinit();
        #endif

        // Hook for resetting anything right at the end of a soft reset command.
        MICROPY_BOARD_END_SOFT_RESET();

        gc_sweep_all();
        mp_deinit();
        #if MICROPY_HW_ENABLE_UART_REPL
        setup_default_uart();
        mp_uart_init();
        #endif
    }

    return 0;
}

void gc_collect(void) {
    //DBG_SEND("gc_collect");
    //DBG_OLED("gc_collect");

    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif
    gc_collect_end();
}

void nlr_jump_fail(void *val) {
    mp_printf(&mp_plat_print, "FATAL: uncaught exception %p\n", val);
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(val));
    for (;;) {
        __breakpoint();
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    panic("Assertion failed");
}
#endif

#define POLY (0xD5)

uint8_t rosc_random_u8(size_t cycles) {
    static uint8_t r;
    for (size_t i = 0; i < cycles; ++i) {
        r = ((r << 1) | rosc_hw->randombit) ^ (r & 0x80 ? POLY : 0);
        mp_hal_delay_us_fast(1);
    }
    return r;
}

uint32_t rosc_random_u32(void) {
    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
        value = value << 8 | rosc_random_u8(32);
    }
    return value;
}
