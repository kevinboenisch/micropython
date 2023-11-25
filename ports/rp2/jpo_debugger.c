#include "jpo_debugger.h"
#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

#include "mphalport.h"
#include "py/runtime.h"
#include "pico/multicore.h"

#define MUTEX_TIMEOUT_MS 100
auto_init_mutex(_dbgr_mutex);

#if JPO_DBGR_BUILD
// True if debugging is active.
// Set to true by the PC before running the program, reset to false by Brain when done.
bool _jpo_dbgr_is_debugging = false;

// If NULL, program is running, if set to a string, it's stopped
// see R_STOPPED_* values in jpo_debugger.h
static char* _stopped_reason = NULL;

// Reset vars to initial state
void reset_vars() {
    _jpo_dbgr_is_debugging = false;
    _stopped_reason = NULL;
}
#else
void reset_vars() {}
#endif // JPO_DBGR_BUILD

static bool jcomp_handler_inlock(JCOMP_MSG msg) {
    if (jcomp_msg_has_str(msg, 0, CMD_DBG_TERMINATE)) {
        // is this thread-safe?
        mp_sched_keyboard_interrupt();
        return true;
    }
#if JPO_DBGR_BUILD
    if (jcomp_msg_has_str(msg, 0, CMD_DBG_START)) {
        DBG_SEND("CMD_DBG_START");
        _jpo_dbgr_is_debugging = true;
        return true;
    }
    if (_jpo_dbgr_is_debugging) {
        if (jcomp_msg_has_str(msg, 0, CMD_DBG_PAUSE)) {
            DBG_SEND("CMD_DBG_PAUSE");
            // If already stopped, do nothing
            if (!_stopped_reason) {
                _stopped_reason = R_STOPPED_PAUSED;
            }
            return true;
        }
        if (jcomp_msg_has_str(msg, 0, CMD_DBG_CONTINUE)) {
            DBG_SEND("CMD_DBG_CONTINUE");
            _stopped_reason = NULL;
            return true;
        }
    }
#endif // JPO_DBGR_BUILD
    return false;
}

bool core1_dbgr_jcomp_handler(JCOMP_MSG msg) {
    bool has_mutex = mutex_enter_timeout_ms(&_dbgr_mutex, MUTEX_TIMEOUT_MS);
    if (!has_mutex) {
        DBG_SEND("Error: core1_dbgr_jcomp_handler() failed to get mutex");
        return false;
    }
    bool handled = jcomp_handler_inlock(msg);
    mutex_exit(&_dbgr_mutex);
    return handled;
}

void jpo_dbgr_init(void) {
    static bool _init_done = false;
    if (_init_done) {
        return;
    }
    _init_done = true;

    JCOMP_RV rv = jcomp_add_core1_handler(core1_dbgr_jcomp_handler);
    if (rv) {
        DBG_SEND("Error: jcomp_add_core1_handler() failed: %d", rv);
        return;
    }

    reset_vars();
}

void send_done(int ret) {
    DBG_SEND("Event: %s %d", EVT_DBG_DONE, ret);

    JCOMP_CREATE_EVENT(evt, 12);
    jcomp_msg_set_str(evt, 0, EVT_DBG_DONE);
    jcomp_msg_set_uint32(evt, 8, (uint32_t) ret);
    jcomp_send_msg(evt);
}
void jpo_parse_compile_execute_done(int ret) {
    reset_vars();
    send_done(ret);
}

#ifdef JPO_DBGR_BUILD
char* get_stopped_reason(void) {
    bool has_mutex = mutex_enter_timeout_ms(&_dbgr_mutex, MUTEX_TIMEOUT_MS);
    if (!has_mutex) {
        DBG_SEND("Error: get_stopped_reason() failed to get mutex");
        // Continue anyway
    }
    char* rv = _stopped_reason;
    mutex_exit(&_dbgr_mutex);
    return rv;
}

void send_stopped(const char* reason8ch) {
    DBG_SEND("Event: %s%s", EVT_DBG_STOPPED, reason8ch);

    JCOMP_CREATE_EVENT(evt, 16);
    jcomp_msg_set_str(evt, 0, EVT_DBG_STOPPED);
    jcomp_msg_set_str(evt, 8, reason8ch);
    jcomp_send_msg(evt);
}
void __jpo_dbgr_check(void) {
    if (!_jpo_dbgr_is_debugging) {
        return;
    }

    // TODO: check breakpoints etc.
    bool stop_reported = false;
    while (true) { 
        // Check if stopped
        char* sreason = get_stopped_reason();
        if (sreason == NULL) {
            break;
        }

        // Report only once
        if (!stop_reported) {
            send_stopped(sreason);
            stop_reported = true;
        }

        // Spin-wait
        MICROPY_EVENT_POLL_HOOK_FAST;
    }
}
#endif //JPO_DBGR_BUILD
