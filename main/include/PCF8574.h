#pragma once

#include <esp_check.h>
#include <driver/i2c.h>
#include <rom/ets_sys.h>

#include "HD44780/commands.h"
#include "HD44780/pins.h"

#define TAG "PCF8574"



// Class for handling connection to the expander
class PCF8574
{
	i2c_port_t PORT;
	uint8_t HEAD;

public:
	const lcd_mode_t mode = LCD_4BIT;

	PCF8574(i2c_port_t port, uint8_t address = 0x3F) : PORT(port), HEAD((address << 1) | I2C_MASTER_WRITE) {}
	~PCF8574() = default;

	esp_err_t write(uint8_t data, bool BL, bool RS = false)
	{
		uint8_t mode = static_cast<uint8_t>(RS ? LCD_RS_PIN_DATA : LCD_RS_PIN_CMND) | static_cast<uint8_t>(BL ? LCD_BL_PIN_ON : LCD_BL_PIN_OFF);

		uint8_t datah = data & 0xF0;
		uint8_t datal = (data << 4) & 0xF0;

		ESP_RETURN_ON_ERROR(
			write_nibble(datah | mode),
			TAG, "Error in ::write() with write_nibble() #1");

		// if (RS)
		// 	ets_delay_us(4); // 4us add exec time for DDR/CGR

		ESP_RETURN_ON_ERROR(
			write_nibble(datal | mode),
			TAG, "Error in ::write() with write_nibble() #2");

		if (RS)
			ets_delay_us(4); // 4us add exec time for DDR/CGR

		return ESP_OK;
	}

	esp_err_t write_init(uint8_t data)
	{
		data = data & 0xF0;

		ESP_RETURN_ON_ERROR(
			write_nibble(data),
			TAG, "Error in ::write_init() with write_nibble()");

		return ESP_OK;
	}

private:
	// writes and clocks-in the data
	esp_err_t write_nibble(uint8_t data)
	{
		ESP_RETURN_ON_ERROR(
			send_i2c(data, false),
			TAG, "Error in ::write_nibble() with send_i2c() #1");

		ets_delay_us(800); // to stabilize pins (why so long gdammit) <- you can try to adjust it, hey, as long as it works lol

		ESP_RETURN_ON_ERROR(
			send_i2c(data, true),
			TAG, "Error in ::write_nibble() with send_i2c() #2");

		ets_delay_us(1); // enable pulse must be >450ns

		ESP_RETURN_ON_ERROR(
			send_i2c(data, false),
			TAG, "Error in ::write_nibble() with send_i2c() #3");

		ets_delay_us(37); // 37us standard execution time

		return ESP_OK;
	}

	// sends command over I2C
	esp_err_t send_i2c(uint8_t data, bool CE)
	{
		i2c_cmd_handle_t cmdh = i2c_cmd_link_create();

		uint8_t mode = static_cast<uint8_t>(CE ? LCD_CE_PIN_EN : LCD_CE_PIN_DIS);

		ESP_RETURN_ON_ERROR(
			i2c_master_start(cmdh),
			TAG, "Error in ::send_byte() with i2c_master_start()");

		ESP_RETURN_ON_ERROR(
			i2c_master_write_byte(cmdh, HEAD, true),
			TAG, "Error in ::send_byte() with i2c_master_write_byte() #1");

		ESP_RETURN_ON_ERROR(
			i2c_master_write_byte(cmdh, data | mode, true),
			TAG, "Error in ::send_byte() with i2c_master_write_byte() #2");

		ESP_RETURN_ON_ERROR(
			i2c_master_stop(cmdh),
			TAG, "Error in ::send_byte() with i2c_master_stop()");

		ESP_RETURN_ON_ERROR(
			i2c_master_cmd_begin(PORT, cmdh, pdMS_TO_TICKS(1000)),
			TAG, "Error in ::send_byte() with i2c_master_cmd_begin()");

		i2c_cmd_link_delete(cmdh);

		return ESP_OK;
	}
};

#undef TAG