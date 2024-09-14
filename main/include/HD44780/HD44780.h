#ifndef HD44780_H
#define HD44780_H

#include <array>
#include <type_traits>

#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include <esp_check.h>

#include <rom/ets_sys.h>

//

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

//

struct lcd_symbol_t : public std::array<uint8_t, 10>
{
	void set(uint8_t y, uint8_t x)
	{
		assert(x < 5);
		at(y) |= (0b00010000 >> x);
	}
	void unset(uint8_t y, uint8_t x)
	{
		at(y) &= ~(0b00010000 >> x);
	}
};

struct lcd_dims_t
{
	uint8_t y;
	uint8_t x;

	lcd_dims_t &operator+=(const lcd_dims_t &r)
	{
		y += r.y;
		x += r.x;
		return *this;
	}
	lcd_dims_t &operator-=(const lcd_dims_t &r)
	{
		y -= r.y;
		x -= r.x;
		return *this;
	}
	lcd_dims_t &operator*=(const lcd_dims_t &r)
	{
		y *= r.y;
		x *= r.x;
		return *this;
	}
	lcd_dims_t &operator/=(const lcd_dims_t &r)
	{
		y /= r.y;
		x /= r.x;
		return *this;
	}
	lcd_dims_t &operator%=(const lcd_dims_t &r)
	{
		y %= r.y;
		x %= r.x;
		return *this;
	}
};

lcd_dims_t operator+(lcd_dims_t l, const lcd_dims_t &r)
{
	return l += r;
}
lcd_dims_t operator-(lcd_dims_t l, const lcd_dims_t &r)
{
	return l -= r;
}
lcd_dims_t operator*(lcd_dims_t l, const lcd_dims_t &r)
{
	return l *= r;
}
lcd_dims_t operator/(lcd_dims_t l, const lcd_dims_t &r)
{
	return l /= r;
}
lcd_dims_t operator%(lcd_dims_t l, const lcd_dims_t &r)
{
	return l %= r;
}

//

// display entry {
enum lcd_entry_t : uint8_t
{
	LCD_ENTRY_DECREMENT = 0b00000000, // Decrements DDRAM and shifts cursor left
	LCD_ENTRY_INCREMENT = 0b00000010, // Increments DDRAM and shifts cursor right
};
enum lcd_shift_t : uint8_t
{
	LCD_DISPLAY_NOSHIFT = 0b00000000, // Display does not shift
	LCD_DISPLAY_SHIFT = 0b00000001,	  // Shifts entire display. Right if decrement. Left if increment
};
// }

// display control {
enum lcd_display_t : uint8_t
{
	LCD_DISPLAY_OFF = 0b00000000, // Display OFF
	LCD_DISPLAY_ON = 0b00000100,  // Display ON
};
enum lcd_cursor_t : uint8_t
{
	LCD_CURSOR_OFF = 0b00000000, // Cursor OFF
	LCD_CURSOR_ON = 0b00000010,	 // Cursor ON
};
enum lcd_blink_t : uint8_t
{
	LCD_BLINK_OFF = 0b00000000, // Blink OFF
	LCD_BLINK_ON = 0b00000001,	// Blink ON
};
// }

// display function {
enum lcd_mode_t : uint8_t
{
	LCD_4BIT = 0b00000000,
	LCD_8BIT = 0b00010000, // 1 << 4
};
enum lcd_lines_t : uint8_t
{
	LCD_1LINE = 0b00000000,
	LCD_2LINE = 0b00001000, // 1 << 3
};
enum lcd_dots_t : uint8_t
{
	LCD_5x8DOTS = 0b00000000,
	LCD_5x10DOTS = 0b00000100, // 1 << 2
};
// }

//

class HD44780_Iface
{
public:
	HD44780_Iface() = default;
	virtual ~HD44780_Iface() = default;
	//
	virtual esp_err_t write_wide(uint8_t) = 0;
	virtual esp_err_t write(uint8_t, bool = false) = 0;
	//
	virtual esp_err_t read(uint8_t &, bool = false) = 0;
	virtual esp_err_t check_bf(bool &) = 0;
	//
	virtual esp_err_t backlight(bool) = 0;
	//
	virtual lcd_mode_t bit_mode() const = 0;
	virtual bool can_read() const = 0;
	virtual uint32_t bf_us() const = 0;
};

