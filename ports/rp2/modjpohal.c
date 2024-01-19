// Direct wrappers around JPO HAL C API

#include "py/builtin.h"
#include "py/runtime.h"
#include "py/obj.h"

#include "mpconfigport.h"
#include "jpo/debug.h" // for DBG_SEND

#if JPO_MOD_JPOHAL
#include "jpo/hal.h"
#include "jpo/hal/brain.h"
#include "jpo/hal/iic.h"
#include "jpo/hal/io.h"
#include "jpo/hal/motor.h"
#include "jpo/hal/oled.h"

// Error in the underlying C JPO HAL API
MP_DEFINE_EXCEPTION(JpoHalError, Exception)

STATIC NORETURN void raise_JpoHalErrorr() {
    mp_raise_msg(&mp_type_JpoHalError, NULL);
}

STATIC void check_byte_range(int value, const char *name) {
    if (value < 0 || value > 255) {
        mp_raise_msg_varg(&mp_type_ValueError,
            MP_ERROR_TEXT("%s out of range [0-255]"), name);
    }
}

STATIC bool _test_no_hw = false;

// Internal. Allow testing Python wrappers with no hardware device present. 
STATIC mp_obj_t jpohal__set_test_no_hw(mp_obj_t enabled_obj) {
    _test_no_hw = mp_obj_is_true(enabled_obj);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal__set_test_no_hw_obj, jpohal__set_test_no_hw);

// === jpo/hal.h

// skip: hal_init() is already called by main.c.
// skip: hal_atexit() is used to keep listening for JCOMP commands, 
//       mpy does that anyway.

// TODO === brain.h
// === iic.h
// IIC1 = 0... IIC8 = 7
int iic_port_to_id(mp_obj_t iic_port_obj) {
    int iic_port = mp_obj_get_int(iic_port_obj);
    int iic_id = iic_port - 1 + IIC1;

    if (iic_id < IIC1 || iic_id > IIC8) {
        mp_raise_ValueError(MP_ERROR_TEXT("iic_port out of range [1-8]"));
    }
    return iic_id;
}

// iic_distance_init(iic_port) -> None
STATIC mp_obj_t jpohal_iic_distance_init(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);
    
    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_distance_init(iic);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_init_obj, jpohal_iic_distance_init);

// iic_distance_deinit(iic_port) -> None
STATIC mp_obj_t jpohal_iic_distance_deinit(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_distance_deinit(iic);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_deinit_obj, jpohal_iic_distance_deinit);

// iic_distance_read(iic_port) -> float in unknown units
STATIC mp_obj_t jpohal_iic_distance_read(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_obj_new_float(123.45); }

    float reading = 0;
    bool rv = iic_distance_read(iic, &reading);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_obj_new_float(reading);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_read_obj, jpohal_iic_distance_read);

// iic_color_init(iic_port) -> None
STATIC mp_obj_t jpohal_iic_color_init(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_color_init(iic);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_color_init_obj, jpohal_iic_color_init);

// iic_color_deinit(iic_port) -> None
STATIC mp_obj_t jpohal_iic_color_deinit(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_color_deinit(iic);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_color_deinit_obj, jpohal_iic_color_deinit);

