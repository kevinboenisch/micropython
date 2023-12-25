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

/** @brief For internal use by JPO_BEFORE_EXECUTE_BYTECODE. Do NOT set (except in debugger.c). */
extern dbgr_status_t dbgr_status;

/** @brief Call after a module has been compiled (mp_compile) */
void dbgr_after_compile_module(qstr module_name);

/** @brief in vm.c, for use by debugger.c */
typedef struct _dbgr_source_pos_t {
    qstr file;
    size_t line;
    qstr block;
    uint16_t depth;
} dbgr_source_pos_t;
dbgr_source_pos_t dbgr_get_source_pos(mp_obj_frame_t* frame);


// Internal, in jpo_dbgr_stackframes.c
void dbgr_send_stack_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame);

// Internal, in jpo_dbgr_variables.c
void dbgr_send_variables_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame);


/** @brief Diagonstics. Check if there is a stack overflow, DBG_SEND info. */
bool dbgr_check_stack_overflow(bool show_if_ok);
/** @brief Diagonstics. DBG_SEND stack info. */
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