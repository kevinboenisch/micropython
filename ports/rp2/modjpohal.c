// Direct wrappers around JPO HAL C API

#include "py/builtin.h"
#include "py/runtime.h"
#include "py/obj.h"

#include "mpconfigport.h"
#include "jpo/jcomp/debug.h" // for DBG_SEND

#if JPO_MOD_JPOHAL
#include "jpo/hal.h"
#include "jpo/hal/brain.h"
#include "jpo/hal/iic.h"
#include "jpo/hal/io.h"
#include "jpo/hal/motor.h"
#include "jpo/hal/oled.h"
#include "jpo/hal/joystick.h"

// Error in the underlying C JPO HAL API
MP_DEFINE_EXCEPTION(JpoHalError, Exception)
MP_DEFINE_EXCEPTION(IicError, JpoHalError)

STATIC NORETURN void raise_JpoHalError() {
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
// If set to True, wrappers will return dummy values or skip operations that might raise a JpoHalError
STATIC mp_obj_t jpohal__set_test_no_hw(mp_obj_t enabled_obj) {
    _test_no_hw = mp_obj_is_true(enabled_obj);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal__set_test_no_hw_obj, jpohal__set_test_no_hw);

// === jpo/hal.h
// skip: hal_init() is already called by main.c.
// skip: hal_atexit() is used to keep listening for JCOMP commands, 
//       mpy does that anyway.

// === brain.h
// brain_get_buttons() -> int (flags: NONE=0, up=1, down=2, cancel=3, enter=4)
mp_obj_t jpohal_brain_get_buttons(void) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    BRAIN_BTN buttons = 0;
    bool rv = brain_get_buttons(&buttons);
    if (!rv) { raise_JpoHalError(); }

    return mp_obj_new_int(buttons);
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_brain_get_buttons_obj, jpohal_brain_get_buttons);

// === iic.h
const char* iic_error_to_string(IIC_ERROR err) {
    switch(err) {
        case IIC_SUCCESS: return "success";
        case IIC_UNCONFIGURED: return "not configured";
        case IIC_NO_DATA: return "no data";
        case IIC_SHTP_INVALID: return "SHTP invalid";
        case IIC_SHTP_INCOMPLETE: return "SHTP incomplete";
        case IIC_SHTP_EOF: return "SHTP eof";
        case IIC_SH2_INVALID: return "SH2 invalid";
        case IIC_SH2_UNIMPLEMENTED: return "SH2 unimplemented";
        case IIC_TIMEOUT: return "timeout";
        case IIC_GENERIC_PROBLEM: return "generic problem";
        default: return "unknown error";
    }
}

void raise_IicError() {
    IIC_ERROR err = iic_error;
    mp_raise_msg_varg(&mp_type_IicError,
        MP_ERROR_TEXT("%s"), iic_error_to_string(err));
}

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
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_init_obj, jpohal_iic_distance_init);

// iic_distance_deinit(iic_port) -> None
STATIC mp_obj_t jpohal_iic_distance_deinit(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_distance_deinit(iic);
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_deinit_obj, jpohal_iic_distance_deinit);

// iic_distance_read(iic_port) -> float in unknown units
STATIC mp_obj_t jpohal_iic_distance_read(mp_obj_t iic_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_obj_new_float(123.45); }

    float reading = 0;
    bool rv = iic_distance_read(iic, &reading);
    if (!rv) { raise_IicError(); }
    return mp_obj_new_float(reading);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_distance_read_obj, jpohal_iic_distance_read);

// iic_color_init(iic_port) -> None
STATIC mp_obj_t jpohal_iic_color_init(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_color_init(iic);
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_color_init_obj, jpohal_iic_color_init);

// iic_color_deinit(iic_port) -> None
STATIC mp_obj_t jpohal_iic_color_deinit(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_color_deinit(iic);
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_color_deinit_obj, jpohal_iic_color_deinit);

