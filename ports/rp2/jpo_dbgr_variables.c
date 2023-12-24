#include "jpo_debugger.h" 

#include <stdio.h>
#include <string.h>

#include "py/bc.h" // for mp_code_state_t
#include "py/objfun.h"

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

// Disable debugging
// #undef DBG_SEND
// #define DBG_SEND(...)


#define CMD_LENGTH 8
#define VARS_PAYLOAD_SIZE JCOMP_MAX_PAYLOAD_SIZE

void print_obj(int i, mp_obj_t obj) {
    if (obj) {
        mp_printf(&mp_plat_print, "[%d] t:%s ", i, mp_obj_get_type_str(obj));
        mp_obj_print(obj, PRINT_REPR);
        mp_printf(&mp_plat_print, "\n");
    }
    else {
        mp_printf(&mp_plat_print, "[%d] NULL\n", i);
    }
}

void dbgr_send_variables_response(const JCOMP_MSG request, dbgr_bytecode_pos_t *bc_stack_top) {
    if (bc_stack_top == NULL) {
        DBG_SEND("Error: dbgr_send_variables_response(): bc_stack_top is NULL");
        return;
    }

    dbgr_bytecode_pos_t *cur_bc = bc_stack_top;


    DBG_SEND("variables in state: cur_bc:0x%p->n_state:%d", cur_bc, cur_bc->n_state);
    for(int i=0; i < cur_bc->n_state; i++) {
        print_obj(i, cur_bc->state[i]);
    }

    // Default values for arguments
    DBG_SEND("variables in fun_bc->extra_args: n_pos_args:%d has_kw_args:%d", 
        cur_bc->fun_bc->n_pos_args, cur_bc->fun_bc->has_kw_args);
    for (int i=0; i < cur_bc->fun_bc->n_pos_args; i++) {
        print_obj(i, cur_bc->fun_bc->extra_args[i]);
    }
    if (cur_bc->fun_bc->has_kw_args) {
        print_obj(-1, cur_bc->fun_bc->extra_args[cur_bc->fun_bc->n_pos_args]);
    }
    DBG_SEND("variables: done");

    // TODO: also:
    //code_state->fun_bc->context->module->globals;
    //code_state->fun_bc->extra_args;
    //code_state->child_table;


    JCOMP_CREATE_RESPONSE(resp, jcomp_msg_id(request), VARS_PAYLOAD_SIZE);;
    // TODO: fill out
    JCOMP_RV rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: dbgr_send_variables_response(): jcomp_send_msg() failed");
    }
}