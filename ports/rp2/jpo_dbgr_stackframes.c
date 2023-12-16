#include "jpo_dbgr_stackframes.h"

#include <stdio.h>
#include <string.h>

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"
#include "jpo_debugger.h" // for dbgr_get_source_pos

#define CMD_LENGTH 8

// Helpers to append "token:"
#define NUM_BUF_SIZE 10
static JCOMP_RV append_int_token(JCOMP_MSG resp, int num) {
    char num_buf[NUM_BUF_SIZE];
    snprintf(num_buf, NUM_BUF_SIZE, "%d", num);
    JCOMP_RV rv = jcomp_msg_append_str(resp, num_buf);
    if (rv) { return rv; }
    return jcomp_msg_append_str(resp, ":");
}
static JCOMP_RV append_str_token(JCOMP_MSG resp, const char* str) {
    JCOMP_RV rv = jcomp_msg_append_str(resp, str);
    if (rv) { return rv; }
    return jcomp_msg_append_str(resp, ":");
}

/**
 * @brief Append a frame to a response, format: "idx:file:line:block::"
 * Frame info might be incomplete (e.g. "idx:file:"" then run out of space). 
 * If complete, it will be terminated with "::"
 * @returns JCOMP_OK if ok, an error (likely JCOMP_ERR_BUFFER_TOO_SMALL) if failed
 */
static JCOMP_RV append_frame(JCOMP_MSG resp, int frame_idx, dbgr_bytecode_pos_t *bc_pos) {
    JCOMP_RV rv = JCOMP_OK;

    // frame_idx
    rv = append_int_token(resp, frame_idx);
    if (rv) { return rv; }

    dbgr_source_pos_t source_pos = dbgr_get_source_pos(bc_pos);

    // file
    rv = append_str_token(resp, qstr_str(source_pos.file));
    if (rv) { return rv; }

    // line
    rv = append_int_token(resp, source_pos.line);
    if (rv) { return rv; }

    // block
    rv = append_str_token(resp, qstr_str(source_pos.block));
    if (rv) { return rv; }

    // final :, so there's :: terminating the frame
    rv = jcomp_msg_append_str(resp, ":");
    if (rv) { return rv; }

    return JCOMP_OK;
}


/**
 * @brief send a reply to a stack request
 * @param request message, with a 4-byte start frame index
 * @return a string of format "idx:file:line:block::idx:file:line:block::<end>"
 * @note If there's no more space, info, including individual frames, might be incomplete:
 * e.g. "idx:file:line:block::idx:file:line:"
 * Complete frame info ends with "::". 
 * "<end>" alone is a valid response.
 */
void dbgr_send_stack_response(const JCOMP_MSG request, dbgr_bytecode_pos_t *bc_stack_top) {
    if (bc_stack_top == NULL) {
        DBG_SEND("Error: send_stack_reply(): bc_stack_top is NULL");
        return;
    }
    
    // request: 8-byte name, 4-byte start frame index
    uint32_t start_frame_idx = jcomp_msg_get_uint32(request, CMD_LENGTH);
    DBG_SEND("send_stack_response start_frame_idx %d", start_frame_idx);
    
    JCOMP_CREATE_RESPONSE(resp, jcomp_msg_id(request), JCOMP_MAX_PAYLOAD_SIZE);
    if (resp == NULL) {
        DBG_SEND("Error in send_stack_reply(): JCOMP_CREATE_RESPONSE failed");
    }

    JCOMP_RV rv = JCOMP_OK;

    dbgr_bytecode_pos_t *bc_pos = bc_stack_top;
    int frame_idx = 0;
    bool is_end = false;
    while(true) {
        if (frame_idx >= start_frame_idx) {
            // Try to append frame info as a string. It might fail due to a lack of space. 
            rv = append_frame(resp, frame_idx, bc_pos);
            if (rv) { break; }
        }
        frame_idx++;
        bc_pos = bc_pos->caller_pos;
        if (bc_pos == NULL) {
            is_end = true;
            break;
        }
    }
    // if rv is not OK, we ran out of space in the response, just send what we have
    
    if (is_end) {
        // Apend the end token
        // ok if it doesn't fit, it will be sent by itself on the next response
        jcomp_msg_append_str(resp, "<end>");
    }
    
    rv = jcomp_send_msg(resp);
    if (rv) {
        DBG_SEND("Error: send_stack_reply() failed: %d", rv);
    }
}
