#include <stdio.h>
#include <string.h>

#include "jpo_debugger.h"
#include "jpo_dbgr_breakpoints.h"

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

#include "mphalport.h" // for JPO_DBGR_BUILD
#include "py/qstr.h"
#include "py/profile.h"

#include "pico/multicore.h"

// Disable output
#undef DBG_SEND
#define DBG_SEND(...)


#define MUTEX_TIMEOUT_MS 100
auto_init_mutex(_dbgr_mutex);

#if JPO_DBGR_BUILD
#define CMD_LENGTH 8


typedef enum _dbgr_status_t {
    // Debugging not enabled by the PC. Program might be running or done, irrelevant.
    DS_NOT_ENABLED = 0, // -> DS_STARTING
    DS_STARTING,        // -> DS_RUNNING 
    // Debugging enabled, program is running
    DS_RUNNING,         // -> DS_PAUSED, DS_NOT_ENABLED (done)
    // Pause was requested, with _stoppedReason to indicate why
    // Program will continue running in DS_STEP_IN mode until right before the next line
    DS_PAUSE_REQUESTED, // -> DS_STOPPED

    // Stepping into/out/over code
    DS_STEP_INTO,       // -> DS_STOPPED
    DS_STEP_OUT,        // -> DS_STOPPED
    DS_STEP_OVER,       // -> DS_STOPPED

    // Stopped, waiting for commands (e.g. continue, breakpoints). _stoppedReason indicates why.
    // Fires a StoppedEvent when entering.
    DS_STOPPED,         // -> DS_RUNNING, DS_STEP_*

    // Temporarily stopped, waiting for specific commands and a continue. A special event (not stopped) is raised.
    DS_STOPPED_TEMP,

    // Program terminated: DS_NOT_ENABLED
} dbgr_status_t;

dbgr_status_t dbgr_status = DS_NOT_ENABLED;

// type: mp_prof_callback_t
void dbgr_trace_callback(mp_prof_trace_type_t type, mp_obj_frame_t* frame);

// Reset vars to initial state
void reset_vars() {
    dbgr_status = DS_NOT_ENABLED;
    mp_prof_callback_c = NULL;
    bkpt_clear_all();
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
        reset_vars();
        dbgr_status = DS_STARTING;
        mp_prof_callback_c = dbgr_trace_callback;
        return true;
    }
    if (dbgr_status != DS_NOT_ENABLED) {
        if (jcomp_msg_has_str(msg, 0, CMD_DBG_PAUSE)) {
            DBG_SEND("CMD_DBG_PAUSE");
            dbgr_status = DS_PAUSE_REQUESTED;
            return true;
        }
        if (jcomp_msg_has_str(msg, 0, CMD_DBG_SET_BREAKPOINTS)) {
            DBG_SEND("CMD_DBG_SET_BREAKPOINTS");
            bkpt_set_from_msg(msg);
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

    JCOMP_CREATE_EVENT(evt, CMD_LENGTH + 4);
    jcomp_msg_set_str(evt, 0, EVT_DBG_DONE);
    jcomp_msg_set_uint32(evt, CMD_LENGTH, (uint32_t) ret);
    jcomp_send_msg(evt);
}

void jpo_after_parse_compile_execute(int ret) {
    send_done(ret);
    reset_vars();
}

#if JPO_DBGR_BUILD

static bool breakpoint_hit(qstr file, int line_num) {
    bool has_mutex = mutex_enter_timeout_ms(&_dbgr_mutex, MUTEX_TIMEOUT_MS);
    if (!has_mutex) {
        DBG_SEND("Error: breakpoint_hit() failed to get mutex");
        return false;
    }
    bool is_set = bkpt_is_set(file, line_num);
    mutex_exit(&_dbgr_mutex);
    return is_set;
}

static void send_stopped(const char* reason8ch) {
    DBG_SEND("Event: %s %s", EVT_DBG_STOPPED, reason8ch);

    JCOMP_CREATE_EVENT(evt, CMD_LENGTH + 8);
    jcomp_msg_set_str(evt, 0, EVT_DBG_STOPPED);
    jcomp_msg_set_str(evt, CMD_LENGTH, reason8ch);
    jcomp_send_msg(evt);
}

static void send_module_loaded(qstr module_name) {
    DBG_SEND("Event: module loaded file:%d '%s'", module_name, qstr_str(module_name));

    const char* module_name_str = qstr_str(module_name);
    JCOMP_CREATE_EVENT(evt, CMD_LENGTH + sizeof(uint32_t) + strlen(module_name_str));

    jcomp_msg_append_str(evt, EVT_DBG_MODULE_LOADED);
    jcomp_msg_append_uint32(evt, (uint32_t)module_name);
    jcomp_msg_append_str(evt, module_name_str);

    jcomp_send_msg(evt);
}

/**
 * @brief Process a command while the program is stopped
 * Sets dbgr_status depending on the command notably to DS_RUNNING on continue. 
 * @param bc_stack_top the top of the stack, or NULL if not available
 * @return true if a command was processed.
 */
static bool try_process_command(mp_obj_frame_t* frame) {
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
        return false;
    }

    char buf[CMD_LENGTH + 1];
    jcomp_msg_get_str(msg, 0, buf, CMD_LENGTH + 1);
    DBG_SEND("try_process_command: %s", buf);

    if (jcomp_msg_has_str(msg, 0, CMD_DBG_CONTINUE)) {
        dbgr_status = DS_RUNNING;
        return true;
    }
    else if (jcomp_msg_has_str(msg, 0, CMD_STEP_INTO)) {
        dbgr_status = DS_STEP_INTO;
        return true;
    }
    else if (jcomp_msg_has_str(msg, 0, CMD_STEP_OVER)) {
        dbgr_status = DS_STEP_OVER;
        return true;
    }
    else if (jcomp_msg_has_str(msg, 0, CMD_STEP_OUT)) {
        dbgr_status = DS_STEP_OUT;
        return true;
    }
    else if (jcomp_msg_has_str(msg, 0, REQ_DBG_STACK)) {
        dbgr_send_stack_response(msg, frame);
        return true;
    }
    else if (jcomp_msg_has_str(msg, 0, REQ_DBG_VARIABLES)) {
        dbgr_send_variables_response(msg, frame);
        return true;
    }

    DBG_SEND("Error: not a dbgr message id:%d", jcomp_msg_id(msg));
    return false;
}

