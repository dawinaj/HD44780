#pragma once

template <typename Comm>
class HD44780
{
public:
	Comm &communicator;

	HD44780(Comm &c) : communicator(c)
	{
		esp_err_t ret = ESP_OK;

		// Initialise the LCD controller by instruction for 4-bit interface
		// First part of reset sequence
		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(handle, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_COMMAND),
			err, TAG, "Unable to complete Reset by Instruction. Part 1.");
		// 4.1 ms delay (min)
		vTaskDelay(pdMS_TO_TICKS(10));
		// second part of reset sequence
		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(handle, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_COMMAND),
			err, TAG, "Unable to complete Reset by Instruction. Part 2.");
		// 100 us delay (min)
		ets_delay_us(200);
		// Third time's a charm
		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(handle, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_COMMAND),
			err, TAG, "Unable to complete Reset by Instruction. Part 3.");
		// Activate 4-bit mode
		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(handle, LCD_FUNCTION_SET | LCD_4BIT_MODE, LCD_COMMAND),
			err, TAG, "Unable to activate 4-bit mode.");
		// 40 us delay (min)
		ets_delay_us(80);

		// --- Busy flag now available ---
		// Set Display Function: # line, font size, etc.
		// 37us max execution time with 270kHz clock
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