//

class HD44780
{
	static constexpr const char *const TAG = "HD44780";
	static constexpr uint32_t base_freq = 270000;

	std::add_const_t<HD44780_Iface *> iface;
	const lcd_dims_t screen_size;
	const lcd_lines_t screen_lines;
	uint32_t freq;

	lcd_dots_t character_dots;
	lcd_entry_t entry_mode;

	lcd_dims_t cursor_pos = {0, 0};
	uint32_t bf_us = 0;
	bool can_read = false;

public:
	HD44780(HD44780_Iface *i, lcd_dims_t sz, lcd_lines_t l = LCD_2LINE, uint32_t f = 270000) : iface(i), screen_size(sz), screen_lines(l), freq(f) {}
	~HD44780() {}

	esp_err_t init(lcd_dots_t d = LCD_5x8DOTS)
	{
		character_dots = d;
		bf_us = iface->bf_us();
		can_read = iface->can_read();

		// Reset sequence (Doc Figs 23 & 24)
		ESP_RETURN_ON_ERROR(
			write_init(LCD_SET_FUNCTION, LCD_8BIT, 4100 * 2), // 4.1ms delay (min)
			TAG, "Failed to write_init!");

		ESP_RETURN_ON_ERROR(
			write_init(LCD_SET_FUNCTION, LCD_8BIT, 100 * 2), // 100us delay (min)
			TAG, "Failed to write_init!");

		ESP_RETURN_ON_ERROR(
			write_init(LCD_SET_FUNCTION, LCD_8BIT),
			TAG, "Failed to write_init!");

		ESP_LOGW(TAG, "Init0 done");

		// initialized, we can now set interface length if required
		if (iface->bit_mode() == LCD_4BIT)
		{
			ESP_RETURN_ON_ERROR(
				write_init(LCD_SET_FUNCTION, LCD_4BIT),
				TAG, "Failed to write_init!");

			ESP_LOGW(TAG, "4bit0 done");
		}

		// and apply init one last time
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_FUNCTION, static_cast<uint8_t>(iface->bit_mode()) | static_cast<uint8_t>(screen_lines) | static_cast<uint8_t>(character_dots)),
			TAG, "Failed to write_cmd!");

		ESP_LOGW(TAG, "function done");

		// default settings

		ESP_RETURN_ON_ERROR(
			set_control(),
			TAG, "Failed to set_control!");

		ESP_LOGW(TAG, "ctrl done");

		ESP_RETURN_ON_ERROR(
			set_entry(),
			TAG, "Failed to set_entry!");

		ESP_LOGW(TAG, "entry done");

		ESP_RETURN_ON_ERROR(
			clear(),
			TAG, "Failed to clear!");

		ESP_LOGW(TAG, "clear done");

		return ESP_OK;
	}

	esp_err_t deinit()
	{
		ESP_RETURN_ON_ERROR(
			clear(),
			TAG, "Failed to clear!");

		return ESP_OK;
	}

	//

	esp_err_t set_entry(lcd_entry_t e = LCD_ENTRY_INCREMENT, lcd_shift_t s = LCD_DISPLAY_NOSHIFT)
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_ENTRY, static_cast<uint8_t>(e) | static_cast<uint8_t>(s)),
			TAG, "Failed to write_cmd!");

		entry_mode = e;

		return ESP_OK;
	}
	esp_err_t set_control(lcd_display_t d = LCD_DISPLAY_ON, lcd_cursor_t c = LCD_CURSOR_OFF, lcd_blink_t b = LCD_BLINK_OFF)
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_CONTROL, static_cast<uint8_t>(d) | static_cast<uint8_t>(c) | static_cast<uint8_t>(b)),
			TAG, "Failed to write_cmd!");

		return ESP_OK;
	}

	esp_err_t backlight(bool b)
	{
		ESP_RETURN_ON_ERROR(
			iface->backlight(b),
			TAG, "Failed to iface.backlight!");

		return ESP_OK;
	}

	esp_err_t write_cstr(const char *cstr, TickType_t del_ms = 0, const lcd_str_flags_t &f = {})
	{
		char c = '\0';
		while ((c = *cstr)) // automatically stops when null
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
						TAG, "Failed to write_char!");
					break;
				case LCD_NL_AS_LF:
					ESP_RETURN_ON_ERROR(
						move_cursor(cursor_pos + lcd_dims_t(1, 0)),
						TAG, "Failed to move_cursor!");
					break;
				case LCD_NL_AS_CRLF:
					ESP_RETURN_ON_ERROR(
						move_cursor(lcd_dims_t(cursor_pos.y + 1, 0)),
						TAG, "Failed to move_cursor!");
					break;
				}
				break;
			case '\t':
				do
				{
					ESP_RETURN_ON_ERROR(
						write_char(' '),
						TAG, "Failed to write_char!");
				} // f off
				while (cursor_pos.x % f.tb_to_sp);
				break;
			default:
				ESP_RETURN_ON_ERROR(
					write_char(c),
					TAG, "Failed to write_char!");
				break;
			}

			++cstr;
			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}

		return ESP_OK;
	}
	esp_err_t write_bffr(const uint8_t *bffr, size_t len, TickType_t del_ms = 0)
	{
		const uint8_t *bffrend = bffr + len;
		while (bffr < bffrend)
		{
			ESP_RETURN_ON_ERROR(
				write_char(*bffr++),
				TAG, "Failed to write_char!");

			if (del_ms)
				vTaskDelay(pdMS_TO_TICKS(del_ms));
		}

		return ESP_OK;
	}

	esp_err_t write_char(uint8_t c)
	{
		ESP_RETURN_ON_ERROR(
			write_data(c),
			TAG, "Failed to write_data!");

		if (entry_mode == LCD_ENTRY_INCREMENT)
			ESP_RETURN_ON_ERROR(
				increment_internal_cursor(),
				TAG, "Failed to increment_internal_cursor!");
		else
			ESP_RETURN_ON_ERROR(
				decrement_internal_cursor(),
				TAG, "Failed to decrement_internal_cursor!");

		return ESP_OK;
	}

	esp_err_t write_cgram(size_t loc, const lcd_symbol_t &sym)
	{
		loc &= 0x7; // we only have 8 slots (or 4 with 5x10, then lowest bit is ignored)
		size_t len = (character_dots == LCD_5x10DOTS) ? 10 : 8;

		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_CGR_ADDR, (loc << 3)), // lowest 3 bits auto-incremented
			TAG, "Failed to write_cmd!");

		for (size_t i = 0; i < len; ++i)
			ESP_RETURN_ON_ERROR(
				write_data(sym[i]),
				TAG, "Failed to write_data!");

		ESP_RETURN_ON_ERROR(
			move_cursor(cursor_pos),
			TAG, "Failed to move_cursor!");

		return ESP_OK;
	}

	esp_err_t clear()
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_CLEAR_DISPLAY, 0x00, 1520),
			TAG, "Failed to write_cmd!");

		cursor_pos = {0, 0};
		entry_mode = LCD_ENTRY_INCREMENT; // Also sets I/D bit to 1 (increment mode)

		return ESP_OK;
	}
	esp_err_t home()
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_CURSOR_HOME, 0x00, 1520),
			TAG, "Failed to write_cmd!");

		cursor_pos = {0, 0};

		return ESP_OK;
	}

	esp_err_t disp_shift_left()
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_MODE, static_cast<uint8_t>(LCD_DISPLAY_MOVE) | static_cast<uint8_t>(LCD_MOVE_LEFT)),
			TAG, "Failed to write_cmd!");
		return ESP_OK;
	}
	esp_err_t disp_shift_right()
	{
		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_MODE, static_cast<uint8_t>(LCD_DISPLAY_MOVE) | static_cast<uint8_t>(LCD_MOVE_RIGHT)),
			TAG, "Failed to write_cmd!");
		return ESP_OK;
	}

	lcd_dims_t get_cursor()
	{
		return cursor_pos;
	}

	esp_err_t move_cursor(const lcd_dims_t &p)
	{
		constexpr uint8_t row_addr[] = {0, 64 + 0, 20, 64 + 20};

		cursor_pos = p % screen_size;

		uint8_t idx = row_addr[cursor_pos.y] + cursor_pos.x;
		idx &= 0b01111111;

		ESP_RETURN_ON_ERROR(
			write_cmd(LCD_SET_DDR_ADDR, idx),
			TAG, "Failed to write_cmd!");

		return ESP_OK;
	}
	esp_err_t move_cursor(uint8_t y, uint8_t x)
	{
		return move_cursor(lcd_dims_t(y, x));
	}

	//

