#pragma once

#include <esp_check.h>
#include <rom/ets_sys.h>

#include "HD44780/commands.h"
#include "HD44780/dimensions.h"
#include "HD44780/symbol.h"
#include "HD44780/str_flags.h"

#define TAG "HD44780"

template <typename Comm>
class HD44780
{
public:
	Comm &com;
	const lcd_dims_t screen_size;
	const display_function_t function;

	display_entry_t entry;
	display_control_t control;
	lcd_backlight_t backlight;

private:
	lcd_dims_t cursor_pos;

public:
	HD44780(Comm &c, lcd_dims_t sz, lcd_lines_t l = LCD_2LINE, lcd_dots_t d = LCD_5x8DOTS) : com(c), screen_size(sz), function(com.mode, l, d), entry(), control(), backlight(), cursor_pos(0, 0)
	{
		control.display = LCD_DISPLAY_ON;
		control.cursor = LCD_CURSOR_OFF;
		control.blink = LCD_BLINK_OFF;

		entry.entry = LCD_ENTRY_INCREMENT;
		entry.shift = LCD_DISPLAY_NOSHIFT;

		backlight = LCD_BACKLIGHT_ON;

		// Reset sequence (Doc Figs 23 & 24)
		com.write_init(LCD_SET_FUNCTION | LCD_8BIT);
		vTaskDelay(pdMS_TO_TICKS(8)); // 4.1 ms delay (min)
		com.write_init(LCD_SET_FUNCTION | LCD_8BIT);
		ets_delay_us(200); // 100 us delay (min)
		com.write_init(LCD_SET_FUNCTION | LCD_8BIT);

		if (function.mode == LCD_4BIT) // Activate 4-bit mode
		{
			com.write_init(LCD_SET_FUNCTION | LCD_4BIT);
		}

		// update_function();
		// update_control();
		// clear();
		// update_entry();
		// home();
		update_settings();
		clear();

		//
	}
	// HD44780(Comm &c, uint8_t h, uint8_t w, lcd_lines_t l = LCD_2LINE, lcd_dots_t d = LCD_5x8DOTS) : HD44780(c, lcd_dims_t(h, w), l, d) {}
	~HD44780() {}

	//

	void set(lcd_entry_t i)
	{
		entry.entry = i;
	}
	void set(lcd_shift_t i)
	{
		entry.shift = i;
	}

	void set(lcd_display_t i)
	{
		control.display = i;
	}
	void set(lcd_cursor_t i)
	{
		control.cursor = i;
	}
	void set(lcd_blink_t i)
	{
		control.blink = i;
	}

	void set(lcd_mode_t i)
	{
		function.mode = i;
	}
	void set(lcd_lines_t i)
	{
		function.lines = i;
	}
	void set(lcd_dots_t i)
	{
		function.dots = i;
	}

	void set(display_entry_t i)
	{
		entry = i;
	}
	void set(display_control_t i)
	{
		control = i;
	}
	void set(display_function_t i)
	{
		control = i;
	}

	void set(lcd_backlight_t i)
	{
		backlight = i;
	}

	void update_entry()
	{
		com.write(LCD_SET_ENTRY | entry, backlight);
	}
	void update_control()
	{
		com.write(LCD_SET_CONTROL | control, backlight);
	}
	void update_function()
	{
		com.write(LCD_SET_FUNCTION | function, backlight);
	}
	void update_backlight()
	{
		com.write(LCD_DO_NOTHING, backlight);
	}

	void update_settings()
	{
		update_function();
		update_entry();
		update_control();
	}

	void write_cstr(const char *const cstr, int del_ms = 0, const lcd_str_flags_t &f = {})
	{
		const char *ptr = cstr;
		char c = '\0';
		while ((c = *ptr)) // automatically stops when null
		{
			switch (c)
			{
			case '\n':
				switch (f.nl_as)
				{
				case LCD_NL_IGNORE:
					break;
				case LCD_NL_AS_SP:
					write_char(' ');
					break;
				case LCD_NL_AS_LF:
					move_cursor(cursor_pos + lcd_dims_t(1, 0));
					break;
				case LCD_NL_AS_CRLF:
					move_cursor(lcd_dims_t(cursor_pos.y + 1, 0));
					break;
				}
				break;
			case '\t':
				do
				{
					write_char(' ');
				} while (cursor_pos.x % f.tb_to_sp);
				break;
			default:
				write_char(c);
				break;
			}

			++ptr;
			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}
	}
	void write_bffr(const char *const cstr, size_t len, int del_ms = 0)
	{
		const char *ptr = cstr;
		const char *ptrend = ptr + len;
		while (ptr < ptrend)
		{
			write_char(*ptr++);
			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}
	}
	void write_char(char c)
	{
		com.write(c, backlight, true);
		if (entry.entry == LCD_ENTRY_INCREMENT)
			increment_internal_cursor();
		else
			decrement_internal_cursor();
	}

	void write_cgram(size_t loc, const lcd_symbol_t &sym)
	{
		loc &= 0x7; // we only have 8 slots (or 4 with 5x10, then lowest bit is ignored)
		size_t len = (function.dots == LCD_5x10DOTS) ? 10 : 8;

		com.write(LCD_SET_CGR_ADDR | (loc << 3), backlight);

		for (size_t i = 0; i < len; ++i)
			com.write(sym[i], backlight, true);

		move_cursor(0, 0);
	}

	void clear()
	{
		com.write(LCD_CLEAR_DISPLAY, backlight);
		cursor_pos = {0, 0};
		entry.entry = LCD_ENTRY_INCREMENT; // Also sets I/D bit to 1 (increment mode)
	}
	void home()
	{
		com.write(LCD_CURSOR_HOME, backlight);
		vTaskDelay(pdMS_TO_TICKS(2)); // 1.52ms execution time
		cursor_pos = {0, 0};
	}

	void disp_shift_left()
	{
		com.write_byte(LCD_SET_MODE | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT, backlight);
	}
	void disp_shift_right()
	{
		com.write_byte(LCD_SET_MODE | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT, backlight);
	}

	lcd_dims_t get_cursor()
	{
		return cursor_pos;
	}

	void move_cursor(lcd_dims_t p)
	{
		constexpr uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54, 0x28, 0x68}; // 3rd line is a stretch, but it makes sense. 4th would not fit anymore

		cursor_pos = p % screen_size;

		uint8_t idx = row_offsets[cursor_pos.y] + cursor_pos.x;

		com.write(LCD_SET_DDR_ADDR | idx, backlight);
	}
	void move_cursor(uint8_t y, uint8_t x)
	{
		move_cursor(lcd_dims_t(y, x));
	}

private:
	void increment_internal_cursor()
	{
		cursor_pos.x++;

		if (cursor_pos.x >= screen_size.x)
		{
			cursor_pos.x = 0;
			cursor_pos.y++;
			if (cursor_pos.y >= screen_size.y)
				cursor_pos.y = 0;

			move_cursor(cursor_pos);
		}
		log_cursor();
	}
	void decrement_internal_cursor()
	{
		cursor_pos.x--;

		if (cursor_pos.x >= screen_size.x)
		{
			cursor_pos.x = screen_size.x - 1;
			cursor_pos.y--;
			if (cursor_pos.y >= screen_size.y)
				cursor_pos.y = screen_size.y - 1;

			move_cursor(cursor_pos);
		}
		log_cursor();
	}

	void log_cursor()
	{
		ESP_LOGD(TAG, "Cursor is at: %d, %d", cursor_pos.y, cursor_pos.x);
	}
};

#undef TAG