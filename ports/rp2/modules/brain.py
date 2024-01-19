import _jpo

class ColorReading:
    """
    Color reading from the color sensor.
    """
    def __init__(self, clear, red, green, blue):
        self.clear = clear
        self.red = red
        self.green = green
        self.blue = blue

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
        color_tuple = _jpo.iic_color_read(self._port)
        print("color_tuple", color_tuple)
        return ColorReading(*color_tuple)
    
    def set_led(self, red, green, blue, white):
        """
        Set the color of the LED on the sensor.
        """
        # return is for testing
        return _jpo.iic_color_set_led(self._port, (red, green, blue, white))

    def deinit(self):
        _jpo.io_deinit(self._port)

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
        _jpo.io_deinit(self._port)

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
        _jpo.oled_write_string(row, column, text)
        if (self.render_immediately):
            _jpo.oled_render()

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
        for row in range(0, 7):
            _jpo.oled_clear_row(row)
        if (self.render_immediately):
            _jpo.oled_render()

        # TODO: if we have print, reset the line to top


oled = Oled()

