#pragma once

#include "HD44780_defines.h"

struct lcd_config_t
{
	uint8_t columns;		  /*!< Number of columns. Must be populated prior to calling lcd_init(). */
	uint8_t rows;			  /*!< Number of rows. Must be populated prior to calling lcd_init(). */
	uint8_t display_function; /*!< Current state of display function flag. Must be populated prior to calling lcd_init(). */
	uint8_t display_control;  /*!< Current state of display control flag. Must be populated prior to calling lcd_init(). */
	uint8_t display_mode;	  /*!< Current state of display mode flag. Must be populated prior to calling lcd_init(). */
	uint8_t backlight;		  /*!< Current state of backlight. */
};

template <typename Comm>
class HD44780
{
public:
	Comm &com;

	display_control_t control;
	display_mode_t mode;
	display_function_t function;

	HD44780(Comm &c) : com(c)
	{

		com.init();
	}
	~HD44780() {}

	//

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

	void set(lcd_move_t i)
	{
		mode.move = i;
	}
	void set(lcd_dir_t i)
	{
		mode.direction = i;
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

	void set(display_control_t i)
	{
		control = i;
	}
	void set(display_mode_t i)
	{
		mode = i;
	}
	void set(display_function_t i)
	{
		control = i;
	}

	void update_control()
	{
		com.write(0, 0, )
	}
	void update_mode()
	{
	}
	void update_function()
	{
	}
	void update_backlight()
	{
	}
}