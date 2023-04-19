#pragma once

enum lcd_newline_as_t : uint8_t
{
	LCD_NL_IGNORE = 0,
	LCD_NL_AS_SP,
	LCD_NL_AS_LF,
	LCD_NL_AS_CRLF
};

struct lcd_str_flags_t
{
	size_t tb_to_sp = 4;
	lcd_newline_as_t nl_as = LCD_NL_AS_CRLF;
};
