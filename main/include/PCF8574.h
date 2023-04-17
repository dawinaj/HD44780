#pragma once

#include <driver/i2c.h>

#include "HD44780_defines.h"

#define TAG "PCF8574"

// Class for handling connection to the expander
class PCF8574
{
	i2c_port_t PORT;
	uint8_t HEAD;

public:
	PCF8574(i2c_port_t port, uint8_t address = 0x3F) : PORT(port), HEAD((address << 1) | I2C_MASTER_WRITE)
	{
	}
	~PCF8574() = default;

	// Interfaces for HD... init, pins

	// Initialise the LCD controller by instruction for 4-bit interface (obviously)
	void init()
	{
		// First part of reset sequence
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT, 0);
		vTaskDelay(pdMS_TO_TICKS(10)); // 4.1 ms delay (min)

		// second part of reset sequence
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT, 0);
		ets_delay_us(200); // 100 us delay (min)

		// Third time's a charm
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT, 0);

		// Activate 4-bit mode
		write_nibble(LCD_SET_FUNCTION | LCD_4BIT, 0);
		ets_delay_us(80); // 40 us delay (min)
	}

	void write(bool RS, bool BL, uint8_t data)
	{
		uint8_t mode = (RS << 0) | (BL << 3);

		uint8_t datah = data & 0xF0;
		uint8_t datal = (data << 4) & 0xF0;

		write_nibble(datah, mode);
		write_nibble(datal, mode);
	}

private:
	// writes and clocks-in the data
	void write_nibble(uint8_t data, uint8_t mode)
	{
		send_i2c(data, mode, false);
		ets_delay_us(1000); // to stabilize pins (why so long gdammit)

		send_i2c(data, mode, true);
		ets_delay_us(1); // enable pulse must be >450ns

		send_i2c(data, mode, false);
		ets_delay_us(42); // 37us + 4us execution time
	}

	void send_i2c(uint8_t data, uint8_t mode, bool CE)
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
		return;

	err:
		ESP_LOGE(TAG, "Error in ::send_byte(): %s", esp_err_to_name(ret));
	}
}

#undef TAG