#pragma once

#include <esp_check.h>

#include <driver/i2c.h>
#include <rom/ets_sys.h>

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
	lcd_mode_t init()
	{
		// First part of reset sequence
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT);
		vTaskDelay(pdMS_TO_TICKS(10)); // 4.1 ms delay (min)

		// second part of reset sequence
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT);
		ets_delay_us(200); // 100 us delay (min)

		// Third time's a charm
		write_nibble(LCD_SET_FUNCTION | LCD_8BIT);

		// Activate 4-bit mode
		write_nibble(LCD_SET_FUNCTION | LCD_4BIT);
		ets_delay_us(80); // 40 us delay (min)

		return LCD_4BIT;
	}

	void write(uint8_t data, bool BL, bool RS = false)
	{
		uint8_t mode = (RS ? LCD_REG_PIN_DATA : LCD_REG_PIN_CMND) | (BL ? LCD_BL_PIN_ON : LCD_BL_PIN_OFF);

		uint8_t datah = data & 0xF0;
		uint8_t datal = (data << 4) & 0xF0;

		write_nibble(datah | mode);
		write_nibble(datal | mode);

		// ets_delay_us(40); // 37us + 4us execution time
	}

private:
	// writes and clocks-in the data
	void write_nibble(uint8_t data)
	{
		send_i2c(data, false);
		ets_delay_us(1000); // to stabilize pins (why so long gdammit)

		send_i2c(data, true);
		ets_delay_us(1); // enable pulse must be >450ns

		send_i2c(data, false);
		ets_delay_us(40); // 37us + 4us execution time
	}

	void send_i2c(uint8_t data, bool CE)
	{
		esp_err_t ret = ESP_OK;
		i2c_cmd_handle_t cmdh = i2c_cmd_link_create();

		uint8_t mode = (CE ? LCD_CLK_PIN_EN : LCD_CLK_PIN_DIS);

		ESP_GOTO_ON_ERROR(
			i2c_master_start(cmdh),
			err, TAG, "Error with i2c_master_start()");

		ESP_GOTO_ON_ERROR(
			i2c_master_write_byte(cmdh, HEAD, true),
			err, TAG, "Error with i2c_master_write_byte() #1");

		ESP_GOTO_ON_ERROR(
			i2c_master_write_byte(cmdh, data | mode, true),
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
};

#undef TAG