// iic_color_read(iic_port) -> tuple(clear: uint16, red: uint16, green: uint16, blue: uint16) 
STATIC mp_obj_t jpohal_iic_color_read(mp_obj_t iic_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

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
        if (!rv) { raise_IicError(); }
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
    MICROPY_EVENT_POLL_HOOK_FAST;

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
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(jpohal_iic_color_set_led_obj, jpohal_iic_color_set_led);

// bool iic_imu_init(IIC iic);
STATIC mp_obj_t jpohal_iic_imu_init(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_imu_init(iic);
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_imu_init_obj, jpohal_iic_imu_init);

// bool iic_imu_deinit(IIC iic);
STATIC mp_obj_t jpohal_iic_imu_deinit(mp_obj_t iic_port_obj) {
    IIC iic = iic_port_to_id(iic_port_obj);

    if (_test_no_hw) { return mp_const_none; }

    bool rv = iic_imu_deinit(iic);
    if (!rv) { raise_IicError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_imu_deinit_obj, jpohal_iic_imu_deinit);

// bool iic_imu_poll_orientation(IIC iic, IIC_IMU_QUATERNION *reading);
STATIC mp_obj_t jpohal_iic_imu_poll_orientation(mp_obj_t iic_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IIC iic = iic_port_to_id(iic_port_obj);

    IIC_IMU_QUATERNION reading = {0};
    if (_test_no_hw) {
        // Test values
        reading.i = 1;
        reading.j = 2;
        reading.k = 3;
        reading.real = 4;
    }
    else {
        bool rv = iic_imu_poll_orientation(iic, &reading);
        if (!rv) { raise_IicError(); }
    }

    mp_obj_t tuple[4];
    tuple[0] = mp_obj_new_float(reading.i);
    tuple[1] = mp_obj_new_float(reading.j);
    tuple[2] = mp_obj_new_float(reading.k);
    tuple[3] = mp_obj_new_float(reading.real);
    return mp_obj_new_tuple(4, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_imu_poll_orientation_obj, jpohal_iic_imu_poll_orientation);

// bool iic_imu_poll_acceleration(IIC iic, IIC_IMU_ACCELERATION *reading);
STATIC mp_obj_t jpohal_iic_imu_poll_acceleration(mp_obj_t iic_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IIC iic = iic_port_to_id(iic_port_obj);

    IIC_IMU_ACCELERATION reading = {0};
    if (_test_no_hw) {
        // Test values
        reading.x = 1;
        reading.y = 2;
        reading.z = 3;
    }
    else {
        bool rv = iic_imu_poll_acceleration(iic, &reading);
        if (!rv) { raise_IicError(); }
    }

    mp_obj_t tuple[3];
    tuple[0] = mp_obj_new_float(reading.x);
    tuple[1] = mp_obj_new_float(reading.y);
    tuple[2] = mp_obj_new_float(reading.z);
    return mp_obj_new_tuple(3, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_iic_imu_poll_acceleration_obj, jpohal_iic_imu_poll_acceleration);

// === io.h

// Convert [1-11] values; 
int io_port_to_id(mp_obj_t io_port_obj) {
    int io_port = mp_obj_get_int(io_port_obj);
    int io_id = io_port - 1 + IO1;

    if (io_id < IO1 || io_id > IO_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("io_port out of range [1-12]"));
    }
    return io_id;
}
int io_port_to_id_adc(mp_obj_t io_port_obj) {
    int io_port = mp_obj_get_int(io_port_obj);
    int io_id = io_port - 1 + IO1;

    // special test, see is_adc_io in io.c
    if (io_id < IO1 || io_id > IO4) {
        mp_raise_ValueError(MP_ERROR_TEXT("adc io_port out of range [1-4]"));
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

//void io_output_init(IO io);
STATIC mp_obj_t jpohal_io_output_init(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    io_output_init(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_output_init_obj, jpohal_io_output_init);

//void io_output_set(IO io, bool is_on);
STATIC mp_obj_t jpohal_io_output_set(mp_obj_t io_port_obj, mp_obj_t is_on_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IO io = io_port_to_id(io_port_obj);
    bool is_on = mp_obj_is_true(is_on_obj);
    io_output_set(io, is_on);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(jpohal_io_output_set_obj, jpohal_io_output_set);

//void io_button_init(IO io);
STATIC mp_obj_t jpohal_io_button_init(mp_obj_t io_port_obj) {
    IO io = io_port_to_id(io_port_obj);
    io_button_init(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_button_init_obj, jpohal_io_button_init);

//bool io_button_is_pressed(IO io);
STATIC mp_obj_t jpohal_io_button_is_pressed(mp_obj_t io_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IO io = io_port_to_id(io_port_obj);
    bool rv = io_button_is_pressed(io);
    return mp_obj_new_bool(rv);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_button_is_pressed_obj, jpohal_io_button_is_pressed);

// same as in io.c
static inline bool is_adc_io(IO io) {
    return io >= IO4;
}
//void io_potentiometer_init(IO io);
STATIC mp_obj_t jpohal_io_potentiometer_init(mp_obj_t io_port_obj) {
    IO io = io_port_to_id_adc(io_port_obj);
    io_potentiometer_init(io);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_potentiometer_init_obj, jpohal_io_potentiometer_init);

//float io_potentiometer_read(IO io);
STATIC mp_obj_t jpohal_io_potentiometer_read(mp_obj_t io_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    IO io = io_port_to_id(io_port_obj);
    float rv = io_potentiometer_read(io);
    return mp_obj_new_float(rv);
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_potentiometer_read_obj, jpohal_io_potentiometer_read);

//bool io_encoder_init_quadrature(IO lower_pin);
STATIC mp_obj_t jpohal_io_encoder_init_quadrature(mp_obj_t io_lower_port_obj) {
    IO io = io_port_to_id(io_lower_port_obj);
    if (io == IO_MAX) { // reversed, descending order
        mp_raise_ValueError(MP_ERROR_TEXT("lower port cannot be IO12"));
    }

    bool rv = io_encoder_init_quadrature(io);
    if (!rv) { raise_JpoHalError(); }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_io_encoder_init_quadrature_obj, jpohal_io_encoder_init_quadrature);

//int32_t io_encoder_read(IO io);
STATIC mp_obj_t jpohal_io_encoder_read(mp_obj_t io_port_obj) {
    MICROPY_EVENT_POLL_HOOK_FAST;

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
    MICROPY_EVENT_POLL_HOOK_FAST;

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

STATIC mp_obj_t jpohal_oled_clear() {
    oled_clear();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_oled_clear_obj, jpohal_oled_clear);

// skip: oled_write_char() as Python has no char type, so oled_printf() is enough

// oled_printf(row, col, str) -> None
// no need to pass a list of args here, fix them up in Python
STATIC mp_obj_t jpohal_oled_printf(mp_obj_t row_obj, mp_obj_t col_obj, mp_obj_t str_obj) {
    OLED_ROW row = mp_obj_get_int(row_obj);
    OLED_COL col = mp_obj_get_int(col_obj);
    if (row < ROW_0 || row > ROW_7) {
        mp_raise_ValueError(MP_ERROR_TEXT("row out of range [0-7]"));
    }
    if (col < COL_0 || col > COL_15) {
        mp_raise_ValueError(MP_ERROR_TEXT("col out of range [0-15]"));
    }
    const char *str = mp_obj_str_get_str(str_obj);

    oled_printf(row, col, str);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(jpohal_oled_printf_obj, jpohal_oled_printf);

// printf with scrolling
// no need to pass a list of args here, fix them up in Python
STATIC mp_obj_t jpohal_oled_printf_line(mp_obj_t str_obj) {
    const char *str = mp_obj_str_get_str(str_obj);

    oled_printf_line(str);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(jpohal_oled_printf_line_obj, jpohal_oled_printf_line);

// oled_render() -> False on error, True on success
// don't want to raise an exception, since it's unclear what went wrong
STATIC mp_obj_t jpohal_oled_render(void) {
    MICROPY_EVENT_POLL_HOOK_FAST;
    
    bool rv = oled_render();
    return mp_obj_new_bool(rv);
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_oled_render_obj, jpohal_oled_render);

// === Joystick ===
// Returns the tuple of buttons/axes, e.g. ((True, True, False), (-1.0, 0, 1.0, 0.73))
STATIC mp_obj_t jpohal_joystick_get_state(void) {
    MICROPY_EVENT_POLL_HOOK_FAST;

    // Works, but there's a fair bit of memory allocation: 3 tuples, floats...
    JOYSTICK_STATE state = {0};
    joystick_read(&state);

    if (_test_no_hw) {
        state.num_buttons = 8;
        state.buttons[0] = 0b10101010;
        state.num_axes = 4;
        state.axes[0] = 0;
        state.axes[1] = 127;
        state.axes[2] = 255;
        state.axes[3] = 0;
    }

    int n_buttons = joystick_button_count(&state);
    mp_obj_t tup_buttons[n_buttons];
    for (int i = 0; i < n_buttons; i++) {
        tup_buttons[i] = joystick_button_is_pressed(&state, i) ? mp_const_true : mp_const_false;
    }

    int n_axes = joystick_axis_count(&state);
    mp_obj_t tup_axes[n_axes];
    for (int i = 0; i < n_axes; i++) {
        tup_axes[i] = mp_obj_new_float(joystick_axis(&state, i));
    }

    mp_obj_t tuple[2];
    tuple[0] = mp_obj_new_tuple(n_buttons, tup_buttons);
    tuple[1] = mp_obj_new_tuple(n_axes, tup_axes);
    return mp_obj_new_tuple(2, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_0(jpohal_joystick_get_state_obj, jpohal_joystick_get_state);


// === Members table ===
STATIC const mp_rom_map_elem_t mp_module_jpohal_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jpohal) },

    { MP_ROM_QSTR(MP_QSTR_brain_get_buttons), MP_ROM_PTR(&jpohal_brain_get_buttons_obj) },

    { MP_ROM_QSTR(MP_QSTR__set_test_no_hw), MP_ROM_PTR(&jpohal__set_test_no_hw_obj) },
    { MP_ROM_QSTR(MP_QSTR_JpoHalError), MP_ROM_PTR(&mp_type_JpoHalError) },
    { MP_ROM_QSTR(MP_QSTR_IicError), MP_ROM_PTR(&mp_type_IicError) },

    { MP_ROM_QSTR(MP_QSTR_iic_imu_init), MP_ROM_PTR(&jpohal_iic_imu_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_imu_deinit), MP_ROM_PTR(&jpohal_iic_imu_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_imu_poll_orientation), MP_ROM_PTR(&jpohal_iic_imu_poll_orientation_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_imu_poll_acceleration), MP_ROM_PTR(&jpohal_iic_imu_poll_acceleration_obj) },

    { MP_ROM_QSTR(MP_QSTR_iic_color_init), MP_ROM_PTR(&jpohal_iic_color_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_deinit), MP_ROM_PTR(&jpohal_iic_color_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_read), MP_ROM_PTR(&jpohal_iic_color_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_color_set_led), MP_ROM_PTR(&jpohal_iic_color_set_led_obj) },

    { MP_ROM_QSTR(MP_QSTR_iic_distance_init), MP_ROM_PTR(&jpohal_iic_distance_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_distance_deinit), MP_ROM_PTR(&jpohal_iic_distance_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_iic_distance_read), MP_ROM_PTR(&jpohal_iic_distance_read_obj) },

    { MP_ROM_QSTR(MP_QSTR_io_deinit), MP_ROM_PTR(&jpohal_io_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_output_init), MP_ROM_PTR(&jpohal_io_output_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_io_output_set), MP_ROM_PTR(&jpohal_io_output_set_obj) },
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
    { MP_ROM_QSTR(MP_QSTR_oled_clear), MP_ROM_PTR(&jpohal_oled_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_printf), MP_ROM_PTR(&jpohal_oled_printf_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_printf_line), MP_ROM_PTR(&jpohal_oled_printf_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_render), MP_ROM_PTR(&jpohal_oled_render_obj) },

    { MP_ROM_QSTR(MP_QSTR_joystick_get_state), MP_ROM_PTR(&jpohal_joystick_get_state_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_jpohal_globals, mp_module_jpohal_globals_table);

// === Module definition ===
const mp_obj_module_t mp_module_jpohal = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_jpohal_globals,
};

MP_REGISTER_MODULE(MP_QSTR__jpo, mp_module_jpohal);

#endif