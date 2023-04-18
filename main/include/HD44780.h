#pragma once

#include "HD44780_defines.h"

#define TAG "HD44780"

template <typename Comm>
class HD44780
{
public:
	Comm &com;

	display_entry_t entry;
	display_control_t control;
	// display_mode_t mode;
	display_function_t function;
	bool backlight;

	const lcd_dims_t screen_size;

private:
	lcd_dims_t cursor_pos;

public:
	HD44780(Comm &c, lcd_dims_t d) : com(c), entry(), control(), function(), backlight(), screen_size(d), cursor_pos(0, 0)
	{
		function.mode = com.init();
		function.lines = LCD_2LINE;
		function.dots = LCD_5x8DOTS;
		update_function();

		control.display = LCD_DISPLAY_ON;
		control.cursor = LCD_CURSOR_OFF;
		control.blink = LCD_BLINK_OFF;
		update_control();

		clear();

		entry.entry = LCD_ENTRY_INCREMENT;
		entry.shift = LCD_DISPLAY_NOSHIFT;
		update_entry();

		backlight = true;
		home();
	}
	HD44780(Comm &c, uint8_t h, uint8_t w) : HD44780(c, lcd_dims_t(h, w)) {}
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

	// void set(lcd_move_t i)
	// {
	// 	mode.move = i;
	// }
	// void set(lcd_dir_t i)
	// {
	// 	mode.direction = i;
	// }

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
	// void set(display_mode_t i)
	// {
	// 	mode = i;
	// }
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
	// void update_mode()
	// {
	// 	com.write(LCD_SET_MODE | mode, backlight);
	// }
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
		update_entry();
		update_control();
		// update_mode();
		update_function();
	}

	void write_string(const char *const str, int del_ms)
	{
		const char *ptr = str;
		while (*ptr) // automatically stops when null
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

	void move_cursor(lcd_dims_t p)
	{
		constexpr uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54, 0x28, 0x68}; // 3rd line is a stretch, but it makes sense. 4th would not fit.

		cursor_pos = p;
		if (p.x >= screen_size.x || p.y >= screen_size.y)
			return;

		uint8_t idx = row_offsets[p.y] + p.x;

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
		ESP_LOGI(TAG, "Cursor is at: %d, %d", cursor_pos.y, cursor_pos.x);
	}
};

#undef TAG