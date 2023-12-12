#ifndef MICROPY_INCLUDED_RP2_JPO_BREAKPOINTS_H
#define MICROPY_INCLUDED_RP2_JPO_BREAKPOINTS_H

#include "mpconfigport.h" // for JPO_DBGR_BUILD
#if JPO_DBGR_BUILD

#include <stdbool.h>
#include "py/qstr.h"
#include "jpo/jcomp_protocol.h"

void bkpt_clear_all();

void bkpt_clear(qstr file);

bool bkpt_is_set(qstr file, int line_num);

void bkpt_set_from_msg(JCOMP_MSG msg);

#endif // JPO_DBGR_BUILD
#endif