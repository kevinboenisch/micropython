#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

#include <stdbool.h>
#include "mpconfigport.h" // for JPO_DBGR_BUILD

#include "py/mpstate.h" // for dbgr_bytecode_pos_t
#include "py/profile.h" // for frame

#include "jpo/jcomp_protocol.h" // for JCOMP_MSG


// Minimal debugger features are always enabled
#define JPO_DBGR (1)

///////////////////////////
// Events/commands/requests
///////////////////////////

#if JPO_DBGR_BUILD
    // PC sends to start debugging.
    // Debugging will be stopped when the program terminates.
    #define CMD_DBG_START    "DBG_STRT"
    // Pause execution
    #define CMD_DBG_PAUSE    "DBG_PAUS"
    // Commands while paused
    #define CMD_DBG_CONTINUE "DBG_CONT"

    #define CMD_STEP_INTO    "DBG_SINT"
    #define CMD_STEP_OVER    "DBG_SOVR"
    #define CMD_STEP_OUT     "DBG_SOUT"

    #define CMD_DBG_SET_BREAKPOINTS "DBG_BRKP"

    // Events Brain sends when stopped
    #define EVT_DBG_STOPPED  "DBG_STOP" // + 8-byte reason str
    #define R_STOPPED_STARTING   ":STARTIN"
    #define R_STOPPED_PAUSED     ":PAUSED_"    
    #define R_STOPPED_BREAKPOINT ":BREAKPT"
    #define R_STOPPED_STEP_INTO  ":SINT___"
    #define R_STOPPED_STEP_OVER  ":SOVR___"
    #define R_STOPPED_STEP_OUT   ":SOUT___"

    #define EVT_DBG_MODULE_LOADED "DBG_MODL" // + <u8 qstr_code><source_file>

    // Requests with responses
    #define REQ_DBG_STACK     "DBG_STAC"
    #define REQ_DBG_VARIABLES "DBG_VARS"
#endif

// PC sends anytime to stop the program.
#define CMD_DBG_TERMINATE    "DBG_TRMT"
// Brain always sends when execution is done. 
#define EVT_DBG_DONE         "DBG_DONE" // + 4-byte int exit value


///////////////////
// Always available
// (jpo_*)
///////////////////

/**
 * @brief Initialize the debugger. 
 * Call it even if not JPO_DBGR_BUILD, to support stopping the program etc.
 */
void jpo_dbgr_init(void);

/**
 * @brief Wrap around pyexec.c::parse_compile_execute
 * Call it even if not JPO_DBGR_BUILD.
 */
void jpo_after_parse_compile_execute(int ret);


//////////////////////
// Debugger build only
// (dbgr_*)
//////////////////////
#if JPO_DBGR_BUILD

/** @brief Call after a module has been compiled (mp_compile) */
void dbgr_after_compile_module(qstr module_name);

// in py/vm.c
qstr dbgr_get_block_name(const mp_code_state_t *code_state);
qstr dbgr_get_source_file(const mp_code_state_t *code_state);

// in jpo_dbgr_stackframes.c
void dbgr_send_stack_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame);

// in jpo_dbgr_variables.c
void dbgr_send_variables_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame);

/** @brief Diagonstics. Check if there is a stack overflow, DBG_SEND info. */
bool dbgr_check_stack_overflow(bool show_if_ok);
/** @brief Diagonstics. DBG_SEND stack info. */
void dbgr_print_stack_info(void);

#endif //JPO_DBGR_BUILD

#endif