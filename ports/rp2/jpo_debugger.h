#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

// Minimal debugger features are always enabled
#define JPO_DBGR (1)

// Debug build is enabled
// There will be two separate Micropython builds: fast (non-debug) and debug.
// TODO: move into build settings, (0) for fast build, (1) for debug
#define JPO_DBGR_BUILD (1)

// Event names are 8 chars
#define DBG_DONE "DBG_DONE"
#define CMD_DBG_STOP "DBG_STOP"

/// @brief Initialize the debugger. 
/// Call it even if not JPO_DBGR_BUILD, to support stopping the program etc.
void jpo_dbgr_init(void);

/// @brief Executing user code finished, either normally or with an error.
/// Call it even if not JPO_DBGR_BUILD.
/// Do not call on every module excution, just for the entire user program.
/// @param ret return value from parse_compile_execute
void jpo_parse_compile_execute_done(int ret);

#ifdef JPO_DBGR_BUILD

extern bool jpo_dbgr_isDebugging;

void jpo_dbgr_check(void);



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