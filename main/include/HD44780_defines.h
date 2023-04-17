#pragma once

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

#define LCD_FUNCTION_SET 0x20 // 1 << 5

#define LCD_8BIT_MODE 0x10 // 1 << 4
#define LCD_4BIT_MODE 0x00 // 0
