# ESP32 HD44780 driver/library

This is a header-only C++ style library (see files in `main/include/`) for driving the popular **HD44780** LCD display from an **ESP32** microcontroller.
Currently, there exists one Connector - using **PCF8574** 8-bit expander via **I2C**.
Feel free to suggest or write other Connectors (like for other expanders, or full 16-pin communication).

Main source file contains simple tests. You can see the results [here](https://imgur.com/a/MCVgFki).

For C-style library, see [bradkeifer/esp32-HD44780](https://github.com/bradkeifer/esp32-HD44780), from which this library is sourced.

## Installation & usage
- Move the files from `main/include/` to your include directory.
- #include the `HD44780.h` and `PCF8574.h` in your code.
- Create `PCF8574` object.
- Create `HD44780<PCF8574>` templated object.
- Done!
- 
