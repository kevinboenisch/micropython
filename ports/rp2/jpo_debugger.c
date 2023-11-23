#include "jpo_debugger.h"
#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"


void jpo_parse_compile_execute_done(int ret) {
    DBG_SEND("Event: DBG_DONE %d", ret);
    // TODO: set the debugging flag to false


    JCOMP_CREATE_EVENT(evt, 12);
    jcomp_msg_set_str(evt, 0, DBG_DONE);
    jcomp_msg_set_uint32(evt, 8, (uint32_t) ret);
    jcomp_send_msg(evt);
}