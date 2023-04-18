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

	void write(uint8_t data, bool BL, bool RS = false)
	{
		uint8_t mode = static_cast<uint8_t>(RS ? LCD_RS_PIN_DATA : LCD_RS_PIN_CMND) | static_cast<uint8_t>(BL ? LCD_BL_PIN_ON : LCD_BL_PIN_OFF);

		uint8_t datah = data & 0xF0;
		uint8_t datal = (data << 4) & 0xF0;

		write_nibble(datah | mode);
		if (RS)
			ets_delay_us(4); // 4us add exec time for DDR/CGR
		write_nibble(datal | mode);
		if (RS)
			ets_delay_us(4); // 4us add exec time for DDR/CGR
	}

	void write_init(uint8_t data)
	{
		data = data & 0xF0;
		write_nibble(data);
	}

private:
	// writes and clocks-in the data
	void write_nibble(uint8_t data)
	{
		send_i2c(data, false);
		ets_delay_us(800); // to stabilize pins (why so long gdammit) <- you can try to adjust it, hey, as long as it works lol

		send_i2c(data, true);
		ets_delay_us(1); // enable pulse must be >450ns

		send_i2c(data, false);
		ets_delay_us(37); // 37us standard execution time
	}

	esp_err_t send_i2c(uint8_t data, bool CE)
	{
		esp_err_t ret = ESP_OK;
		i2c_cmd_handle_t cmdh = i2c_cmd_link_create();

		uint8_t mode = static_cast<uint8_t>(CE ? LCD_CE_PIN_EN : LCD_CE_PIN_DIS);

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
		return ESP_OK;

	err:
		ESP_LOGE(TAG, "Error in ::send_byte(): %s", esp_err_to_name(ret));
		return ret;
	}
};

#undef TAG