#pragma once

#include <esp_check.h>
#include <rom/ets_sys.h>

#include "HD44780/commands.h"
#include "HD44780/dimensions.h"
#include "HD44780/symbol.h"
#include "HD44780/str_flags.h"

#define TAG "HD44780"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

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

		esp_err_t ret = ESP_OK;

		// Reset sequence (Doc Figs 23 & 24)
		ESP_GOTO_ON_ERROR(
			com.write_init(LCD_SET_FUNCTION | LCD_8BIT),
			err, TAG, "Error in ::HD44780() with com.write_init() #1");

		vTaskDelay(pdMS_TO_TICKS(8)); // 4.1 ms delay (min)

		ESP_GOTO_ON_ERROR(
			com.write_init(LCD_SET_FUNCTION | LCD_8BIT),
			err, TAG, "Error in ::HD44780() with com.write_init() #2");

		ets_delay_us(200); // 100 us delay (min)

		ESP_GOTO_ON_ERROR(
			com.write_init(LCD_SET_FUNCTION | LCD_8BIT),
			err, TAG, "Error in ::HD44780() with com.write_init() #3");

		if (function.mode == LCD_4BIT) // Activate 4-bit mode
		{
			ESP_GOTO_ON_ERROR(
				com.write_init(LCD_SET_FUNCTION | LCD_4BIT),
				err, TAG, "Error in ::HD44780() with com.write_init() #4");
		}

		ESP_GOTO_ON_ERROR(
			update_function(),
			err, TAG, "Error in ::HD44780() with update_function()");
		ESP_GOTO_ON_ERROR(
			update_settings(),
			err, TAG, "Error in ::HD44780() with update_settings()");
		ESP_GOTO_ON_ERROR(
			clear(),
			err, TAG, "Error in ::HD44780() with clear()");

		return;

	err:
		ESP_LOGE(TAG, "Failed to construct! %s", esp_err_to_name(ret));
	}

	HD44780(Comm &c, uint8_t h, uint8_t w, lcd_lines_t l = LCD_2LINE, lcd_dots_t d = LCD_5x8DOTS) : HD44780(c, lcd_dims_t(h, w), l, d) {}

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

	esp_err_t update_entry()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_SET_ENTRY | entry, backlight),
			TAG, "Error in ::update_entry() with com.write()");

		return ESP_OK;
	}
	esp_err_t update_control()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_SET_CONTROL | control, backlight),
			TAG, "Error in ::update_control() with com.write()");

		return ESP_OK;
	}
	esp_err_t update_function()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_SET_FUNCTION | function, backlight),
			TAG, "Error in ::update_function() with com.write()");

		return ESP_OK;
	}
	esp_err_t update_backlight()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_DO_NOTHING, backlight),
			TAG, "Error in ::update_backlight() with com.write()");

		return ESP_OK;
	}

	esp_err_t update_settings()
	{
		ESP_RETURN_ON_ERROR(
			update_entry(),
			TAG, "Error in ::update_settings() with update_entry()");

		ESP_RETURN_ON_ERROR(
			update_control(),
			TAG, "Error in ::update_settings() with update_control()");

		return ESP_OK;
	}

	esp_err_t write_cstr(const char *const cstr, int del_ms = 0, const lcd_str_flags_t &f = {})
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
					ESP_RETURN_ON_ERROR(
						write_char(' '),
						TAG, "Error in ::write_cstr() with write_char() #1");
					break;
				case LCD_NL_AS_LF:
					ESP_RETURN_ON_ERROR(
						move_cursor(cursor_pos + lcd_dims_t(1, 0)),
						TAG, "Error in ::write_cstr() with move_cursor() #1");
					break;
				case LCD_NL_AS_CRLF:
					ESP_RETURN_ON_ERROR(
						move_cursor(lcd_dims_t(cursor_pos.y + 1, 0)),
						TAG, "Error in ::write_cstr() with move_cursor() #2");
					break;
				}
				break;
			case '\t':
				do
				{
					ESP_RETURN_ON_ERROR(
						write_char(' '),
						TAG, "Error in ::write_cstr() with write_char() #2");
				} // f off
				while (cursor_pos.x % f.tb_to_sp);
				break;
			default:
				ESP_RETURN_ON_ERROR(
					write_char(c),
					TAG, "Error in ::write_cstr() with write_char() #3");
				break;
			}

			++ptr;
			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}

		return ESP_OK;
	}
	esp_err_t write_bffr(const char *const cstr, size_t len, int del_ms = 0)
	{
		const char *ptr = cstr;
		const char *ptrend = ptr + len;
		while (ptr < ptrend)
		{
			ESP_RETURN_ON_ERROR(
				write_char(*ptr++),
				TAG, "Error in ::write_bffr() with write_char()");

			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}

		return ESP_OK;
	}
	esp_err_t write_char(char c)
	{
		ESP_RETURN_ON_ERROR(
			com.write(c, backlight, true),
			TAG, "Error in ::write_char() with com.write()");

		if (entry.entry == LCD_ENTRY_INCREMENT)
			ESP_RETURN_ON_ERROR(
				increment_internal_cursor(),
				TAG, "Error in ::write_char() with increment_internal_cursor()");
		else
			ESP_RETURN_ON_ERROR(
				decrement_internal_cursor(),
				TAG, "Error in ::write_char() with decrement_internal_cursor()");

		return ESP_OK;
	}

	esp_err_t write_cgram(size_t loc, const lcd_symbol_t &sym)
	{
		loc &= 0x7; // we only have 8 slots (or 4 with 5x10, then lowest bit is ignored)
		size_t len = (function.dots == LCD_5x10DOTS) ? 10 : 8;

		ESP_RETURN_ON_ERROR(
			com.write(LCD_SET_CGR_ADDR | (loc << 3), backlight),
			TAG, "Error in ::write_cgram() with com.write() #1");

		for (size_t i = 0; i < len; ++i)
			ESP_RETURN_ON_ERROR(
				com.write(sym[i], backlight, true),
				TAG, "Error in ::write_cgram() with com.write() #2");

		ESP_RETURN_ON_ERROR(
			move_cursor(0, 0),
			TAG, "Error in ::write_cgram() with move_cursor()");

		return ESP_OK;
	}

	esp_err_t clear()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_CLEAR_DISPLAY, backlight),
			TAG, "Error in ::clear() with com.write()");

		cursor_pos = {0, 0};
		entry.entry = LCD_ENTRY_INCREMENT; // Also sets I/D bit to 1 (increment mode)

		return ESP_OK;
	}
	esp_err_t home()
	{
		ESP_RETURN_ON_ERROR(
			com.write(LCD_CURSOR_HOME, backlight),
			TAG, "Error in ::home() with com.write()");

		vTaskDelay(pdMS_TO_TICKS(2)); // 1.52ms execution time
		cursor_pos = {0, 0};

		return ESP_OK;
	}

	esp_err_t disp_shift_left()
	{
		ESP_RETURN_ON_ERROR(
			com.write_byte(LCD_SET_MODE | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT, backlight),
			TAG, "Error in ::disp_shift_left() with com.write()");

		return ESP_OK;
	}
	esp_err_t disp_shift_right()
	{
		ESP_RETURN_ON_ERROR(
			com.write_byte(LCD_SET_MODE | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT, backlight),
			TAG, "Error in ::disp_shift_right() with com.write()");

		return ESP_OK;
	}

	lcd_dims_t get_cursor()
	{
		return cursor_pos;
	}

	esp_err_t move_cursor(lcd_dims_t p)
	{
		constexpr uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54, 0x28, 0x68}; // 3rd line is a stretch, but it makes sense. 4th would not fit anymore

		cursor_pos = p % screen_size;

		uint8_t idx = row_offsets[cursor_pos.y] + cursor_pos.x;

		ESP_RETURN_ON_ERROR(
			com.write(LCD_SET_DDR_ADDR | idx, backlight),
			TAG, "Error in ::move_cursor() with com.write()");

		return ESP_OK;
	}
	esp_err_t move_cursor(uint8_t y, uint8_t x)
	{
		return move_cursor(lcd_dims_t(y, x));
	}