private:
	esp_err_t write_init(uint8_t cmd, uint8_t data = 0x00, uint32_t dly = 37)
	{
		assert(cmd > data);

		ESP_RETURN_ON_ERROR(
			iface->write_wide(cmd | data),
			TAG, "Failed to iface.write!");

		ESP_RETURN_ON_ERROR(
			delay_or_bf(dly, true),
			TAG, "Failed to delay_or_bf!");

		return ESP_OK;
	}
	/*/esp_err_t write_init(uint8_t cmd, uint8_t data = 0x0)
	{
		assert(cmd > data);

		ESP_RETURN_ON_ERROR(
			iface->write(cmd | data),
			TAG, "Failed to iface.write!");

		ets_delay_us(80); // 37us standard execution time at 270kHz, min frequency with min voltage is 125kHz so double the time

		return ESP_OK;
	}//*/

	esp_err_t write_cmd(uint8_t cmd, uint8_t data = 0x00, uint32_t dly = 37)
	{
		assert(cmd > data);

		ESP_RETURN_ON_ERROR(
			iface->write(cmd | data, false),
			TAG, "Failed to iface.write!");

		ESP_RETURN_ON_ERROR(
			delay_or_bf(dly),
			TAG, "Failed to delay_or_bf!");

		return ESP_OK;
	}
	esp_err_t write_data(uint8_t data)
	{
		ESP_RETURN_ON_ERROR(
			iface->write(data, true),
			TAG, "Failed to iface.write!");

		ESP_RETURN_ON_ERROR(
			delay_or_bf(37),
			TAG, "Failed to delay_or_bf!");

		ESP_RETURN_ON_ERROR(
			delay_or_bf(4, true),
			TAG, "Failed to delay_or_bf!");

		return ESP_OK;
	}

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
				TAG, "Failed to move_cursor!");
		}
		// log_cursor();

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
				TAG, "Failed to move_cursor!");
		}
		// log_cursor();

		return ESP_OK;
	}

	void log_cursor()
	{
		ESP_LOGD(TAG, "Cursor is at: %d, %d", cursor_pos.y, cursor_pos.x);
	}

