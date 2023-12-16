#ifndef MICROPY_INCLUDED_RP2_JPO_DBGR_STACKFRAMES_H
#define MICROPY_INCLUDED_RP2_JPO_DBGR_STACKFRAMES_H

#include "mpconfigport.h" // for JPO_DBGR_BUILD
#if JPO_DBGR_BUILD

#include <stdbool.h>
#include <stdint.h>

#include "py/mpstate.h" // for dbgr_bytecode_pos_t
//#include "py/qstr.h"

#include "jpo/jcomp_protocol.h"

void dbgr_send_stack_response(const JCOMP_MSG request, dbgr_bytecode_pos_t *bc_stack_top);



#endif // JPO_DBGR_BUILD
#endif