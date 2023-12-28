#include "jpo_debugger.h" 
#include "jpo_dbgr_protocol.h"

#include <stdio.h>
#include <string.h>

#include "py/bc.h" // for mp_code_state_t
#include "py/objfun.h"
#include "py/obj.h"

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

// Disable debugging
// #undef DBG_SEND
// #define DBG_SEND(...)

#define VARS_PAYLOAD_SIZE JCOMP_MAX_PAYLOAD_SIZE
#define OBJ_RER_MAX_SIZE 50

void dbg_print_obj(int i, mp_obj_t obj) {
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
} varinfo_t;

static void varinfo_clear(varinfo_t* vi) {
    vstr_clear(&(vi->name));
    vi->name.len = 0;
    
    vstr_clear(&(vi->value));
    vi->value.len = 0;

    vi->type = 0; // qstr
    vi->address = 0;
}

typedef struct {
    const vars_request_t* args;

    // Usually iterating a dict
    mp_obj_dict_t* dict;

    // Sometimes it's a list
    int objs_size;
    const mp_obj_t* objs;

    int cur_idx;
    varinfo_t vi;
} vars_iter_t;

static void iter_clear(vars_iter_t* iter) {
    iter->args = NULL;

    iter->dict = NULL;
    iter->objs_size = 0;
    iter->objs = NULL;

    iter->cur_idx = -1;
    varinfo_clear(&iter->vi);
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

        iter->objs_size = cur_bc->n_state;
        iter->objs = cur_bc->state;
    }
    else if (args->scope_type == VSCOPE_GLOBAL) {
        iter->dict = MP_STATE_THREAD(dict_globals);
    }
    else if (args->scope_type == VSCOPE_OBJECT) {
        // TODO
    }
    else {
        DBG_SEND("Error: iter_start(): unknown scope_type:%d", args->scope_type);
        return;
    }
}
static bool iter_has_next(vars_iter_t* iter) {
    if (iter->dict != NULL) {
        // Copy the index, do we don't advance iter->cur_idx
        size_t idx = iter->cur_idx;
        if (idx == -1) { idx = 0; }

        mp_map_elem_t* elem = dict_iter_next(iter->dict, &idx);
        return (elem != NULL);
    }
    else if (iter->objs != NULL) {
        return (iter->cur_idx < iter->objs_size);
    }
    else {
        return false;
    }
}

// helper
static void varinfo_set_type_and_address(varinfo_t* varinfo, mp_obj_t obj) {
    // type: always set
    const mp_obj_type_t* obj_type = mp_obj_get_type(obj);
    varinfo->type = obj_type->name;

    // address: set for certain types, so debugger can drill down to examine the object
    if (obj_type == &mp_type_object) {
        varinfo->address = (uint32_t)obj;
    }
}
// helper
static void obj_repr_to_vstr(mp_obj_t obj, vstr_t* out_vstr) {
    mp_print_t print_to_vstr;
    vstr_init_print(out_vstr, OBJ_RER_MAX_SIZE, &print_to_vstr);
    mp_obj_print_helper(&print_to_vstr, obj, PRINT_REPR);
}

static varinfo_t* iter_next_dict(vars_iter_t* iter) {
    if (iter->dict == NULL) {
        return NULL;
    }

    if (iter->cur_idx == -1) {
        iter->cur_idx = 0;
    }

    // Advances to the next item
    mp_map_elem_t* elem = dict_iter_next(iter->dict, (size_t*)&(iter->cur_idx));
    if (elem == NULL) {
        return NULL;
    }

    varinfo_t* vi = &(iter->vi); 

    // clear previous info
    varinfo_clear(vi);

    // name is key
    // TODO: this is already a string most likely, can we just pull out out?
    obj_repr_to_vstr(elem->key, &(vi->name));

    // value
    obj_repr_to_vstr(elem->value, &(vi->value));

    // type, address
    varinfo_set_type_and_address(vi, elem->value);

    return vi;
}
static varinfo_t* iter_next_list(vars_iter_t* iter) {
    if (iter->objs == NULL) {
        return NULL;
    }

    // advance to the next
    iter->cur_idx++;
    if (iter->cur_idx >= iter->objs_size) {
        return NULL;
    }

    //DBG_SEND("iter_next() idx:%d size:%d", iter->cur_idx, iter->objs_size);

    // clear previous info
    varinfo_t* vi = &(iter->vi);
    varinfo_clear(vi);

    // might be null
    mp_obj_t obj = iter->objs[iter->cur_idx];
    //dbg_print_obj(iter->cur_idx, obj);

    if (obj != NULL) {
        // name
        // for local vars (VSCOPE_FRAME/VKIND_VARIABLES) names are not available

        // value
        obj_repr_to_vstr(obj, &(vi->value));

        // type, address
        varinfo_set_type_and_address(vi, obj);
    }

    //DBG_SEND("iter_next_list() done");

    return vi;
}

// NULL if no more items
static varinfo_t* iter_next(vars_iter_t* iter) {
    if (iter->dict != NULL) {
        return iter_next_dict(iter);
    }
    else if (iter->objs != NULL) {
        return iter_next_list(iter);
    }
    return NULL;
}

static int varinfo_get_size(varinfo_t* vi) {
    // name, value, type, address
    //DBG_SEND("length of name:%d value:%d type:%d", vi->name.len, vi->value.len, strlen(qstr_str(vi->type)));
    return (vi->name.len + 1 + 
            vi->value.len + 1 +
            strlen(qstr_str(vi->type)) + 1 + 
            4);
}
static void varinfo_append(varinfo_t* vi, JCOMP_MSG resp) {
    // Ignoring rv for now
    const char* name = vstr_str(&(vi->name));
    const char* value = vstr_str(&(vi->value));
    const char* type = qstr_str(vi->type); 

    if (name == NULL) { name = ""; }
    if (value == NULL) { value = ""; }
    if (type == NULL) { type = ""; }

    jcomp_msg_append_str0(resp, name);
    jcomp_msg_append_str0(resp, value);
    jcomp_msg_append_str0(resp, type);
    jcomp_msg_append_uint32(resp, vi->address);
}


void send_vars_response(uint8_t req_id, const vars_request_t* args, mp_obj_frame_t* top_frame) {
    DBG_SEND("send_vars_response: req: scope_type:%d list_kind:%d scope_id:%d var_start_idx:%d",
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
    while(true) {
        varinfo_t* vi = iter_next(&iter);
        if (vi == NULL) {
            break;
        }

        //DBG_SEND("loop: iter->cur_idx:%d var_idx:%d var_start_idx:%d", iter.cur_idx, var_idx, args->var_start_idx);
        if (var_idx >= args->var_start_idx) {
            int var_size = varinfo_get_size(vi);
            //DBG_SEND("pos: %d var_size:%d", pos, var_size);
            if (pos + var_size > VARS_PAYLOAD_SIZE) {
                break;
            }
            pos += var_size;

            varinfo_append(vi, resp);
            // TODO-P3: rv
        }
        var_idx++;
    }

    bool is_end = !iter_has_next(&iter);
    if (is_end) {        
        if (pos + END_TOKEN_SIZE > VARS_PAYLOAD_SIZE) {
            // no room for the end token, send what we have
        } else {
            // Apend the end token
            jcomp_msg_append_str0(resp, END_TOKEN);
            pos += END_TOKEN_SIZE;
        }
    }

    iter_clear(&iter);

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