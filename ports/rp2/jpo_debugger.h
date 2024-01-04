#ifndef MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H
#define MICROPY_INCLUDED_RP2_JPO_DEBUGGER_H

#include <stdbool.h>
#include "mpconfigport.h" // for JPO_DBGR_BUILD

#include "py/mpconfig.h" // for MICROPY_MODULE_FROZEN
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
mp_map_elem_t *mp_map_iter_next(const mp_map_t *map, size_t *cur);

// in modbuiltins.c 
mp_obj_t mp_builtin_dir(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_builtin_getattr(size_t n_args, const mp_obj_t *args);
// in objclosure.c
void closure_get_closed(mp_obj_t closure_in, size_t *n_closed, mp_obj_t **closed);

#if MICROPY_MODULE_FROZEN
extern const char mp_frozen_names[];
#endif

// in jpo_dbgr_util.c
/// @brief Print object info (index, type, rerp) to stdout
void dbgr_print_obj(int i, mp_obj_t obj);
/// @brief Convert an object to a string, respecting max_length
void dbgr_obj_to_vstr(mp_obj_t obj, vstr_t* out_vstr, mp_print_kind_t print_kind, size_t max_length);

/// @brief Diagonstics. Check if there is a stack overflow, DBG_SEND info.
bool dbgr_check_stack_overflow(bool show_if_ok);
/// @brief Diagonstics. DBG_SEND stack info.
void dbgr_print_stack_info(void);

#endif //JPO_DBGR_BUILD

#endif