static int get_call_depth(mp_obj_frame_t* frame) {
    // Slightly inefficient, maybe add a field to frame struct.
    int depth = 0;
    // Warning: frame->back is not set to a previous frame. Not sure what it's meant for. 
    const mp_code_state_t *cur_state = frame->code_state;
    while(cur_state->prev_state) {
        cur_state = cur_state->prev_state;
        depth++;
    }
    return depth;
}

static void on_trace_line(mp_obj_frame_t* frame) {
    // static/global
    // position at the start of the step over/into/out
    static int step_depth = -1;

    // locals
    char* stopped_reason = "";
    qstr file = dbgr_get_source_file(frame->code_state);
    int line = (int)frame->lineno;

    if (breakpoint_hit(file, line)) {
         DBG_SEND("breakpoint_hit %s:%d", qstr_str(file), line);
         stopped_reason = R_STOPPED_BREAKPOINT;
         dbgr_status = DS_STOPPED;
    }

    switch (dbgr_status)
    {
    case DS_RUNNING:
        // Continue execution
        return;

    // Reasons to stop
    case DS_STARTING:
        stopped_reason = R_STOPPED_STARTING;
        dbgr_status = DS_STOPPED;
        break;
    
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
    {
        // Only triggered if the depth is lower than the last depth
        // NOT-BUG: after stepping out, the fn call line is highlighted again.
        // That's ok, PC Python debugger does the same.
        int cur_depth = get_call_depth(frame);
        DBG_SEND("DS_STEP_OUT: cur_depth %d < step_depth %d ?", cur_depth, step_depth);
        if (cur_depth < step_depth) {
            stopped_reason = R_STOPPED_STEP_OUT;
            dbgr_status = DS_STOPPED;
        }
        else {
            return;
        }
        break;
    }
    case DS_STEP_OVER:
    {
        // Triggered if the depth is same or lower than one set when step over was requested
        int cur_depth = get_call_depth(frame);
        DBG_SEND("DS_STEP_OVER: cur_depth %d <= step_depth %d ?", cur_depth, step_depth);
        if (cur_depth <= step_depth) {
            stopped_reason = R_STOPPED_STEP_OVER;
            dbgr_status = DS_STOPPED;
        }
        else {
            return;
        }
        break;
    }
    case DS_STOPPED:
        // Do nothing
        break;

    default:
        DBG_SEND("Error: unexpected dbgr_status: %d, continuing", dbgr_status);
        return;
    }
    
    // Stopped
    send_stopped(stopped_reason);

    while (true) {
        if (try_process_command(frame)) {
            switch(dbgr_status) {
                case DS_RUNNING:
                    return;
                case DS_STEP_INTO:
                case DS_STEP_OUT:
                case DS_STEP_OVER:
                    step_depth = get_call_depth(frame);
                    DBG_SEND("STEP set step_depth=%d", step_depth);
                    return;
                case DS_STOPPED:
                    // do nothing, continue polling while paused
                    break;
                default:
                    // shouldn't happen, but continue polling
                    break;
            }
        }
        // Spin-wait
        MICROPY_EVENT_POLL_HOOK_FAST;
    }
}


void dbgr_after_compile_module(qstr module_name) {
    if (dbgr_status == DS_NOT_ENABLED) {
        return;
    }

    // Save the old status so we can restore it after the pause
    dbgr_status_t old_status = dbgr_status;

    // Special stopped state for the module loaded event.
    // The debugger does not expect any kind of command (e.g. stack trace request),
    // only set breakpoints (optional) and a continue (required). 
    dbgr_status = DS_STOPPED_TEMP;
    send_module_loaded(module_name);

    // Client will send CMD_DBG_SET_BREAKPOINTS, processed on core1,
    // followed by a continue;

    // Wait for a continue command
    // TODO: shouldn't continue if we were trying to step in/over/out before the break, 
    // but do that action instead
    while (true) {
        if (try_process_command(NULL)) {
            if (dbgr_status == DS_RUNNING) {
                break;
            }
        }
        // Spin-wait
        MICROPY_EVENT_POLL_HOOK_FAST;
    }

    // Restore the old status (e.g. step into/over/out)
    dbgr_status = old_status;

}

void dbgr_trace_callback(mp_prof_trace_type_t type, mp_obj_frame_t* frame) {
    if (dbgr_status == DS_NOT_ENABLED) {
        return;
    }

    switch(type) {
        case MP_PROF_TRACE_LINE:
            //DBG_SEND("trace: line %d depth:%d", frame->lineno, get_call_depth(frame));
            on_trace_line(frame);
            break;

        // case MP_PROF_TRACE_CALL:
        //     DBG_SEND("trace: call depth:%d", get_call_depth(frame));
        //     break;
        // case MP_PROF_TRACE_RETURN:
        //     DBG_SEND("trace: return new-depth:%d", get_call_depth(frame));
        //     break;
        // case MP_PROF_TRACE_EXCEPTION:
        //     DBG_SEND("trace: exception new-depth:%d", get_call_depth(frame));
        //     break;
        default:
            // DBG_SEND("Unkown trace type: %s", qstr_str(type));
            break;
    }
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
