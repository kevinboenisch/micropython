// Direct wrappers around JPO HAL C API

#include "py/builtin.h"
#include "py/runtime.h"

#include "mpconfigport.h"

#if JPO_MOD_JPOHAL

// skip: hal_init() is already called by main.c.
// skip: hal_atexit() is used to keep listening for JCOMP commands, 
//       which mpy does anyway

// info()
STATIC mp_obj_t jpohal_info(void) {
    return MP_OBJ_NEW_SMALL_INT(43);
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_info_obj, jpohal_info);

STATIC const mp_rom_map_elem_t mp_module_jpohal_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jpohal) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&jpohal_info_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_jpohal_globals, mp_module_jpohal_globals_table);

const mp_obj_module_t mp_module_jpohal = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_jpohal_globals,
};

MP_REGISTER_MODULE(MP_QSTR_jpohal, mp_module_jpohal);

#endif