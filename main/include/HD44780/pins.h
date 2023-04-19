#pragma once

// http://1.bp.blogspot.com/-3rGvx1WiEHQ/VRJQyqSIqTI/AAAAAAAASAc/Gs9DN97CM8Q/s1600/PCF8574-LCD-HD44780-schemat.png

// Pin mappings
// P0 -> RS
// P1 -> RW
// P2 -> E
// P3 -> Backlight
// P4 -> D4
// P5 -> D5
// P6 -> D6
// P7 -> D7

// https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller

// RS - only used wor writing characters, otherwise 0
// RW - not used (only write)
// E - used internally for clocking data in

enum lcd_rs_pin_t : uint8_t // RS pin
{
	LCD_RS_PIN_CMND = 0b00000000, // write command to internals
	LCD_RS_PIN_DATA = 0b00000001, // write data to DDR/CGR
};

enum lcd_rw_pin_t : uint8_t // "R ~W" pin
{
	LCD_RW_PIN_WR = 0b00000000, // write data
	LCD_RW_PIN_RD = 0b00000010, // read data
};

enum lcd_ce_pin_t : uint8_t // CE pin
{
	LCD_CE_PIN_DIS = 0b00000000, // clock disable
	LCD_CE_PIN_EN = 0b00000100,	 // clock enable

};

enum lcd_bl_pin_t : uint8_t // BL pin
{
	LCD_BL_PIN_OFF = 0b00000000, // backlight off
	LCD_BL_PIN_ON = 0b00001000,	 // backlight on
};
