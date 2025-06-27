# pylint: disable line-too-long
# pyright: reportAttributeAccessIssue=false

"""
API for the JPO Brain and connected devices.
"""
import _jpo

class ImuQuaternion:
    """
    Orientation as a quaternion.
    """
    def __init__(self, i: float, j: float, k: float, real: float):
        self.i: float = i
        self.j: float = j
        self.k: float = k
        self.real: float = real

    def __repr__(self):
        return f"ImuQuaternion({self.i}, {self.j}, {self.k}, {self.real})"
    def __str__(self):
        return f"ImuQuaternion(i:{self.i} j:{self.j} k:{self.k} real:{self.real})"

class ImuAcceleration:
    """
    Linear acceleration (i.e., removing the effect of gravity) in [m/s^2].
    """
    def __init__(self, x: float, y: float, z: float):
        self.x: float = x
        self.y: float = y
        self.z: float = z

    def __repr__(self):
        return f"ImuAcceleration({self.x}, {self.y}, {self.z})"
    def __str__(self):
        return f"ImuAcceleration(x:{self.x} y:{self.y} z:{self.z})"

class ColorReading:
    """
    Color reading from the color sensor.
    """
    def __init__(self, clear: int, red: int, green: int, blue: int):
        self.clear: int = clear
        """Clear component of the color reading, range [0-65535]"""
        self.red: int = red
        """Red component of the color reading, range [0-65535]"""
        self.green: int = green
        """Green component of the color reading, range [0-65535]"""
        self.blue: int = blue
        """Blue component of the color reading, range [0-65535]"""

    def __repr__(self):
        return f"ColorReading({self.clear}, {self.red}, {self.green}, {self.blue})"
    def __str__(self):
        return f"ColorReading(clear:{self.clear} red:{self.red} green:{self.green} blue:{self.blue})"

IO_PORT_MIN = 1
IO_PORT_MAX = 12
IIC_PORT_MIN = 1
IIC_PORT_MAX = 8
MOTOR_PORT_MIN = 1
MOTOR_PORT_MAX = 10

# IIC
# ===
class ImuSensor:
    """
    Inertial Measurement Unit (IMU) sensor.
    """
    def __init__(self, iic_port: int):
        """
        Args:
            iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port: int = iic_port
        _jpo.iic_imu_init(self._port)

    def poll_orientation(self) -> ImuQuaternion:
        """
        Returns:
            An ImuQuaternion object with orientation data
        """
        tup = _jpo.iic_imu_poll_orientation(self._port)
        #print("orientation tuple", tup)
        return ImuQuaternion(*tup)

    def poll_acceleration(self) -> ImuAcceleration:
        """
        Polls the accelerometer for linear acceleration (i.e., removing the effect of gravity)

        Returns:
            An ImuAcceleration object with acceleration data in [m/s^2]
        """
        tup = _jpo.iic_imu_poll_acceleration(self._port)
        return ImuAcceleration(*tup)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.iic_imu_deinit(self._port)

class ColorSensor:
    """
    Color detection sensor.
    """
    def __init__(self, iic_port: int):
        """
        Args:
            iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port: int = iic_port
        _jpo.iic_color_init(self._port)

    def read(self) -> ColorReading:
        """
        Returns:
            A ColorReading object with the detected color. Components are in range [0-65535]
        """
        tup = _jpo.iic_color_read(self._port)
        #print("color_tuple", tup)
        return ColorReading(*tup)

    def set_led(self, red: int, green: int, blue: int, white: int):
        """
        Set the color of the LED on the sensor.
        Args:
            red: the red component [0-255]
            green: the green component [0-255]
            blue: the blue component [0-255]
            white: the white component [0-255]
        """
        # return is for testing
        return _jpo.iic_color_set_led(self._port, (red, green, blue, white))

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.iic_color_deinit(self._port)