// iic_color_read(iic_port) -> tuple(clear: uint16, red: uint16, green: uint16, blue: uint16) 
STATIC mp_obj_t jpohal_iic_color_read(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    IIC_COLOR_READING reading = {0};
    if (_test_no_hw) {
        // Test values
        reading.clear = 1;
        reading.red = 2;
        reading.green = 3;
        reading.blue = 4;
    }
    else {
        bool rv = iic_color_read(iic, &reading);
        if (!rv) { raise_JpoHalErrorr(); }
    }

    mp_obj_t tuple[4];
    tuple[0] = mp_obj_new_int(reading.clear);
    tuple[1] = mp_obj_new_int(reading.red);
    tuple[2] = mp_obj_new_int(reading.green);
    tuple[3] = mp_obj_new_int(reading.blue);
    return mp_obj_new_tuple(4, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_color_read_obj, jpohal_iic_color_read);

// iic_color_set_led(IIC iic, setting: tuple(red: int [0-255], green, blue, white) ) -> None;
STATIC mp_obj_t jpohal_iic_color_set_led(mp_obj_t iic_port_obj, mp_obj_t setting_tuple) {
    IIC iic = iic_port_to_id(iic_port_obj);

    size_t len = 0;    
    mp_obj_t* items = NULL;
    mp_obj_tuple_get(setting_tuple, &len, &items);
    if (len != 4) {
        mp_raise_ValueError(MP_ERROR_TEXT("setting must be a tuple of 4 items"));
    }

    IIC_COLOR_LED_SETTING setting = {0};
    setting.red = mp_obj_get_int(items[0]);
    setting.green = mp_obj_get_int(items[1]);
    setting.blue = mp_obj_get_int(items[2]);
    setting.white = mp_obj_get_int(items[3]);
    check_byte_range(setting.red, "red");
    check_byte_range(setting.green, "green");
    check_byte_range(setting.blue, "blue");
    check_byte_range(setting.white, "white");

    if (_test_no_hw) {
        // test value: if args are 1,2,3,4 return 1234
        return mp_obj_new_int(setting.red * 1000 + setting.green * 100 + setting.blue * 10 + setting.white); 
    }

    bool rv = iic_color_set_led(iic, setting);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(jpohal_iic_color_set_led_obj, jpohal_iic_color_set_led);

// bool iic_imu_init(IIC iic);
// bool iic_imu_deinit(IIC iic);
// bool iic_imu_poll_orientation(IIC iic, IIC_IMU_QUATERNION *reading);
// bool iic_imu_poll_acceleration(IIC iic, IIC_IMU_ACCELERATION *reading);


// === io.h

// Convert [1-11] values; 
int io_port_to_id(mp_obj_t io_port_obj) {
    int io_port = mp_obj_get_int(io_port_obj);
    //DBG_SEND("io_port %d", io_port);
    int io_id = IO1 - (io_port - 1); // IO11 = 19, IO1=29, descending order
    //DBG_SEND("io_id %d [IO1=%d..IO11=%d]", io_id, IO1, IO11);

    if (io_id < IO11 || io_id > IO1) { // reversed, descending order
        mp_raise_ValueError(MP_ERROR_TEXT("io_port out of range [1-11]"));
    }
    return io_id;
}

//void io_deinit(IO io);
STATIC mp_obj_t jpohal_io_deinit(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    io_deinit(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_deinit_obj, jpohal_io_deinit);

//void io_button_init(IO io);
STATIC mp_obj_t jpohal_io_button_init(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    io_button_init(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_button_init_obj, jpohal_io_button_init);

//bool io_button_is_pressed(IO io);
STATIC mp_obj_t jpohal_io_button_is_pressed(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    bool rv = io_button_is_pressed(io);
    return mp_obj_new_bool(rv);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_button_is_pressed_obj, jpohal_io_button_is_pressed);

//void io_potentiometer_init(IO io);
STATIC mp_obj_t jpohal_io_potentiometer_init(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    io_potentiometer_init(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_potentiometer_init_obj, jpohal_io_potentiometer_init);

//float io_potentiometer_read(IO io);
STATIC mp_obj_t jpohal_io_potentiometer_read(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    float rv = io_potentiometer_read(io);
    return mp_obj_new_float(rv);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_potentiometer_read_obj, jpohal_io_potentiometer_read);

//bool io_encoder_init_quadrature(IO lower_pin);
STATIC mp_obj_t jpohal_io_encoder_init_quadrature(mp_obj_t io_lower_port_obj) {
    IO io = io_port_to_id(io_lower_port_obj);
    if (io == IO11) { // reversed, descending order
        mp_raise_ValueError(MP_ERROR_TEXT("lower port cannot be IO11"));
    }

    bool rv = io_encoder_init_quadrature(io);
    if (!rv) { raise_JpoHalErrorr(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_encoder_init_quadrature_obj, jpohal_io_encoder_init_quadrature);

//int32_t io_encoder_read(IO io);
STATIC mp_obj_t jpohal_io_encoder_read(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    int32_t rv = io_encoder_read(io);
    return mp_obj_new_int(rv);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_encoder_read_obj, jpohal_io_encoder_read);

// TODO: io_output_init
// TODO: io_output_set

// === motor.h
Motor motor_port_to_id(mp_obj_t motor_port_obj) {
    int motor_port = mp_obj_get_int(motor_port_obj);
    Motor motor_id = motor_port - 1 + M1;
    if (motor_id < M1 || motor_id > M10) {
        mp_raise_ValueError(MP_ERROR_TEXT("motor_port out of range [1-10]"));
    }
    return motor_id;
}

// motor_set(motor_port, float percent) -> None
// Port is in the [1-10] range.
STATIC mp_obj_t jpohal_motor_set(mp_obj_t port_obj, mp_obj_t percent_obj) {
    Motor motor = motor_port_to_id(port_obj);

    float percent = 0;
    if (!mp_obj_get_float_maybe(percent_obj, &percent)) {
        mp_raise_msg_varg(&mp_type_TypeError,
            MP_ERROR_TEXT("percent: can't convert %s to float"), mp_obj_get_type_str(percent_obj));
    }

    motor_set(motor, percent);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(jpohal_motor_set_obj, jpohal_motor_set);

// === oled.h

// oled_access_buffer() -> bytearray
// Buffer format is an internal implementation detail (e.g [0] is special).
// Exposing to allow Python graphics code to manipulate the buffer directly.
STATIC mp_obj_t jpohal_oled_access_buffer(void) {
    // bytes is immutable, bytearray can be changed
    mp_obj_t ba = mp_obj_new_bytearray_by_ref(oled_buffer_size(), oled_access_buffer());
    return ba;
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_oled_access_buffer_obj, jpohal_oled_access_buffer);

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

    { MP_ROM_QSTR(MP_QSTR__set_test_no_hw), MP_ROM_PTR(&jpohal__set_test_no_hw_obj) },
    { MP_ROM_QSTR(MP_QSTR_JpoHalError), MP_ROM_PTR(&mp_type_JpoHalError) },

    { MP_ROM_QSTR(MP_QSTR_iic_color_init), MP_ROM_PTR(&jpohal_iic_color_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_deinit), MP_ROM_PTR(&jpohal_iic_color_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_read), MP_ROM_PTR(&jpohal_iic_color_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_set_led), MP_ROM_PTR(&jpohal_iic_color_set_led_obj) },

    { MP_ROM_QSTR(MP_QSTR_iic_distance_init), MP_ROM_PTR(&jpohal_iic_distance_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_distance_deinit), MP_ROM_PTR(&jpohal_iic_distance_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_distance_read), MP_ROM_PTR(&jpohal_iic_distance_read_obj) },

    { MP_ROM_QSTR(MP_QSTR_io_deinit), MP_ROM_PTR(&jpohal_io_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_button_init), MP_ROM_PTR(&jpohal_io_button_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_button_is_pressed), MP_ROM_PTR(&jpohal_io_button_is_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_potentiometer_init), MP_ROM_PTR(&jpohal_io_potentiometer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_potentiometer_read), MP_ROM_PTR(&jpohal_io_potentiometer_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_encoder_init_quadrature), MP_ROM_PTR(&jpohal_io_encoder_init_quadrature_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_encoder_read), MP_ROM_PTR(&jpohal_io_encoder_read_obj) },

    { MP_ROM_QSTR(MP_QSTR_motor_set), MP_ROM_PTR(&jpohal_motor_set_obj) },

    { MP_ROM_QSTR(MP_QSTR_oled_access_buffer), MP_ROM_PTR(&jpohal_oled_access_buffer_obj) },
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

MP_REGISTER_MODULE(MP_QSTR__jpo, mp_module_jpohal);

#endif