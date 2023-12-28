#ifndef MICROPY_INCLUDED_RP2_JPO_DBGR_PROTOCOL_H
#define MICROPY_INCLUDED_RP2_JPO_DBGR_PROTOCOL_H

#include "mpconfigport.h" // for JPO_DBGR_BUILD

///////////////////
// Always available
///////////////////

#define CMD_LENGTH 8

// PC sends anytime to stop the program.
#define CMD_DBG_TERMINATE   "DBG_TRMT"

// Brain always sends when execution is done. 
// Format: DBG_DONE<u32 exit_value>
#define EVT_DBG_DONE        "DBG_DONE"

//////////////////////
// Debugger build only
//////////////////////
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

// Clear and set new breaakpoints for the given file
// Format: DBG_BRKP<file>\0<u32 line1><u32 line2><u32 line3>...
// Only one packet is sent
#define CMD_DBG_SET_BREAKPOINTS "DBG_BRKP"

// Brain signals program execution was stopped
// Format: DBG_STOP<8-byte reason str>
#define EVT_DBG_STOPPED      "DBG_STOP"
#define R_STOPPED_STARTING   ":STARTIN"
#define R_STOPPED_PAUSED     ":PAUSED_"    
#define R_STOPPED_BREAKPOINT ":BREAKPT"
#define R_STOPPED_STEP_INTO  ":SINT___"
#define R_STOPPED_STEP_OVER  ":SOVR___"
#define R_STOPPED_STEP_OUT   ":SOUT___"

// Brain signals a module was loaded
// Format: DBG_MODL<u8 qstr_code><source_file>
#define EVT_DBG_MODULE_LOADED "DBG_MODL"

/**
 * Request stack at a given frame.
 * 
 * Request format:DBG_STAC
 * - <u32 start_frame_idx> start frame index, 0 = top of the stack
 * 
 * Response format:
 * sequence of
 * - <file_name>\0
 * - <block_name>\0
 * - <u32 line_num>
 * - <u32 frame_idx>
 * ...
 * - "<end>"\0 if there are no more items available
 */
#define REQ_DBG_STACK     "DBG_STAC"

/**
 * Request format: DBG_VARS
 * - <u8 scope_type> type of scope: frame, global, object
 * - <u8 list_kind> kind of variable list to return: special, function, class, variable
 * - <u32 scope_id> frame index (0 = top of the stack) or object address
 * - <u32 var_start_idx> variable start index
 *
 * Response format:
 * - <u8 subsection_flags> 0x1 has_special_vars 0x2 has_function_vars 0x4 has_class_vars 
 * sequence of
 * - <var_name>\0
 * - <var_value>\0
 * - <var_type>\0
 * - <u32 var_address> address if variable can be examined, 0 if it's fixed
 * ...
 * - "<end>"\0 as the final token, skip if there are more
 */
#define REQ_DBG_VARIABLES "DBG_VARS"

typedef enum _var_scope_type_t {
    VSCOPE_FRAME = 1,
    VSCOPE_GLOBAL,
    VSCOPE_OBJECT,
} var_scope_type_t;

typedef enum _varinfo_kind_t {
    VKIND_NORMAL   = 0x1,
    VKIND_SPECIAL  = 0x2,
    VKIND_FUNCTION = 0x4,
    VKIND_CLASS    = 0x8,
    VKIND_ALL      = 0xF,
} varinfo_kind_t;

// 5 chars + \0
#define END_TOKEN_SIZE 6    
#define END_TOKEN "<end>"

#endif // JPO_DBRG_BUILD

#endif