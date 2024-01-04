#include "jpo_debugger.h" 

#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/objtype.h"

#include "jpo/debug.h"

// Improvement on vstr.c::vstr_add_strn
// Add a string if there's enough space; truncate if needed. Does NOT add \0.
static void vstr_add_strn_if_space(vstr_t *vstr, const char *str, size_t len) {
    if (vstr->len >= vstr->alloc) {
        //DBG_SEND("vstr_add_strn_if_space(): full, skipping '%s'", str);
        return;
    }
    if (vstr->len + len >= vstr->alloc) {
        // Truncate added string
        //DBG_SEND("vstr_add_strn_if_space() partial vs->len:%d vs->alloc:%d len:%d '%s'", vstr->len, vstr->alloc, len, str);
        len = vstr->alloc - vstr->len;
    }

    memmove(vstr->buf + vstr->len, str, len);
    vstr->len += len;
}
// Improvement on vstr.c::vstr_init_print, respect max_length
static void vstr_init_print_if_space(vstr_t *vstr, size_t max_length, mp_print_t *print) {
    vstr_init(vstr, max_length);
    print->data = vstr;
    print->print_strn = (mp_print_strn_t)vstr_add_strn_if_space;
}
void dbgr_obj_to_vstr(mp_obj_t obj, vstr_t* out_vstr, mp_print_kind_t print_kind, size_t max_length) {
    mp_print_t print_to_vstr;
    // Leave space for \0
    vstr_init_print_if_space(out_vstr, max_length - 1, &print_to_vstr);

    if (print_kind == PRINT_EXC) {
        // Prints a traceback, not the same as mp_obj_exception_print (used by mp_obj_print_helper)
        mp_obj_print_exception(&print_to_vstr, obj);
    }
    else {
        mp_obj_print_helper(&print_to_vstr, obj, print_kind);
    }

    // Add ... if truncated
    if (vstr_len(out_vstr) >= max_length - 1) {
        vstr_cut_tail_bytes(out_vstr, 3);
        vstr_add_str(out_vstr, "...");
    }

    vstr_null_terminated_str(out_vstr);
}


//////////
// Helpers
//////////
void dbgr_print_obj(int i, mp_obj_t obj) {
    if (obj) {
        mp_printf(&mp_plat_print, "[%d] t:%s ", i, mp_obj_get_type_str(obj));
        mp_obj_print(obj, PRINT_REPR);
        mp_printf(&mp_plat_print, "\n");
    }
    else {
        mp_printf(&mp_plat_print, "[%d] NULL\n", i);
    }
}

//////////////
// Diagnostics
//////////////
extern uint8_t __StackTop, __StackBottom;
extern uint8_t __StackOneBottom, __StackOneTop;

void dbgr_print_stack_info(void) {
    DBG_SEND("__StackTop:%p __StackBottom:%p __StackOneTop:%p __StackOneBottom:%p // s0size:%d", 
             &__StackTop,  &__StackBottom,  &__StackOneTop,  &__StackOneBottom,
             &__StackTop - &__StackOneTop);
}

bool dbgr_check_stack_overflow(bool show_if_ok) {
    uint32_t stack_size = &__StackTop - &__StackOneTop;

    // using the *address* of stack_size (last var on the stack), not the actual size
    int remaining = (uint32_t)&stack_size - (uint32_t)&__StackOneTop;
    
    if (remaining < 0) {
        DBG_SEND("ERROR: Stack overflow. this:%p __StackOneTop:%p size:%d remaining:%d", 
            &stack_size, &__StackOneTop, stack_size, remaining);
        return true;
    }

    if (show_if_ok) {
        DBG_SEND("Stack ok. this:%p __StackOneTop:%p size:%d remaining:%d", 
            &stack_size, &__StackOneTop, stack_size, remaining);
    }
    return false;
}

