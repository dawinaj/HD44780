# ESP32 HD44780 driver/library

This is a header-only C++ style library (see files in `main/include/`) for driving the popular **HD44780** LCD display from an **ESP32** microcontroller.
Currently, there exist two Interfaces - one using **PCF8574** expander via **I2C** and one using 4-bit **GPIO** connection.
Feel free to suggest or write other Interfaces (like for other expanders, or full 8-bit 12-pin communication).

Main source file contains simple tests. You can see the results [here](https://imgur.com/a/MCVgFki). See performance [here](https://www.youtube.com/watch?v=B4rCxr45nzM).

## Installation & usage
See example in `main/main.cpp`.

- Move the files from `main/include/` to your include directory.
- #include the `HD44780.h` and `PCF8574.h` or `GPIO4BIT.h` in your code.
- Create `HD44780_PCF8574` or `HD44780_GPIO4BIT` object.
- Create `HD44780` object with the earlier Interface object.
- Done!
