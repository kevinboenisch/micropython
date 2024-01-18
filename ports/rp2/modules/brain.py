import _jpo

class Motor:
    def __init__(self, port):
        """
        @param port: port the motor is connected to [1-10]
        """
        self._port = port
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