class DistanceSensor:
    """
    Ultrsonic distance sensor.
    """
    def __init__(self, iic_port: int):
        """
        Args:
            iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port: int = iic_port
        _jpo.iic_distance_init(self._port)

    def read(self) -> float:
        """
        Returns:
            the distance in meters
        """
        return _jpo.iic_distance_read(self._port)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.iic_distance_deinit(self._port)

class AbsoluteEncoder:
    """
    Absolute encoder.
    """
    def __init__(self, iic_port: int):
        """
        Args:
            iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port: int = iic_port
        _jpo.iic_absoluteenc_init(self._port)

    def poll_raw(self) -> int:
        """
        Read the raw 12-bit value of the encoder.

        Returns: 
            the encoder reading
        """
        return _jpo.iic_absoluteenc_poll_raw(self._port, False)

    def poll_raw_continuous(self) -> int:
        """
        Read the raw value of the encoder, without wrapping.
        Correct continuous readings require this function to be called often enough
        that the encoder never rotates by a half-turn or more between successive
        calls, as this makes it impossible to identify the direction of rotation.

        Returns: 
            the encoder reading
        """
        return _jpo.iic_absoluteenc_poll_raw(self._port, True)

    def poll_angle(self) -> float:
        """
        Read the angle of the encoder, in radians.
        This value is always non-negative and strictly less than 2*pi. To get a
        continuous reading, see `poll_angle_continous`.

        Returns: 
            the encoder angle
        """
        return _jpo.iic_absoluteenc_poll_angle(self._port, False)

    def poll_angle_continuous(self) -> float:
        """
        Read the angle of the encoder, in radians, without wrapping.
        This internally uses `poll_raw_continuous`, see that method for more details.
        
        Returns: 
            the encoder angle
        """
        return _jpo.iic_absoluteenc_poll_angle(self._port, True)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.iic_absoluteenc_deinit(self._port)

# I/O
# ===
class Button:
    """
    Button with an on/off state on an IO port.
    """
    def __init__(self, io_port: int):
        """
        Args:
            io_port: IO port the button is connected to [1-12]
        """
        self._port: int = io_port
        _jpo.io_button_init(self._port)

    def is_pressed(self) -> bool:
        """
        Returns:
            True if the button is pressed, False otherwise
        """
        return _jpo.io_button_is_pressed(self._port)

    # Micropython does not call __del__ on object destruction
    # See https://github.com/micropython/micropython/issues/1878
    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.io_deinit(self._port)

class Output:
    """
    Digital output (on/off) on an IO port.
    """
    def __init__(self, io_port: int):
        """
        Args:
            io_port: IO port the output is connected to [1-12]
        """
        self._port: int = io_port
        _jpo.io_output_init(self._port)

    def set(self, is_on):
        """
        Set the output on or off.

        Args:
            is_on: True to set, False to clear
        """
        _jpo.io_output_set(self._port, is_on)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.io_deinit(self._port)

class Potentiometer:
    """
    Potentiometer analog sensor.
    """
    def __init__(self, adc_io_port: int):
        """
        Args:
            adc_io_port: IO port the potentiometer is connected to [1-4]
                Ports above 4 are not supported. 
        """
        self._port: int = adc_io_port
        _jpo.io_potentiometer_init(self._port)

    def read(self) -> float:
        """
        Returns:
            Readout of the potentiometer, range [0-1]
        """
        return _jpo.io_potentiometer_read(self._port)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.io_deinit(self._port)

class MotorQuadratureEncoder:
    """
    Quadrature encoder sensor.
    """
    def __init__(self, io_lower_port: int):
        """
        Args:
            io_lower_port: lower IO port the encoder is connected to [1-11]
                Quadrature encoders use two consecutive IO ports.
                For example, to set up an encoder on IO ports 4 and 5, pass 4 here.
        """
        self._port: int = io_lower_port
        _jpo.io_motorquadenc_init(self._port)

    def read(self) -> int:
        """
        @return: the value of the quadrature encoder, in ticks
        """
        return _jpo.io_motorquadenc_read(self._port)

    def deinit(self):
        """
        Deinitialize the sensor.
        """
        _jpo.io_deinit(self._port)

# Motors
# ======
class Motor:
    """
    Motor connected to a motor port.
    """
    def __init__(self, motor_port: int):
        """
        Args: 
            motor_port: motor port the motor is connected to [1-10]
        """
        self._port: int = motor_port
        # Raises an error port is out of range (and stops the motor if running)
        _jpo.motor_set(self._port, 0)

    def set_speed(self, speed: float):
        """
        Set the speed of the motor.
        Args: 
            speed: the speed in percent [-100.0, 100.0]
        """
        _jpo.motor_set(self._port, speed)

