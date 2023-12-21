#include "jpo_debugger.h" 

#include <stdio.h>
#include <string.h>

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

// Disable debugging
// #undef DBG_SEND
// #define DBG_SEND(...)


#define CMD_LENGTH 8
#define VARS_PAYLOAD_SIZE JCOMP_MAX_PAYLOAD_SIZE

void dbgr_send_variables_response(const JCOMP_MSG request, dbgr_bytecode_pos_t *bc_stack_top) {
    if (bc_stack_top == NULL) {
        DBG_SEND("Error: dbgr_send_variables_response(): bc_stack_top is NULL");
        return;
    }

    DBG_SEND("In dbgr_send_variables_response");

    JCOMP_CREATE_RESPONSE(resp, jcomp_msg_id(request), VARS_PAYLOAD_SIZE);;
    // TODO: fill out
    JCOMP_RV rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: dbgr_send_variables_response(): jcomp_send_msg() failed");
    }
}