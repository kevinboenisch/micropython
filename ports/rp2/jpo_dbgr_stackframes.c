#include "jpo_debugger.h" 
#include "jpo_dbgr_protocol.h"

#include <stdio.h>
#include <string.h>

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

// Disable debugging
#undef DBG_SEND
#define DBG_SEND(...)

// make it smaller for testing
#define FRAME_PAYLOAD_SIZE JCOMP_MAX_PAYLOAD_SIZE

static JCOMP_RV append_int_token(JCOMP_MSG msg, int num) {
    return jcomp_msg_append_uint32(msg, num);
}
static JCOMP_RV append_str_token(JCOMP_MSG msg, const char* str) {
    return jcomp_msg_append_bytes(msg, (uint8_t*) str, strlen(str) + 1);
}

static int get_frame_size(mp_obj_frame_t* frame) {
    if (frame == NULL) {
        DBG_SEND("Error: get_frame_size(): frame is NULL");
        return 0;
    }
    
    qstr file = dbgr_get_source_file(frame->code_state);
    qstr block = dbgr_get_block_name(frame->code_state);

    const char* str_file = qstr_str(file);
    const char* str_block = qstr_str(block);

    return (strlen(str_file) + 1) + (strlen(str_block) + 1) + 4 + 4;
}

/**
 * @brief Append a frame to a response
 * @returns JCOMP_OK if ok, an error (likely JCOMP_ERR_BUFFER_TOO_SMALL) if failed
 */
static JCOMP_RV append_frame(JCOMP_MSG resp, int frame_idx, mp_obj_frame_t* frame) {
    if (resp == NULL || frame == NULL) {
        DBG_SEND("Error: append_frame(): resp or frame is NULL");
        return JCOMP_ERR_ARG_NULL;
    }

    JCOMP_RV rv = JCOMP_OK;

    qstr file = dbgr_get_source_file(frame->code_state);
    qstr block = dbgr_get_block_name(frame->code_state);
    const char* str_file = qstr_str(file);
    const char* str_block = qstr_str(block);

    // file
    rv = append_str_token(resp, str_file);
    if (rv) { return rv; }

    // block
    rv = append_str_token(resp, str_block);
    if (rv) { return rv; }

    // line
    rv = append_int_token(resp, frame->lineno);
    if (rv) { return rv; }

    // frame_idx
    rv = append_int_token(resp, frame_idx);
    if (rv) { return rv; }


    return JCOMP_OK;
}

int dbgr_get_call_depth(mp_obj_frame_t* frame) {
    // Slightly inefficient, maybe add a field to the frame or state struct.
    int depth = 0;
    // Warning: frame->back is not set to a previous frame. Not sure what it's meant for. 
    const mp_code_state_t *cur_state = frame->code_state;
    while(cur_state->prev_state) {
        cur_state = cur_state->prev_state;
        depth++;
    }
    return depth;
}


mp_obj_frame_t* dbgr_find_frame(int frame_idx, mp_obj_frame_t* top_frame) {
    if (top_frame == NULL) {
        return NULL;
    }
 
    const mp_code_state_t *cur_state = top_frame->code_state;
    int cur_frame_idx = 0;
    while(true) {
        if (cur_frame_idx == frame_idx) {
            return cur_state->frame;
        }

        cur_frame_idx++;
        cur_state = cur_state->prev_state;
        if (cur_state == NULL) {
            break;
        }
    }
    return NULL;
}

/**
 * @brief send a reply to a stack request
 * @param request message, with a 4-byte start frame index
 * @return a packet of format <file>\0<block>\0<4b u32 line_num><4b u32 frame_idx>
 * "<end>" alone is a valid response.
 */
void dbgr_send_stack_response(const JCOMP_MSG request, mp_obj_frame_t* top_frame) {
    if (top_frame == NULL) {
        DBG_SEND("Error: dbgr_send_stack_response(): top_frame is NULL");
        return;
    }
    
    // request: 8-byte name, 4-byte start frame index
    uint32_t start_frame_idx = jcomp_msg_get_uint32(request, CMD_LENGTH);
    DBG_SEND("stack request: start_frame_idx %d", start_frame_idx);
    
    JCOMP_CREATE_RESPONSE(resp, jcomp_msg_id(request), FRAME_PAYLOAD_SIZE);
    if (resp == NULL) {
        DBG_SEND("Error in dbgr_send_stack_response(): JCOMP_CREATE_RESPONSE failed");
    }

    JCOMP_RV rv = JCOMP_OK;

    const mp_code_state_t *cur_state = top_frame->code_state;
    int frame_idx = 0;
    bool is_end = false;
    int pos = 0;
    while(true) {
        if (frame_idx >= start_frame_idx) {
            int frame_size = get_frame_size(cur_state->frame);
            if (pos + frame_size > FRAME_PAYLOAD_SIZE) {
                break;
            }
            pos += frame_size;

            rv = append_frame(resp, frame_idx, cur_state->frame);
            if (rv) { 
                DBG_SEND("Error in dbgr_send_stack_response: append_frame rv:%d", rv);
                return; 
            }
        }
        frame_idx++;

        cur_state = cur_state->prev_state;
        if (cur_state == NULL) {
            is_end = true;
            break;
        }
    }

    // if rv is not OK, we ran out of space in the response, just send what we have
    DBG_SEND("Done appending frames, count:%d pos:%d rv:%d", frame_idx, pos, rv);

    if (is_end) {
        if (pos + END_TOKEN_SIZE > FRAME_PAYLOAD_SIZE) {
            // no room for the end token, send what we have
        } else {
            // Apend the end token
            append_str_token(resp, END_TOKEN);
            pos += END_TOKEN_SIZE;
        }
    }
    
    jcomp_msg_set_payload_size(resp, pos);

    DBG_SEND("about to send stack response pos:%d payload_size:%d", pos, jcomp_msg_payload_size(resp));
    rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: send_stack_reply() failed: %d", rv);
    }
    DBG_SEND("done sending stack response");

}
