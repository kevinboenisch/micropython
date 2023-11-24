#include "jpo_debugger.h"
#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

#include "py/runtime.h"

// TODO: set to false, enable before program execution
static bool jpo_dbgr_isDebugging = true;

bool core1_dbgr_jcomp_handler(JCOMP_MSG msg) {
    // lock before modifying shared state
    if (jcomp_msg_has_str(msg, 0, CMD_DBG_STOP)) {
        // is this thread-safe?
        mp_sched_keyboard_interrupt();
        return true;
    }

    return false;
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
}

void jpo_dbgr_check(void) {
    // TODO: check breakpoints etc.
}

void jpo_parse_compile_execute_done(int ret) {
    DBG_SEND("Event: DBG_DONE %d", ret);
    // TODO: set the debugging flag to false


    JCOMP_CREATE_EVENT(evt, 12);
    jcomp_msg_set_str(evt, 0, DBG_DONE);
    jcomp_msg_set_uint32(evt, 8, (uint32_t) ret);
    jcomp_send_msg(evt);
}