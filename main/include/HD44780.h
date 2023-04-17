#pragma once

struct lcd_config_t
{
	i2c_port_t i2c_port;	  /*!< I2C controller used. Must be populated prior to calling lcd_init(). */
	uint8_t address;		  /*!< Address of the LCD on the I2C bus. Must be populated prior to calling lcd_init(). */
	uint8_t columns;		  /*!< Number of columns. Must be populated prior to calling lcd_init(). */
	uint8_t rows;			  /*!< Number of rows. Must be populated prior to calling lcd_init(). */
	uint8_t display_function; /*!< Current state of display function flag. Must be populated prior to calling lcd_init(). */
	uint8_t display_control;  /*!< Current state of display control flag. Must be populated prior to calling lcd_init(). */
	uint8_t display_mode;	  /*!< Current state of display mode flag. Must be populated prior to calling lcd_init(). */
	uint8_t cursor_column;	  /*!< Current column position of cursor. First column is position 0. */
	uint8_t cursor_row;		  /*!< Current row position of cursor. First row is position 0. */
	uint8_t backlight;		  /*!< Current state of backlight. */
	bool initialized;		  /*!< Private flag to reflect initialization state. */
};

enum lcd_control_t
{
	LCD_DISPLAY_ON = 0x04,	/*!< Display control bitmask for Display On */
	LCD_DISPLAY_OFF = 0x00, /*!< Display control bitmask for Display Off */
	LCD_CURSOR_ON = 0x02,	/*!< Display control bitmask for Cursor On */
	LCD_CURSOR_OFF = 0x00,	/*!< Display control bitmask for Cursor Off */
	LCD_BLINK_ON = 0x01,	/*!< Display control bitmask for Cursor Blink On */
	LCD_BLINK_OFF = 0x00	/*!< Display control bitmask for Cursor Blink Off */
}

enum lcd_cursor_t
{
	LCD_DISPLAY_MOVE = 0x08, /*!< Cursor/Display Shift bitmask for Display Shift */
	LCD_CURSOR_MOVE = 0x00,	 /*!< Cursor/Display Shift bitmask for Cursor Move */
	LCD_MOVE_RIGHT = 0x04,	 /*!< Cursor/Display Shift bitmask for Shift Right */
	LCD_MOVE_LEFT = 0x00,	 /*!< Cursor/Display Shift bitmask for Shift Left */

}

enum lcd_mode_t
{
	LCD_MODE_8BIT = 0x10, // 1 << 4
	LCD_MODE_4BIT = 0x00
}

enum lcd_lines_t
{
	LCD_2LINE = 0x08, // 1 << 3
	LCD_1LINE = 0x00
}

enum lcd_lines_t
{
	LCD_5x10DOTS = 0x04, // 1 << 2
	LCD_5x8DOTS = 0x00
}

#define LCD_DEFAULT_CONFIG()                                                \
	{                                                                       \
		.i2c_port = I2C_MASTER_NUM,                                         \
		.address = LCD_ADDR,                                                \
		.columns = LCD_COLUMNS,                                             \
		.rows = LCD_ROWS,                                                   \
		.display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS,        \
		.display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF, \
		.display_mode = LCD_ENTRY_INCREMENT | LCD_ENTRY_DISPLAY_NO_SHIFT,   \
		.cursor_column = 0,                                                 \
		.cursor_row = 0,                                                    \
		.backlight = LCD_BACKLIGHT,                                         \
		.initialized = false,                                               \
	}

template <typename Comm>
class HD44780
{
public:
	Comm &communicator;

	HD44780(Comm &c) : communicator(c)
	{

		communicator.init();

		// --- Busy flag now available ---
		// Set Display Function: # line, font size, etc.
		// Set mode, lines, and font
		ESP_GOTO_ON_ERROR(
			lcd_write_byte(handle, LCD_FUNCTION_SET | handle->display_function, LCD_COMMAND),
			err, TAG, "Unable to perform Set Display Function instruction.");
		ets_delay_us(LCD_STD_EXEC_TIME_US);

		// turn the display on with no cursor or blinking default
		ESP_GOTO_ON_ERROR(
			lcd_display(handle),
			err, TAG, "Error with lcd_display()");

		// Clear Display instruction
		ESP_GOTO_ON_ERROR(
			lcd_clear_screen(handle),
			err, TAG, "Error with lcd_clear_screen()");

		// Entry Mode Set instruction.
		// Sets cursor move direction and specifies display shift
		// 37us max execution time with 270kHz clock
		ESP_GOTO_ON_ERROR(
			lcd_write_byte(handle, LCD_ENTRY_MODE_SET | handle->display_mode, LCD_COMMAND),
			err, TAG, "Unable to perform Entry Mode Set instruction.");
		ets_delay_us(LCD_STD_EXEC_TIME_US);

		ESP_GOTO_ON_ERROR(
			lcd_home(handle),
			err, TAG, "Error with lcd_home()");
		handle->initialized = true;

		return ret;
	err:
		if (ret == ESP_ERR_INVALID_STATE)
		{
			ESP_LOGE(TAG, "I2C driver must be installed before attempting to initalize LCD.");
		}
		return ret;
	}
	~HD44780() {}
}