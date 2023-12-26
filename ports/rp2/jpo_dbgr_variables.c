#include "jpo_debugger.h" 
#include "jpo_dbgr_protocol.h"

#include <stdio.h>
#include <string.h>

#include "py/bc.h" // for mp_code_state_t
#include "py/objfun.h"

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

// Disable debugging
// #undef DBG_SEND
// #define DBG_SEND(...)

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

typedef struct {
    enum _var_scope_type_t scope_type;
    enum _var_list_kind_t list_kind;
    int scope_id;
    int var_start_idx;
} vars_request_t;

typedef struct {
    vstr_t name;
    vstr_t value;
    qstr type;
    uint32_t address;
} var_info_t;

static void var_info_clear(var_info_t* vi) {
    vstr_clear(&(vi->name));
    vstr_clear(&(vi->value));
    vi->type = 0;
    vi->address = 0;
}

typedef struct {
    const vars_request_t* args;
    int size;
    const mp_obj_t* objs;

    int cur_idx;
    var_info_t vi;
} vars_iter_t;

static void iter_clear(vars_iter_t* iter) {
    iter->args = NULL;
    iter->size = 0;
    iter->objs = NULL;

    iter->cur_idx = -1;
    var_info_clear(&iter->vi);
}
static void iter_init(vars_iter_t* iter, const vars_request_t* args, mp_obj_frame_t* top_frame) {
    iter_clear(iter);

    iter->args = args;

    if (args->scope_type == VSCOPE_FRAME) {
        mp_obj_frame_t* frame = dbgr_find_frame(args->scope_id, top_frame);
        if (frame == NULL) {
            return;
        }
        const mp_code_state_t* cur_bc = frame->code_state;
        iter->size = cur_bc->n_state;
        iter->objs = cur_bc->state;
    }
    else if (args->scope_type == VSCOPE_GLOBAL) {
        // TODO
        iter->size = 0;
        iter->objs = NULL;
    }
    else if (args->scope_type == VSCOPE_OBJECT) {
        // TODO
        iter->size = 0;
        iter->objs = NULL;
    }
    else {
        DBG_SEND("Error: iter_start(): unknown scope_type:%d", args->scope_type);
        return;
    }
}
static bool iter_next(vars_iter_t* iter) {
    if (iter->cur_idx >= iter->size) {
        return false;
    }
    iter->cur_idx++;

    // clear previous info
    var_info_clear(&iter->vi);

    if (iter->objs == NULL) {
        return false;
    }

    // skip nulls
    while (iter->objs[iter->cur_idx] == NULL) {
        iter->cur_idx++;
        if (iter->cur_idx >= iter->size) {
            return false;
        }
    }

    mp_obj_t obj = iter->objs[iter->cur_idx];

    // name
    // for local vars (VSCOPE_FRAME/VKIND_VARIABLES) names are not available
    // TODO: for others

    // value
    mp_print_t print_to_vstr;
    vstr_init_print(&(iter->vi.value), 16, &print_to_vstr);
    mp_obj_print_helper(&print_to_vstr, obj, PRINT_REPR);

    // type
    const mp_obj_type_t* obj_type = mp_obj_get_type(obj);
    iter->vi.type = obj_type->name;

    // address: set so that the debugger can examine the object
    if (obj_type == &mp_type_object) {
        iter->vi.address = (uint32_t)obj;
    }

    return true;
}
static int iter_get_size(vars_iter_t* iter) {
    // name, value, type, address
    return (iter->vi.name.len + 
            iter->vi.value.len + 
            strlen(qstr_str(iter->vi.type)) + 
            4);
}
static void iter_append(vars_iter_t* iter, JCOMP_MSG resp) {
    // Ignoring rv for now
    const char* name = vstr_str(&(iter->vi.name));
    const char* value = vstr_str(&(iter->vi.value));
    const char* type = qstr_str(iter->vi.type); 

    jcomp_msg_append_str0(resp, name);
    jcomp_msg_append_str0(resp, value);
    jcomp_msg_append_str0(resp, type);
    jcomp_msg_append_uint32(resp, iter->vi.address);
}


void send_vars_response(uint8_t req_id, const vars_request_t* args, mp_obj_frame_t* top_frame) {
    DBG_SEND("send_vars_response: scope_type:%d list_kind:%d scope_id:%d var_start_idx:%d",
        args->scope_type, args->list_kind, args->scope_id, args->var_start_idx);

    JCOMP_CREATE_RESPONSE(resp, req_id, VARS_PAYLOAD_SIZE);
    if (resp == NULL) {
        DBG_SEND("Error in send_vars_response(): JCOMP_CREATE_RESPONSE failed");
    }

    // subsection_flags
    int pos = 0;
    uint8_t subsection_flags = 0;
    jcomp_msg_append_byte(resp, subsection_flags);
    pos += 1;

    // Fill the items
    vars_iter_t iter;
    iter_init(&iter, args, top_frame);

    int var_idx = 0;
    bool is_end = true;
    while(iter_next(&iter)) {
        if (var_idx >= args->var_start_idx) {
            int var_size = iter_get_size(&iter);
            if (pos + var_size > VARS_PAYLOAD_SIZE) {
                is_end = false;
                break;
            }
            pos += var_size;

            iter_append(&iter, resp);
            // TODO-P3: rv
        }
        var_idx++;
    }

    if (is_end) {        
        if (pos + END_TOKEN_SIZE > VARS_PAYLOAD_SIZE) {
            // no room for the end token, send what we have
        } else {
            // Apend the end token
            jcomp_msg_append_str0(resp, END_TOKEN);
            pos += END_TOKEN_SIZE;
        }
    }
    
    jcomp_msg_set_payload_size(resp, pos);

    JCOMP_RV rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: send_vars_response() send failed: %d", rv);
    }

    DBG_SEND("send_vars_response(): done");
}

void dbgr_send_variables_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame) {
    if (top_frame == NULL || request == NULL) {
        DBG_SEND("Error: dbgr_send_variables_response(): top_frame or request is NULL");
        return;
    }

    int pos = CMD_LENGTH;
    var_scope_type_t scope_type = (var_scope_type_t)jcomp_msg_get_byte(request, pos++);
    var_list_kind_t list_kind = (var_list_kind_t)jcomp_msg_get_byte(request, pos++);
    int scope_id = jcomp_msg_get_uint32(request, pos);
    pos += 4;
    int var_start_idx = jcomp_msg_get_uint32(request, pos);
    pos += 4;

    vars_request_t args = {
        .scope_type = scope_type,
        .list_kind = list_kind,
        .scope_id = scope_id,
        .var_start_idx = var_start_idx,
    };
    send_vars_response(jcomp_msg_id(request), &args, top_frame);
}