private:
	esp_err_t increment_internal_cursor()
	{
		cursor_pos.x++;

		if (cursor_pos.x >= screen_size.x)
		{
			cursor_pos.x = 0;
			cursor_pos.y++;
			if (cursor_pos.y >= screen_size.y)
				cursor_pos.y = 0;

			ESP_RETURN_ON_ERROR(
				move_cursor(cursor_pos),
				TAG, "Error in ::increment_internal_cursor() with move_cursor()");
		}
		log_cursor();

		return ESP_OK;
	}
	esp_err_t decrement_internal_cursor()
	{
		cursor_pos.x--;

		if (cursor_pos.x >= screen_size.x)
		{
			cursor_pos.x = screen_size.x - 1;
			cursor_pos.y--;
			if (cursor_pos.y >= screen_size.y)
				cursor_pos.y = screen_size.y - 1;

			ESP_RETURN_ON_ERROR(
				move_cursor(cursor_pos),
				TAG, "Error in ::decrement_internal_cursor() with move_cursor()");
		}
		log_cursor();

		return ESP_OK;
	}

	void log_cursor()
	{
		ESP_LOGD(TAG, "Cursor is at: %d, %d", cursor_pos.y, cursor_pos.x);
	}
};

#pragma GCC diagnostic pop

#undef TAG