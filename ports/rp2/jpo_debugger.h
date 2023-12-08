#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

#include <stdbool.h>
#include "mpconfigport.h" // for JPO_DBGR_BUILD

#include "py/mpstate.h" // for jpo_code_location_t

// Minimal debugger features are always enabled
#define JPO_DBGR (1)

// Event/command names are 8 chars

#if JPO_DBGR_BUILD
    // PC sends to start debugging.
    // Debugging will be stopped when the program terminates.
    #define CMD_DBG_START    "DBG_STRT"
    // Pause execution
    #define CMD_DBG_PAUSE    "DBG_PAUS"
    // Commands while paused
    #define CMD_DBG_CONTINUE "DBG_CONT"

    // Events Brain sends when stopped
    #define EVT_DBG_STOPPED  "DBG_STOP" // + 8-byte reason str
    #define R_STOPPED_PAUSED ":PAUSED_"    

    // Requests with responses
    #define REQ_DBG_STACK    "DBG_STAC"
#endif

// PC sends anytime to stop the program.
#define CMD_DBG_TERMINATE    "DBG_TRMT"
// Brain always sends when execution is done. 
#define EVT_DBG_DONE         "DBG_DONE" // + 4-byte int exit value


/// @brief Initialize the debugger. 
/// Call it even if not JPO_DBGR_BUILD, to support stopping the program etc.
void jpo_dbgr_init(void);

/// @brief Executing user code finished, either normally or with an error.
/// Call it even if not JPO_DBGR_BUILD.
/// Do not call on every module excution, just for the entire user program.
/// @param ret return value from parse_compile_execute
void jpo_parse_compile_execute_done(int ret);


/**
 * Check and perform debugger actions if needed.
 * Blocks execution, returns when done. 
 * @param code_loc current location within the code. 
 *    Not always available, so can't pass it as arg, needs to be a variable. 
 * @param ip instruction pointer
 */
#if JPO_DBGR_BUILD
#define JPO_DBGR_CHECK(ip) \
    if (jpo_dbgr_is_debugging) { \
        if (code_loc) { code_loc->ip = ip; } \
        jpo_dbgr_check(code_loc); \
    }
#else
#define JPO_DBGR_CHECK(ip)
#endif //JPO_DBGR_BUILD


//////////////////////
// Debugger build only
//////////////////////
#if JPO_DBGR_BUILD

/** 
 * @brief true if debugging.
 * @note set to true by the PC immediately before running the program, reset to false by Brain when done.
 */
extern bool jpo_dbgr_is_debugging;

/**
 * @see JPO_DBGR_CHECK macro.
 */
void jpo_dbgr_check(jpo_code_location_t *code_loc);

// in vm.c
void dbgr_get_frame_info(jpo_code_location_t *code_loc, qstr *out_file, size_t *out_line, qstr *out_block);

/** @brief Check if there is a stack overflow, DBG_SEND info. */
bool dbgr_check_stack_overflow(bool show_if_ok);
void dbgr_print_stack_info(void);


// See RobotMesh

// // Initial check, send an event, see if the PC replies
// void dbgr_init(void);

// // Disable so it doesn't interfere with loading of built-ins
// void dbgr_disable(void);

// // True if debugger was disabled.
// // only used in dev.py
// uint8_t dbgr_isDisabled(void);

// /**
//  * Check for debug events at the start of every invocation.
//  * Returns PM_RET_OK if the execution should continue, or
//  * PM_RET_BREAK if it should break to prevent running of code
//  * (shouldRunIterate to be called on the next iteration).
//  */
// PmReturn_t dbgr_shouldRunIterate(void);

// /**
//  * Check if breaking is necessary. Returns PM_RET_OK if
//  * execution should continue, PM_RET_BREAK if it should break
//  * (a breakpoint is set or stepping through code).
//  */
// PmReturn_t dbgr_tryBreak(void);

// /**
//  * Break on error
//  */
// PmReturn_t dbgr_breakOnError(void);

#endif //JPO_DBGR_BUILD


#endif