# Brain
# =====
class BrainButtons:
    """
    Buttons built into the Brain: up, down, enter, cancel.
    """

    BTN_NONE: int = 0
    BTN_UP: int = 1 << 0
    BTN_DOWN: int = 1 << 1
    BTN_CANCEL: int = 1 << 2
    BTN_ENTER: int = 1 << 3

    def __init__(self, value: int):
        self.value: int = value

    def is_up_pressed(self) -> bool:
        """
        Returns:
            True if the up button is pressed, False otherwise
        """
        return self.value & BrainButtons.BTN_UP == BrainButtons.BTN_UP

    def is_down_pressed(self) -> bool:
        """
        Returns:
            True if the down button is pressed, False otherwise
        """
        return self.value & BrainButtons.BTN_DOWN == BrainButtons.BTN_DOWN

    def is_cancel_pressed(self) -> bool:
        """
        Returns:
            True if the cancel button is pressed, False otherwise
        """
        return self.value & BrainButtons.BTN_CANCEL == BrainButtons.BTN_CANCEL

    def is_enter_pressed(self) -> bool:
        """
        Returns:
            True if the enter button is pressed, False otherwise
        """
        return self.value & BrainButtons.BTN_ENTER == BrainButtons.BTN_ENTER

class Brain:
    """
    Brain unit with the buttons and an OLED display.
    """
    def __init__(self):
        self.render_immediately: bool = True
        """
        If True, the display is rendered immediately after each operation.
        If False, the display is rendered only after calling render_oled().
        """

    # Brain buttons
    
    def read_buttons(self) -> BrainButtons:
        """
        Read the state of the buttons on the Brain.
        """
        value = self._read_buttons_raw()
        return BrainButtons(value)

    def _read_buttons_raw(self) -> int:
        """
        Retrieve the state of all the buttons. 

        Returns:
            A bitfield indicating state of all the buttons. 
            Check against BrainButtons.BTN_* constants.
        
        Example:
            bb._read_buttons_raw() & BrainButtons.BTN_UP == BrainButtons.BTN_UP
        """
        return _jpo.brain_get_buttons() 

    # Brain OLED

    def oled_buffer(self) -> bytearray:
        """
        Access the internal display buffer.

        Returns:
            Bytearray representing the display buffer. Format is hardware specific. 
            First byte is special, modify others to manipulate pixels. 
        """
        return _jpo.brain_oled_buffer()

    def set_pixel(self, x: int, y: int, is_on = True) -> None:
        """
        Set a pixel on the display.
        When working with individual pixels, for better performance set `render_immediately` to False.

        Args:
            x: the x coordinate [0-127]
            y: the y coordinate [0-63]
            is_on: True to set, False to clear
        """
        # SSD1306_WIDTH, SSD1306_HEIGHT
        _jpo.brain_set_pixel(x, y, is_on)
        if self.render_immediately:
            _jpo.brain_render_oled()

    def clear_pixel(self, x: int, y: int) -> None:
        """
        Clear a pixel on the display.

        Args:
            x: the x coordinate [0-128]
            y: the y coordinate [0-64]
        """
        self.set_pixel(x, y, False)

    def clear_row(self, row: int) -> None:
        """ 
        Clear a row of characters.

        Args:
            row: the row to clear [0-7]
        """
        _jpo.brain_clear_row(row)
        if self.render_immediately:
            _jpo.brain_render_oled()

    def clear_oled(self) -> None:
        """
        Clear the entire display.
        """
        _jpo.brain_clear_oled()
        if self.render_immediately:
            _jpo.brain_render_oled()

    def write_at(self, row: int, column: int, *items) -> None:
        """
        Write a string to the display.

        Args:
            row: the row to write to [0-7]
            column: the column to write to [0-15]
            items: items to write, any object, similar to built-in `print`
        """
        text = ' '.join([str(a) for a in items])
        _jpo.brain_printf(row, column, text)
        if self.render_immediately:
            _jpo.brain_render_oled()

    def print(self, *items) -> None:
        """
        Write a line to the display and scroll as needed. Always renders immediately.

        Args: 
            items: items to write, any object, similar to built-in `print`
        """
        text = ' '.join([str(a) for a in items])
        _jpo.brain_printf_line(text)
        # always renders immediately

    def render_oled(self) -> None:
        """
        Render the display.
        """
        _jpo.brain_render_oled()


