#include <stdio.h>
#include "jpo_debugger.h"
#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

#include "mphalport.h" // for JPO_DBGR_BUILD

#include "py/runtime.h" // for jpo_bytecode_pos_t
#include "py/qstr.h"
#include "pico/multicore.h"



#define MUTEX_TIMEOUT_MS 100
auto_init_mutex(_dbgr_mutex);

#if JPO_DBGR_BUILD
dbgr_status_t dbgr_status = DS_NOT_ENABLED;

// Reset vars to initial state
void reset_vars() {
    dbgr_status = DS_NOT_ENABLED;
}

#define DBGR_RV_CONTINUE (JCOMP_ERR_CLIENT + 1)

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
        dbgr_status = DS_RUNNING;
        return true;
    }
    if (dbgr_status != DS_NOT_ENABLED) {
        if (jcomp_msg_has_str(msg, 0, CMD_DBG_PAUSE)) {
            DBG_SEND("CMD_DBG_PAUSE");
            // If already stopped, do nothing
            dbgr_status = DS_PAUSE_REQUESTED;
            return true;
        }
        // Other messages are handled on core0, in process_jcomp_message_while_stopped
    }
#endif // JPO_DBGR_BUILD
    return false;
}

static bool core1_dbgr_jcomp_handler(JCOMP_MSG msg) {
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

#if JPO_DBGR_BUILD

static void send_stopped(const char* reason8ch) {
    DBG_SEND("Event: %s%s", EVT_DBG_STOPPED, reason8ch);

    JCOMP_CREATE_EVENT(evt, 16);
    jcomp_msg_set_str(evt, 0, EVT_DBG_STOPPED);
    jcomp_msg_set_str(evt, 8, reason8ch);
    jcomp_send_msg(evt);
}

// Helpers to append "token:"
#define NUM_BUF_SIZE 10
static JCOMP_RV append_int_token(JCOMP_MSG resp, int num) {
    char num_buf[NUM_BUF_SIZE];
    snprintf(num_buf, NUM_BUF_SIZE, "%d", num);
    JCOMP_RV rv = jcomp_msg_append_str(resp, num_buf);
    if (rv) { return rv; }
    return jcomp_msg_append_str(resp, ":");
}
static JCOMP_RV append_str_token(JCOMP_MSG resp, const char* str) {
    JCOMP_RV rv = jcomp_msg_append_str(resp, str);
    if (rv) { return rv; }
    return jcomp_msg_append_str(resp, ":");
}

/**
 * @brief Append a frame to a response, format: "idx:file:line:block::"
 * Frame info might be incomplete (e.g. "idx:file:"" then run out of space). 
 * If complete, it will be terminated with "::"
 * @returns JCOMP_OK if ok, an error (likely JCOMP_ERR_BUFFER_TOO_SMALL) if failed
 */
static JCOMP_RV append_frame(JCOMP_MSG resp, int frame_idx, jpo_bytecode_pos_t *bc_pos) {
    JCOMP_RV rv = JCOMP_OK;

    // frame_idx
    rv = append_int_token(resp, frame_idx);
    if (rv) { return rv; }

    jpo_source_pos_t source_pos = dbgr_get_source_pos(bc_pos);

    // file
    rv = append_str_token(resp, qstr_str(source_pos.file));
    if (rv) { return rv; }

    // line
    rv = append_int_token(resp, source_pos.line);
    if (rv) { return rv; }

    // block
    rv = append_str_token(resp, qstr_str(source_pos.block));
    if (rv) { return rv; }

    // final :, so there's :: terminating the frame
    rv = jcomp_msg_append_str(resp, ":");
    if (rv) { return rv; }

    return JCOMP_OK;
}

/**
 * @brief send a reply to a stack request
 * @param request message, with a 4-byte start frame index
 * @return a string of format "idx:file:line:block::idx:file:line:block::<end>"
 * @note If there's no more space, info, including individual frames, might be incomplete:
 * e.g. "idx:file:line:block::idx:file:line:"
 * Complete frame info ends with "::". 
 * "<end>" alone is a valid response.
 */
static void send_stack_response(JCOMP_MSG request, jpo_bytecode_pos_t *bc_stack_top) {
    // request: 8-byte name, 4-byte start frame index
    uint32_t start_frame_idx = jcomp_msg_get_uint32(request, 8);
    DBG_SEND("start_frame_idx %d", start_frame_idx);
    
    JCOMP_CREATE_RESPONSE(resp, jcomp_msg_id(request), JCOMP_MAX_PAYLOAD_SIZE);
    if (resp == NULL) {
        DBG_SEND("Error in send_stack_reply(): JCOMP_CREATE_RESPONSE failed");
    }

    JCOMP_RV rv = JCOMP_OK;

    jpo_bytecode_pos_t *bc_pos = bc_stack_top;
    int frame_idx = 0;
    bool is_end = false;
    while(true) {
        if (frame_idx >= start_frame_idx) {
            // Try to append frame info as a string. It might fail due to a lack of space. 
            rv = append_frame(resp, frame_idx, bc_pos);
            if (rv) { break; }
        }
        frame_idx++;
        bc_pos = bc_pos->caller_pos;
        if (bc_pos == NULL) {
            is_end = true;
            break;
        }
    }
    // if rv is not OK, we ran out of space in the response, just send what we have
    
    if (is_end) {
        // Apend the end token
        // ok if it doesn't fit, it will be sent by itself on the next response
        jcomp_msg_append_str(resp, "<end>");
    }
    
    rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: send_stack_reply() failed: %d", rv);
    }
}

