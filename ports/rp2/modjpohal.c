// Direct wrappers around JPO HAL C API

#include "py/builtin.h"
#include "py/runtime.h"

#include "mpconfigport.h"

#if JPO_MOD_JPOHAL
#include "jpo/hal.h"
#include "jpo/hal/brain.h"
#include "jpo/hal/iic.h"
#include "jpo/hal/io.h"
#include "jpo/hal/motor.h"
#include "jpo/hal/oled.h"

// === jpo/hal.h

// skip: hal_init() is already called by main.c.
// skip: hal_atexit() is used to keep listening for JCOMP commands, 
//       mpy does that anyway.

// TODO === brain.h
// TODO === iic.h
// TODO === io.h
// TODO === motor.h

// TODO === oled.h

// skip: oled_access_buffer()
// TODO add a way to get/set the entire buffer from a Python byte buffer object
//    (in conjunction with a higher-level fn)

// oled_set_pixel(x, y, is_on) -> None
STATIC mp_obj_t jpohal_oled_set_pixel(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t is_on_obj) {
    size_t x = mp_obj_get_int(x_obj);
    size_t y = mp_obj_get_int(y_obj);
    bool is_on = mp_obj_is_true(is_on_obj);

    bool rv = oled_set_pixel(x, y, is_on);
    if (!rv) {
        mp_raise_ValueError(MP_ERROR_TEXT("pixel out of bounds"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(jpohal_oled_set_pixel_obj, jpohal_oled_set_pixel);

// oled_clear_row(row); -> None
STATIC mp_obj_t jpohal_oled_clear_row(mp_obj_t row_obj) {
    OLED_ROW row = mp_obj_get_int(row_obj);
    if (row < ROW_0 || row > ROW_7) {
        mp_raise_ValueError(MP_ERROR_TEXT("row out of range [0-7]"));
    }
    oled_clear_row(row);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_oled_clear_row_obj, jpohal_oled_clear_row);

// skip: oled_write_char() as Python has no char type, so write_string() is enough

// oled_write_string(row, col, str) -> None
STATIC mp_obj_t jpohal_oled_write_string(mp_obj_t row_obj, mp_obj_t col_obj, mp_obj_t str_obj) {
    OLED_ROW row = mp_obj_get_int(row_obj);
    OLED_COL col = mp_obj_get_int(col_obj);
    if (row < ROW_0 || row > ROW_7) {
        mp_raise_ValueError(MP_ERROR_TEXT("row out of range [0-7]"));
    }
    if (col < COL_0 || col > COL_15) {
        mp_raise_ValueError(MP_ERROR_TEXT("col out of range [0-15]"));
    }
    const char *str = mp_obj_str_get_str(str_obj);

    // skipping the return value, in C code it's not returned
    oled_write_string(row, col, str);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(jpohal_oled_write_string_obj, jpohal_oled_write_string);

// skip: oled_printf(OLED_ROW row, OLED_COL col, const char *fmt, ...);
// implement differently in the Python wrapper, using str() and write_string()

// oled_render() -> False on error, True on success
// don't want to raise an exception, since it's unclear what went wrong
STATIC mp_obj_t jpohal_oled_render(void) {
    bool rv = oled_render();
    return mp_obj_new_bool(rv);
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_oled_render_obj, jpohal_oled_render);

// === Members table ===
STATIC const mp_rom_map_elem_t mp_module_jpohal_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jpohal) },

    { MP_ROM_QSTR(MP_QSTR_oled_set_pixel), MP_ROM_PTR(&jpohal_oled_set_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_clear_row), MP_ROM_PTR(&jpohal_oled_clear_row_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_write_string), MP_ROM_PTR(&jpohal_oled_write_string_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_render), MP_ROM_PTR(&jpohal_oled_render_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_jpohal_globals, mp_module_jpohal_globals_table);

// === Module definition ===
const mp_obj_module_t mp_module_jpohal = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_jpohal_globals,
};

MP_REGISTER_MODULE(MP_QSTR_jpohal, mp_module_jpohal);

#endif