import _jpo

class ImuQuaternion:
    """
    Orientation as a quaternion.
    """
    def __init__(self, i, j, k, real):
        self.i = i
        self.j = j
        self.k = k
        self.real = real

    def __repr__(self):
        return f"ImuQuaternion({self.i}, {self.j}, {self.k}, {self.real})"
    def __str__(self):
        return f"ImuQuaternion(i:{self.i} j:{self.j} k:{self.k} real:{self.real})"

class ImuAcceleration:
    """
    Linear acceleration (i.e., removing the effect of gravity) in [m/s^2].
    """
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return f"ImuAcceleration({self.x}, {self.y}, {self.z})"
    def __str__(self):
        return f"ImuAcceleration(x:{self.x} y:{self.y} z:{self.z})"

class ColorReading:
    """
    Color reading from the color sensor.
    """
    def __init__(self, clear, red, green, blue):
        self.clear = clear
        self.red = red
        self.green = green
        self.blue = blue

    def __repr__(self):
        return f"ColorReading({self.clear}, {self.red}, {self.green}, {self.blue})"
    def __str__(self):
        return f"ColorReading(clear:{self.clear} red:{self.red} green:{self.green} blue:{self.blue})"


METER_PER_SECOND_SQUARED = 1

# IIC
# ===
class ImuSensor:
    def __init__(self, iic_port):
        """
        @param iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port = iic_port
        _jpo.iic_imu_init(self._port)

    def poll_orientation(self):
        """
        @return: an ImuQuaternion object with orientation data
        """
        tup = _jpo.iic_imu_poll_orientation(self._port)
        #print("orientation tuple", tup)
        return ImuQuaternion(*tup)

    def poll_acceleration(self, unit = METER_PER_SECOND_SQUARED):
        """
        Polls the accelerometer for linear acceleration (i.e., removing the effect of gravity)
        @param unit: unit of acceleration, METER_PER_SECOND_SQUARED 
        @return: an ImuAcceleration object with acceleration data in [m/s^2]
        """
        tup = _jpo.iic_imu_poll_acceleration(self._port)
        return ImuAcceleration(*tup)

    def deinit(self):
        _jpo.iic_imu_deinit(self._port)

class ColorSensor:
    def __init__(self, iic_port):
        """
        @param iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port = iic_port
        _jpo.iic_color_init(self._port)

    def read(self):
        """
        @return: a ColorReading object with detected color
        """
        tup = _jpo.iic_color_read(self._port)
        #print("color_tuple", tup)
        return ColorReading(*tup)
    
    def set_led(self, red, green, blue, white):
        """
        Set the color of the LED on the sensor.
        """
        # return is for testing
        return _jpo.iic_color_set_led(self._port, (red, green, blue, white))

    def deinit(self):
        _jpo.iic_color_deinit(self._port)

class DistanceSensor:
    def __init__(self, iic_port):
        """
        @param iic_port: IIC port the sensor is connected to [1-8]
        """
        self._port = iic_port
        _jpo.iic_distance_init(self._port)

    def read(self):
        """
        @return: the distance (TODO: specify unit)
        """
        return _jpo.iic_distance_read(self._port)

    def deinit(self):
        _jpo.iic_distance_deinit(self._port)

# I/O
# ===
class Button:
    def __init__(self, io_port):
        """
        @param io_port: IO port the button is connected to [1-11]
        """
        self._port = io_port
        _jpo.io_button_init(self._port)

    def is_pressed(self):
        """
        @return: True if the button is pressed, False otherwise
        """
        return _jpo.io_button_is_pressed(self._port)
    
    # Micropython does not call __del__ on object destruction
    # See https://github.com/micropython/micropython/issues/1878
    def deinit(self):
        _jpo.io_deinit(self._port)

class Output:
    def __init__(self, io_port):
        """
        @param io_port: IO port the output is connected to [1-11]
        """
        self._port = io_port
        _jpo.io_output_init(self._port)

    def set(self, is_on):
        """
        Set the output.
        @param is_on: True to set, False to clear
        """
        _jpo.io_output_set(self._port, is_on)

    def deinit(self):
        _jpo.io_deinit(self._port)

