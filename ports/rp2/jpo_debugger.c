#include "jpo_debugger.h"
#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

#include "mphalport.h" // for JPO_DBGR_BUILD

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
        // Other messages are handled on core0, in process_message_while_stopped
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

static void send_done(int ret) {
    //DBG_SEND("Event: %s %d", EVT_DBG_DONE, ret);

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

char* get_and_clear_stopped_reason(void) {
    bool has_mutex = mutex_enter_timeout_ms(&_dbgr_mutex, MUTEX_TIMEOUT_MS);
    if (!has_mutex) {
        DBG_SEND("Error: get_stopped_reason() failed to get mutex");
        // Continue anyway
    }
    char* rv = _stopped_reason;
    _stopped_reason = NULL;
    mutex_exit(&_dbgr_mutex);
    return rv;
}

static void send_stopped(const char* reason8ch) {
    DBG_SEND("Event: %s%s", EVT_DBG_STOPPED, reason8ch);

    JCOMP_CREATE_EVENT(evt, 16);
    jcomp_msg_set_str(evt, 0, EVT_DBG_STOPPED);
    jcomp_msg_set_str(evt, 8, reason8ch);
    jcomp_send_msg(evt);
}

#define RAM_START 0x20000000

// should be JCOMP_MAX_PAYLOAD_SIZE, but there's a bug with large packets in mpy
#define STACK_BUF_SIZE 200 
void send_stack(int req_id, jpo_code_location_t *code_loc) {
    char stack_buf[STACK_BUF_SIZE];
    while(code_loc != NULL) {
        dbgr_get_stack_trace(code_loc, stack_buf, STACK_BUF_SIZE);
        DBG_SEND("stack_trace: %s", stack_buf);
        code_loc = code_loc->caller_loc;
    }
    DBG_SEND("stack_trace: done");

    // JCOMP_CREATE_RESPONSE(resp, req_id, 8 + strlen(stack_buf));
    // jcomp_msg_set_str(resp, 0, RSP_DBG_STACK);
    // jcomp_msg_set_str(resp, 8, stack_buf);
    // jcomp_send_msg(resp);
}

#define DBGR_RV_CONTINUE (JCOMP_ERR_CLIENT + 1)
static JCOMP_RV process_message_while_stopped(jpo_code_location_t *code_loc) {
    JCOMP_RECEIVE_MSG(msg, rv, 0);

    // static bool printed = false;
    // if (!printed) {
    //     //DBG_SEND("JCOMP_MSG_BUF_SIZE_MAX: %d", JCOMP_MSG_BUF_SIZE_MAX);
    //     dbgr_print_stack_info();
    //     dbgr_check_stack_overflow(true);
    //     printed = true;
    // }

    if (rv) {
        if (rv != JCOMP_ERR_TIMEOUT) {
            DBG_SEND("Error: while paused, receive failed: %d", rv);
        }
        return rv;
    }

    if (jcomp_msg_has_str(msg, 0, CMD_DBG_CONTINUE)) {
        DBG_SEND("%s", CMD_DBG_CONTINUE);
        return DBGR_RV_CONTINUE;
    }
    else if (jcomp_msg_has_str(msg, 0, REQ_DBG_STACK)) {
        DBG_SEND("%s", REQ_DBG_STACK);
        // TODO: send as a response, not an event
        send_stack(jcomp_msg_id(msg), code_loc);
        return JCOMP_OK;
    }

    return JCOMP_OK;
}

void __jpo_dbgr_check(jpo_code_location_t *code_loc) {
    if (!_jpo_dbgr_is_debugging) {
        return;
    }
    if (code_loc == NULL) {
        DBG_SEND("Warning: __jpo_dbgr_check(): code_loc is NULL, skipping the check");
        return;
    }

    // TODO: check breakpoints etc, in case we need to stop
    // without the pause command
    
    // Check if stopped
    char* sreason = get_and_clear_stopped_reason();
    if (sreason == NULL) {
        return;
    }

    // Report stopped
    send_stopped(sreason);

    // TODO: for testing only. Remove, only do on stack request
    //send_stack(code_loc);

    // Loop and handle requests until continued
    while (true) {
        JCOMP_RV rv = process_message_while_stopped(code_loc);
        if (rv == DBGR_RV_CONTINUE) {
            DBG_SEND("Continuing");
            break;
        }
        // Spin-wait
        MICROPY_EVENT_POLL_HOOK_FAST;
    }
}

extern uint8_t __StackTop, __StackBottom;
extern uint8_t __StackOneBottom, __StackOneTop;

void dbgr_print_stack_info(void) {
    DBG_SEND("__StackTop:%p __StackBottom:%p __StackOneTop:%p __StackOneBottom:%p // s0size:%d", 
             &__StackTop,  &__StackBottom,  &__StackOneTop,  &__StackOneBottom,
             &__StackTop - &__StackOneTop);
}

bool dbgr_check_stack_overflow(bool show_if_ok) {
    uint32_t stack_size = &__StackTop - &__StackOneTop;

    // using the *address* of stack_size (last var on the stack), not the actual size
    int remaining = (uint32_t)&stack_size - (uint32_t)&__StackOneTop;
    
    if (remaining < 0) {
        DBG_SEND("ERROR: Stack overflow. this:%p __StackOneTop:%p size:%d remaining:%d", 
            &stack_size, &__StackOneTop, stack_size, remaining);
        return true;
    }

    if (show_if_ok) {
        DBG_SEND("Stack ok. this:%p __StackOneTop:%p size:%d remaining:%d", 
            &stack_size, &__StackOneTop, stack_size, remaining);
    }
    return false;
}

#endif //JPO_DBGR_BUILD
