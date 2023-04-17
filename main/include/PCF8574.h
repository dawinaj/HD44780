#pragma once

#define TAG "PCF8574"

// Class for handling connection to the expander
class PCF8574
{
	i2c_port_t PORT;
	uint8_t HEAD;

public:
	PCF8574(i2c_port_t port, uint8_t address) : PORT(port), HEAD((address << 1) | I2C_MASTER_WRITE)
	{
	}
	~PCF8574() = default;

	// Interface for HD...
	esp_err_t setup_pins(bool RS, bool RW, bool CE, bool BL, uint8_t data)
	{
		uint8_t datah = (data & 0xF0) | mode | BL;

		(data << 4) & 0xF0

			esp_err_t ret;

		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(, data & 0xF0, mode),
			err, TAG, "Error with lcd_write_nibble()");

		ESP_GOTO_ON_ERROR(
			lcd_write_nibble(handle, (data << 4) & 0xF0, mode),
			err, TAG, "Error with lcd_write_nibble()");

		return ESP_OK;
	err:
		ESP_LOGE(TAG, "lcd_write_byte:%s", esp_err_to_name(ret));
		return ret;
	}

	static esp_err_t lcd_write_nibble(uint8_t nibble, uint8_t mode)
	{
		esp_err_t ret = ESP_OK;
		uint8_t data = 0;

		if (handle->backlight)
		{
			data = (nibble & 0xF0) | mode | LCD_BACKLIGHT_CONTROL_ON;
		}
		else
		{
			data = (nibble & 0xF0) | mode | LCD_BACKLIGHT_CONTROL_OFF;
		}

		ESP_GOTO_ON_ERROR(
			send_byte(data),
			err, TAG, "Error with lcd_i2c_write()");

		ets_delay_us(LCD_PRE_PULSE_DELAY_US); // Need a decent delay here, else display won't work

		// Clock the data into the LCD
		ESP_GOTO_ON_ERROR(
			lcd_pulse_enable(handle, data),
			err, TAG, "Error in lcd_pulse_enable()");

		return ESP_OK;

	err:
		ESP_LOGE(TAG, "lcd_write_nibble:%s", esp_err_to_name(ret));
		return ret;
	}

	esp_err_t send_byte(uint8_t byte)
	{
		esp_err_t ret = ESP_OK;
		i2c_cmd_handle_t cmdh = i2c_cmd_link_create();

		ESP_GOTO_ON_ERROR(
			i2c_master_start(cmdh),
			err, TAG, "Error with i2c_master_start()");

		ESP_GOTO_ON_ERROR(
			i2c_master_write_byte(cmdh, HEAD, true),
			err, TAG, "Error with i2c_master_write_byte() #1");

		ESP_GOTO_ON_ERROR(
			i2c_master_write_byte(cmdh, byte, true),
			err, TAG, "Error with i2c_master_write_byte() #2");

		ESP_GOTO_ON_ERROR(
			i2c_master_stop(cmdh),
			err, TAG, "Error with i2c_master_stop()");

		ESP_GOTO_ON_ERROR(
			i2c_master_cmd_begin(PORT, cmdh, pdMS_TO_TICKS(1000)),
			err, TAG, "Error with i2c_master_cmd_begin()");

		i2c_cmd_link_delete(cmdh);

		return ESP_OK;

	err:
		ESP_LOGE(TAG, "Error in ::send_byte(): %s", esp_err_to_name(ret));
		return ret;
	}
}

#undef TAG