class Potentiometer:
    def __init__(self, io_lower_port):
        """
        @param io_lower_port: lower IO port the potentiometer is connected to [1-10]
               For example, to set up an encoder on ports 4 and 5, pass 4 here.
        """
        self._port = io_lower_port
        _jpo.io_potentiometer_init(self._port)

    def read(self):
        """
        @return: the value of the potentiometer [0-100]
        """
        return _jpo.io_potentiometer_read(self._port)

    def deinit(self):
        _jpo.io_deinit(self._port)

class QuadratureEncoder:
    def __init__(self, io_port):
        """
        @param io_port: IO port the encoder is connected to [1-11]
        """
        self._port = io_port
        _jpo.io_encoder_init_quadrature(self._port)

    def read(self):
        """
        @return: the value of the encoder [-100, 100]
        """
        return _jpo.io_encoder_read(self._port)

    def deinit(self):
        _jpo.io_deinit(self._port)

# Motors
# ======
class Motor:
    def __init__(self, motor_port):
        """
        @param motor_port: port the motor is connected to [1-10]
        """
        self._port = motor_port
        # Raises an error port is out of range (and stops the motor if running)
        _jpo.motor_set(self._port, 0)

    def set_speed(self, speed):
        """
        Set the speed of the motor.
        @param speed: the speed in percent [-100.0, 100.0]
        """
        _jpo.motor_set(self._port, speed)        

# Brain
# =====
class BrainButtons:
    BTN_NONE = 0
    BTN_UP = 1 << 0
    BTN_DOWN = 1 << 1
    BTN_CANCEL = 1 << 2
    BTN_ENTER = 1 << 3

    def __init__(self):
        pass

    def read(self):
        return _jpo.brain_get_buttons()

    def is_up_pressed(self):
        return _jpo.brain_get_buttons() & BrainButtons.BTN_UP == BrainButtons.BTN_UP
    
    def is_down_pressed(self):
        return _jpo.brain_get_buttons() & BrainButtons.BTN_DOWN == BrainButtons.BTN_DOWN
    
    def is_cancel_pressed(self):
        return _jpo.brain_get_buttons() & BrainButtons.BTN_CANCEL == BrainButtons.BTN_CANCEL
    
    def is_enter_pressed(self):
        return _jpo.brain_get_buttons() & BrainButtons.BTN_ENTER == BrainButtons.BTN_ENTER

class Oled:
    def __init__(self):
        self.render_immediately = True

    def set_pixel(self, x, y, is_on = True):
        """
        Set a pixel on the display.
        @param x: the x coordinate
        @param y: the y coordinate
        @param is_on: True to set, False to clear
        """
        _jpo.oled_set_pixel(x, y, is_on)
        if (self.render_immediately):
            _jpo.oled_render()
    
    def clear_pixel(self, x, y):
        """
        Clear a pixel on the display.
        @param x: the x coordinate
        @param y: the y coordinate
        """
        self.set_pixel(x, y, False)
    
    def clear_row(self, row):
        """ 
        Clear a row of pixels
        @param row: the row to clear [0-7]
        """
        _jpo.oled_clear_row(row)
        if (self.render_immediately):
            _jpo.oled_render()

    def write_at(self, row, column, *items):
        """
        Write a string to the display
        @param row: the row to write to [0-7]
        @param column: the column to write to [0-15]
        @param items: items to write
        """
        text = ' '.join([str(a) for a in items])
        _jpo.oled_printf(row, column, text)
        if (self.render_immediately):
            _jpo.oled_render()

    def print(self, *items):
        """
        Write a line to the display and scroll as needed
        @param items: items to write
        """
        text = ' '.join([str(a) for a in items])
        _jpo.oled_printf_line(text)
        # always renders immediately

    def render(self):
        _jpo.oled_render()

    # TODO: get/set buffer

    # TODO: print with scrolling and line-clearing,
    # similar to built-in print
    # maybe separate into another class?
    # we alrady have something like this in C,
    # with DBG_OLED, maybe add that to hal API?

    def clear(self):
        """
        Clear the display.
        """
        _jpo.oled_clear()
        if (self.render_immediately):
            _jpo.oled_render()

        # TODO: if we have print, reset the line to top
