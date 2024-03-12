#include "jpo_dbgr_breakpoints.h"
#include "jpo_dbgr_protocol.h"

#include "jpo/jcomp/jcomp_protocol.h"
#include "jpo/jcomp/debug.h"

#if JPO_DBGR_BUILD

// Disable debugging
#undef DBG_SEND
#define DBG_SEND(...)


#define CMD_LENGTH 8
#define MAX_BREAKPOINTS 100

// Odd items are qstr file, even items are line numbers
// Valid items are on top, free items (file=0) are at the bottom
static uint16_t breakpoints[MAX_BREAKPOINTS * 2] = {0};
#define FILE(breakpoints, idx) (breakpoints[idx * 2])
#define LINE(breakpoints, idx) (breakpoints[idx * 2 + 1])


void bkpt_clear_all() {
    for(int bp_idx = 0; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        FILE(breakpoints, bp_idx) = 0;
        LINE(breakpoints, bp_idx) = 0; // Could skip
    }
}

static int find_free_idx(int start) {
    for(int bp_idx = start; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) == 0) {
            return bp_idx;
        }
    }
    return -1;
}
static int find_set_idx(int start) {
    for(int bp_idx = start; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) != 0) {
            return bp_idx;
        }
    }
    return -1;
}

static void dbg_send_breakpoints() {
    int i = 0;
    (void)(i); // suppress warning
    DBG_SEND("[%d] 0:%d/%d 1:%d/%d 2:%d/%d 3:%d:%d 4:%d/%d 5:%d/%d 6:%d/%d 7:%d/%d 8:%d/%d 9:%d/%d", i,
        FILE(breakpoints, i+0), LINE(breakpoints, i+0),
        FILE(breakpoints, i+1), LINE(breakpoints, i+1),
        FILE(breakpoints, i+2), LINE(breakpoints, i+2),
        FILE(breakpoints, i+3), LINE(breakpoints, i+3),
        FILE(breakpoints, i+4), LINE(breakpoints, i+4),
        FILE(breakpoints, i+5), LINE(breakpoints, i+5),
        FILE(breakpoints, i+6), LINE(breakpoints, i+6),
        FILE(breakpoints, i+7), LINE(breakpoints, i+7),
        FILE(breakpoints, i+8), LINE(breakpoints, i+8),
        FILE(breakpoints, i+9), LINE(breakpoints, i+9));
}

/// @brief Compact the breakpoints array, putting all empty items at the bottom
static void bkpt_compact() {
    DBG_SEND("bkpt_compact()");
    dbg_send_breakpoints();

    int cur_idx = 0;
    while(true) {
        int free_idx = find_free_idx(cur_idx);
        if (free_idx == -1) {
            break;
        }
        int next_set_idx = find_set_idx(free_idx + 1);
        if (next_set_idx == -1) {
            // No more set items
            break;
        }
        //DBG_SEND("bkpt_compact() cur_idx:%d free_idx:%d next_set_idx:%d", cur_idx, free_idx, next_set_idx);
        // move set item to free spot
        FILE(breakpoints, free_idx) = FILE(breakpoints, next_set_idx);
        LINE(breakpoints, free_idx) = LINE(breakpoints, next_set_idx);
        // clear the set item
        FILE(breakpoints, next_set_idx) = 0;
        LINE(breakpoints, next_set_idx) = 0;

        // search on from the next spot
        cur_idx = free_idx + 1;
    }

    dbg_send_breakpoints();
}

void bkpt_clear(qstr file) {
    DBG_SEND("bkpt_clear() file:%d '%s'", file, qstr_str(file));

    for(int bp_idx = 0; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) == file) {
            FILE(breakpoints, bp_idx) = 0;
            LINE(breakpoints, bp_idx) = 0; // Could skip
        }
    }
    bkpt_compact();
}

bool bkpt_is_set(qstr file, int line_num) {
    for(int bp_idx = 0; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) == 0) {
            // Reached the end
            //DBG_SEND("bkpt_is_set() %d '%s' line:%d not found", file, qstr_str(file), line_num);
            return false;
        }
        if (FILE(breakpoints, bp_idx) == file
            && LINE(breakpoints, bp_idx) == line_num) {
            // Found it
            //DBG_SEND("bkpt_is_set() %d '%s' line:%d FOUND", file, qstr_str(file), line_num);
            return true;
        }
    }
    return false;
}

bool bkpt_set(qstr file, int line_num) {    
    DBG_SEND("bkpt_set() file:%d '%s' line:%d", file, qstr_str(file), line_num);

    for(int bp_idx = 0; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) == 0) {
            // Free spot
            // Is it safe to cast qstr to uint16_t?
            if (file != (uint16_t)file) { DBG_SEND("Warning: bkpt_set() file qstr:%d doesn't fit in uint16_t", file); }

            FILE(breakpoints, bp_idx) = (uint16_t)file;
            LINE(breakpoints, bp_idx) = (uint16_t)line_num;
            return true;
        }
    }
    // No free spot
    DBG_SEND("Warning: bkpt_set() no free spot for file:%d '%s' line:%d", qstr_str(file), line_num);
    return false;
}

void bkpt_set_from_msg(JCOMP_MSG msg) {
    int delim_pos = jcomp_msg_find_byte(msg, CMD_LENGTH, (uint8_t)'\0');
    if (delim_pos == -1) {
        DBG_SEND("Error: bkpt no '\\0' found");
        return;
    }

    // get file
    size_t file_len = delim_pos - CMD_LENGTH;
    char file[file_len + 1];
    jcomp_msg_get_str(msg, CMD_LENGTH, file, file_len + 1);
    qstr file_qstr = qstr_find_strn(file, file_len);
    if (file_qstr == 0) {
        DBG_SEND("Warning: file '%s' not found as qstr, ignoring.", file);
        return;
    }

    // Clear all breakpoints for this file
    bkpt_clear(file_qstr);

    uint16_t pos = delim_pos + 1;
    while(pos < jcomp_msg_payload_size(msg)) {
        uint32_t line_num = jcomp_msg_get_uint32(msg, pos);
        pos += 4;

        bkpt_set(file_qstr, line_num);
    }
}

#endif //JPO_DBGR_BUILD