static JCOMP_RV process_jcomp_message_while_stopped(jpo_bytecode_pos_t *bc_stack_top) {
    JCOMP_RECEIVE_MSG(msg, rv, 0);

    // // Print stack info for debugging
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
        send_stack_response(msg, bc_stack_top);
        return JCOMP_OK;
    }

    return JCOMP_OK;
}

static bool breakpoint_hit(jpo_source_pos_t *cur_pos) {
    return false;
}

static bool source_pos_equal(jpo_source_pos_t *a, jpo_source_pos_t *b) {
    return (a->file == b->file
        && a->line == b->line
        && a->block == b->block
        && a->depth == b->depth);
}

// Called when source position changes (any field)
// last_pos and cur_pos are guaranteed to be different
static void on_pos_change(jpo_source_pos_t *cur_pos, jpo_source_pos_t *last_pos, jpo_bytecode_pos_t *bc_stack_top) {
    // static/global
    int step_over_depth = -1;

    // locals
    char* stopped_reason = "";

    if (breakpoint_hit(cur_pos)) {
        stopped_reason = R_STOPPED_BREAKPOINT;
        dbgr_status = DS_STOPPED;
    }

    // Normal run, no breakpoints or pause/step in progress    
    switch (dbgr_status)
    {
    case DS_RUNNING:
        // Continue execution
        return;

    case DS_PAUSE_REQUESTED:
        stopped_reason = R_STOPPED_PAUSED;
        dbgr_status = DS_STOPPED;
        break;

    case DS_STEP_INTO:
        // Triggered on any source position change
        stopped_reason = R_STOPPED_STEP_INTO;
        dbgr_status = DS_STOPPED;
        break;

    case DS_STEP_OUT:
        // Only triggered if the depth is lower than the last depth
        if (cur_pos->depth < last_pos->depth) {
            stopped_reason = R_STOPPED_STEP_OUT;
            dbgr_status = DS_STOPPED;
        }
        else {
            return;
        }
        break;
    
    case DS_STEP_OVER:
        // Triggered if the depth is same or lower than one set when step over was requested
        if (cur_pos->depth <= step_over_depth) {
            stopped_reason = R_STOPPED_STEP_OVER;
            dbgr_status = DS_STOPPED;
        }
        else {
            return;
        }
        break;

    case DS_STOPPED:
        // Do nothing
        return;

    default:
        DBG_SEND("Error: unexpected dbgr_status: %d, continuing", dbgr_status);
        return;
    }
        
    // Stopped
    send_stopped(stopped_reason);

    while (true) {
        JCOMP_RV rv = process_jcomp_message_while_stopped(bc_stack_top);
        if (rv == DBGR_RV_CONTINUE) {
            DBG_SEND("Continuing");
            dbgr_status = DS_RUNNING;
            break;
        }
        // Spin-wait
        MICROPY_EVENT_POLL_HOOK_FAST;
    }
}

// Main debugger function, called before every opcode execution
void dbgr_process(jpo_bytecode_pos_t *bc_pos) {
    static jpo_source_pos_t last_pos = {0};

    // Already checked, but doesn't hurt
    if (dbgr_status == DS_NOT_ENABLED) {
        return;
    }
    if (bc_pos == NULL) {
        DBG_SEND("Warning: dbgr_check(): bc_pos is NULL, skipping the check");
        return;
    }

    jpo_source_pos_t cur_pos = dbgr_get_source_pos(bc_pos);
    if (source_pos_equal(&cur_pos, &last_pos)) {
        return;
    } 
    on_pos_change(&cur_pos, &last_pos, bc_pos);
    last_pos = cur_pos;
}




//////////////
// Diagnostics
//////////////
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
