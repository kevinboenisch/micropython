#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

#define JPO_DEBUG (1)

// Event names are 8 chars
#define DBG_DONE "DBG_DONE"
#define CMD_DBG_STOP "DBG_STOP"

void jpo_dbgr_init(void);

/// @brief Executing user code finished, either normally or with an error.
/// Call every time, even if debugging is not enabled.
/// Do not call on every module excution, just for the entire user program.
/// @param ret return value from parse_compile_execute
void jpo_parse_compile_execute_done(int ret);

#endif