class JoystickState:
    """
    State of the joystick (game controller).  
    """
    def __init__(self):
        # get state as two tuples, eg. ((True,False), (0, -1.0, 1.0)) 
        data = _jpo.joystick_get_state()
        self.buttons: tuple[bool] = data[0]
        self.axes: tuple[float] = data[1]

    def button(self, i) -> bool:
        return self.buttons[i]
    
    def axis(self, i) -> float:
        return self.axes[i]

class Joystick:
    """
    Joystick (game controller) connected to the Joystick Module 
    and comunicating with JPO Brain over the radio.
    """
    def __init__(self, continuous_reporting = True):
        """
        Args: 
            continuous_reporting: True to send reports continuously many times per second,
               and reset state to zero if reports are missed. Puts strain on the radio connection.
               False to send reports only when the state changes. 
        """

        self.render_immediately: bool = True
        """
        If True, the display is rendered immediately after each operation.
        If False, the display is rendered only after calling render_oled().
        """

        _jpo.joystick_init(continuous_reporting)

    def deinit(self):
        """
        Deinitialize the joystick, turn off reporting.
        """
        _jpo.joystick_deinit()

    # Joystick state

    def read(self) -> JoystickState:
        """
        Returns: the latest joystick state.
        """
        return JoystickState()

    # Joystick OLED

    def clear_row(self, row: int) -> None:
        """ 
        Clear a row of characters.

        Args:
            row: the row to clear [0-7]
        """
        _jpo.joystick_clear_row(row)
        if self.render_immediately:
            _jpo.joystick_render_oled()

    def clear_oled(self) -> None:
        """
        Clear the entire display.
        """
        _jpo.joystick_clear_oled()
        if self.render_immediately:
            _jpo.joystick_render_oled()

    def write_at(self, row: int, column: int, *items) -> None:
        """
        Write a string to the display.

        Args:
            row: the row to write to [0-7]
            column: the column to write to [0-15]
            items: items to write, any object, similar to built-in `print`
        """
        text = ' '.join([str(a) for a in items])
        _jpo.joystick_printf(row, column, text)
        if self.render_immediately:
            _jpo.joystick_render_oled()

    def print(self, *items) -> None:
        """
        Write a line to the display and scroll as needed. Always renders immediately.

        Args: 
            items: items to write, any object, similar to built-in `print`
        """
        text = ' '.join([str(a) for a in items])
        _jpo.joystick_printf_line(text)
        # always renders immediately

    def render_oled(self) -> None:
        """
        Render the display.
        """
        _jpo.joystick_render_oled()


class MT208JoystickState(JoystickState):
    def __init__(self):
        super().__init__()

    def btn1(self) -> bool: return self.buttons[0]
    def btn2(self) -> bool: return self.buttons[1]
    def btn3(self) -> bool: return self.buttons[2]
    def btn4(self) -> bool: return self.buttons[3]

    def btn_back_left1(self) -> bool: return self.buttons[4]
    def btn_back_right1(self) -> bool: return self.buttons[5]
    def btn_back_left2(self) -> bool: return self.buttons[6]
    def btn_back_right2(self) -> bool: return self.buttons[7]

    def btn_select(self) -> bool: return self.buttons[8]
    def btn_start(self) -> bool: return self.buttons[9]
    def btn_left_stick(self) -> bool: return self.buttons[10]
    def btn_right_stick(self) -> bool: return self.buttons[11]

    def axis_dpad_x(self) -> float: return self.axes[0]
    def axis_dpad_y(self) -> float: return self.axes[1]
    def axis_left_x(self) -> float: return self.axes[2]
    def axis_left_y(self) -> float: return self.axes[3]
    def axis_right_x(self) -> float: return self.axes[4]
    def axis_right_y(self) -> float: return self.axes[5]

class MT208Joystick(Joystick):
    """
    MT208 Joystick (game controller) connected to the Joystick Module 
    and comunicating with JPO Brain over the radio.
    """
    def read(self) -> MT208JoystickState:
        """
        Returns: the latest MT208 joystick state.
        """
        return MT208JoystickState()

if (__name__ == "xjpo"):
    # Print the warning
    print("*** WARNING")
    print("*** Using the `xjpo` module, which is not frozen.")
    print("*** Remember to freeze the code within `jpo` module in Micropython.")
    print()