private:
	esp_err_t delay_or_bf(uint32_t delay, bool force = false)
	{
		const uint32_t us = delay * base_freq / freq;

		if (force || !can_read || bf_us > us)
		{
			const TickType_t tck = pdMS_TO_TICKS(us / 1000);
			if (tck) // > 0
				vTaskDelay(tck + 1);
			else
				ets_delay_us(us + 1);
		}
		else
		{
			bool bf;
			do
			{
				ESP_RETURN_ON_ERROR(
					iface->check_bf(bf),
					TAG, "Failed to iface.check_bf!");
			} //
			while (bf);
		}

		return ESP_OK;
	}

private:
	enum lcd_command_t : uint8_t
	{
		LCD_DO_NOTHING /*   */ = 0b00000000,
		LCD_CLEAR_DISPLAY /**/ = 0b00000001,
		LCD_CURSOR_HOME /*  */ = 0b00000010,
		LCD_SET_ENTRY /*    */ = 0b00000100,
		LCD_SET_CONTROL /*  */ = 0b00001000,
		LCD_SET_MODE /*     */ = 0b00010000,
		LCD_SET_FUNCTION /* */ = 0b00100000,
		LCD_SET_CGR_ADDR /* */ = 0b01000000,
		LCD_SET_DDR_ADDR /* */ = 0b10000000,
	};

	// display shift {
	enum lcd_move_t : uint8_t
	{
		LCD_CURSOR_MOVE = 0b00000000,  // Move Cursor
		LCD_DISPLAY_MOVE = 0b00001000, // Move Display
	};
	enum lcd_dir_t : uint8_t
	{
		LCD_MOVE_LEFT = 0b00000000,	 // Shift Left
		LCD_MOVE_RIGHT = 0b00000100, // Shift Right
	};
	// }
};

#endif