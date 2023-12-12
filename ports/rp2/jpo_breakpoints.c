#include "jpo_breakpoints.h"

#include "jpo/jcomp_protocol.h"
#include "jpo/debug.h"

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

/// @brief Compact the breakpoints array, putting all empty items at the bottom
static void bkpt_compact() {
    for(int bp_idx = 0; bp_idx < MAX_BREAKPOINTS; bp_idx++) {
        if (FILE(breakpoints, bp_idx) == 0) {
            // Found a free spot
            for(int bp_idx2 = bp_idx + 1; bp_idx2 < MAX_BREAKPOINTS; bp_idx2++) {
                if (FILE(breakpoints, bp_idx2) != 0) {
                    FILE(breakpoints, bp_idx) = FILE(breakpoints, bp_idx2);
                    LINE(breakpoints, bp_idx) = LINE(breakpoints, bp_idx2);
                    FILE(breakpoints, bp_idx2) = 0;
                    LINE(breakpoints, bp_idx2) = 0; // Could skip
                    break;
                }
            }
        }
    }
}

void bkpt_clear(qstr file) {
    DBG_SEND("bkpt_clear() file:%s", qstr_str(file));

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
            //DBG_SEND("bkpt_is_set() %s:%d not found", qstr_str(file), line_num);
            return false;
        }
        if (FILE(breakpoints, bp_idx) == file
            && LINE(breakpoints, bp_idx) == line_num) {
            // Found it
            //DBG_SEND("bkpt_is_set() %s:%d FOUND", qstr_str(file), line_num);
            return true;
        }
    }
    return false;
}

bool bkpt_set(qstr file, int line_num) {    
    DBG_SEND("bkpt_set() file:%s line:%d", qstr_str(file), line_num);

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
    DBG_SEND("Warning: bkpt_set() no free spot for file:%s line:%d", qstr_str(file), line_num);
    return false;
}

// Expected format: file\0num1num2num3
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
        DBG_SEND("Warning: bkpt file '%s' not found as qstr, ignoring.", file);
        return;
    }

    // Clear all breakpoints for this file
    bkpt_clear(file_qstr);

    // Set the new ones
    uint16_t pos = delim_pos + 1;
    while(pos < jcomp_msg_payload_size(msg)) {
        uint32_t line_num = jcomp_msg_get_uint32(msg, pos);
        pos += 4;

        bkpt_set(file_qstr, line_num);
    }
}

