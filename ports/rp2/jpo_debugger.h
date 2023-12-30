#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

#include <stdbool.h>
#include "mpconfigport.h" // for JPO_DBGR_BUILD

#include "py/mpstate.h" // for dbgr_bytecode_pos_t
#include "py/profile.h" // for frame

#include "jpo/jcomp_protocol.h" // for JCOMP_MSG

// Minimal debugger features are always enabled
#define JPO_DBGR (1)

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
int dbgr_get_call_depth(mp_obj_frame_t* frame);
mp_obj_frame_t* dbgr_find_frame(int frame_idx, mp_obj_frame_t* top_frame);

// in jpo_dbgr_variables.c
void dbgr_send_variables_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame);

// in objdict.c
mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur);

// in modbuiltins.c 
mp_obj_t mp_builtin_dir(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_builtin_getattr(size_t n_args, const mp_obj_t *args);
// in objclosure.c
void closure_get_closed(mp_obj_t closure_in, size_t *n_closed, mp_obj_t **closed);


/** @brief Diagonstics. Check if there is a stack overflow, DBG_SEND info. */
bool dbgr_check_stack_overflow(bool show_if_ok);
/** @brief Diagonstics. DBG_SEND stack info. */
void dbgr_print_stack_info(void);

#endif //JPO_DBGR_BUILD

#endif