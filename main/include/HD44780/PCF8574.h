#ifndef HD44780_PCF8574_H
#define HD44780_PCF8574_H

#include <array>

#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include <esp_check.h>

#include <driver/i2c_master.h>
#include <rom/ets_sys.h>

#include "HD44780/HD44780.h"

#include "bitwise_print.h"

#define HD44780_PCF8574_I2C_OPTIMIZE_WRITE

// Class for handling connection to the expander
class HD44780_PCF8574 : public HD44780_Iface
{
	static constexpr const char *const TAG = "HD44780_PCF8574";

	enum lcd_rs_pin_t : uint8_t // RS pin
	{
		LCD_RS_PIN_CMND = 0b00000000, // write command to internals
		LCD_RS_PIN_DATA = 0b00000001, // write data to DDR/CGR
	};

	enum lcd_rw_pin_t : uint8_t // "R ~W" pin
	{
		LCD_RW_PIN_WR = 0b00000000, // write data
		LCD_RW_PIN_RD = 0b00000010, // read data
	};

	enum lcd_ce_pin_t : uint8_t // CE pin
	{
		LCD_CE_PIN_DIS = 0b00000000, // clock disable
		LCD_CE_PIN_EN = 0b00000100,	 // clock enable
	};

	enum lcd_bl_pin_t : uint8_t // BL pin
	{
		LCD_BL_PIN_OFF = 0b00000000, // backlight off
		LCD_BL_PIN_ON = 0b00001000,	 // backlight on
	};

	i2c_master_bus_handle_t i2c_host;
	uint16_t address;
	uint32_t clk_hz;
	i2c_master_dev_handle_t i2c_hdl = nullptr;
	bool BL = false;

public:
	HD44780_PCF8574(i2c_master_bus_handle_t ih, uint16_t a = 0b111, uint32_t chz = 400'000) : i2c_host(ih), address((a & 0b111) | 0b0111000), clk_hz(chz) //
	{
		ESP_LOGI(TAG, "Constructed with address: %d", address); // port: %d, , ih
	}
	virtual ~HD44780_PCF8574() = default;

	esp_err_t init()
	{
		assert(!i2c_hdl);

		i2c_device_config_t dev_cfg = {
			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
			.device_address = address,
			.scl_speed_hz = clk_hz,
			.scl_wait_us = 0,
			.flags = {
				.disable_ack_check = false,
			},
		};

		ESP_RETURN_ON_ERROR(
			i2c_master_bus_add_device(i2c_host, &dev_cfg, &i2c_hdl),
			TAG, "Failed to i2c_master_bus_add_device!");

		return ESP_OK;
	}
	esp_err_t deinit()
	{
		assert(i2c_hdl);

		ESP_RETURN_ON_ERROR(
			i2c_master_bus_rm_device(i2c_hdl),
			TAG, "Failed to i2c_master_bus_rm_device!");

		i2c_hdl = nullptr;

		return ESP_OK;
	}

	//

	esp_err_t write_wide(uint8_t data) override
	{
		uint8_t mode = get_mode(false);

		uint8_t datah = data & 0xF0;
		datah |= mode;

		std::array txbuf = {
#ifndef HD44780_PCF8574_I2C_OPTIMIZE_WRITE
			datah,
#endif
			static_cast<uint8_t>(datah | static_cast<uint8_t>(LCD_CE_PIN_EN)),
			datah,
		};

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit(i2c_hdl, txbuf.data(), txbuf.size(), -1),
			TAG, "Failed to i2c_master_transmit!");

		return ESP_OK;
	}

	esp_err_t write(uint8_t data, bool rs = false) override
	{
		uint8_t mode = get_mode(rs);

		uint8_t datah = data & 0xF0;
		uint8_t datal = data << 4;

		datah |= mode;
		datal |= mode;

		std::array txbuf = {
#ifndef HD44780_PCF8574_I2C_OPTIMIZE_WRITE
			datah,
#endif
			static_cast<uint8_t>(datah | static_cast<uint8_t>(LCD_CE_PIN_EN)),
			datah,
#ifndef HD44780_PCF8574_I2C_OPTIMIZE_WRITE
			datal,
#endif
			static_cast<uint8_t>(datal | static_cast<uint8_t>(LCD_CE_PIN_EN)),
			datal,
		};

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit(i2c_hdl, txbuf.data(), txbuf.size(), -1),
			TAG, "Failed to i2c_master_transmit!");

		return ESP_OK;
	}

	esp_err_t read(uint8_t &data, bool rs = false) override
	{
		// procedure:
		// send RW CE
		// read D
		// send RW!CE and RW CE
		// read D
		// send RW!CE

		uint8_t moder = 0xF0 | get_mode(rs, true);
		// uint8_t modew = 0x00 | get_mode(rs, false);

		std::array txbufw1 = {
			moder,
			static_cast<uint8_t>(moder | static_cast<uint8_t>(LCD_CE_PIN_EN)),
		};
		// read
		std::array txbufw2 = {
			moder, // CE off
			static_cast<uint8_t>(moder | static_cast<uint8_t>(LCD_CE_PIN_EN)),
		};
		// read
		std::array txbufw3 = {
			moder, // CE off
				   // modew, // RW off
		};

		uint8_t datah, datal;

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit_receive(i2c_hdl, txbufw1.data(), txbufw1.size(), &datah, 1, -1),
			TAG, "Failed to i2c_master_transmit_receive!");

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit_receive(i2c_hdl, txbufw2.data(), txbufw2.size(), &datal, 1, -1),
			TAG, "Failed to i2c_master_transmit_receive!");

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit(i2c_hdl, txbufw3.data(), txbufw3.size(), -1),
			TAG, "Failed to i2c_master_transmit!");

		data = (datah & 0xF0) | (datal >> 4);

		return ESP_OK;
	}

	esp_err_t check_bf(bool &bf) override
	{
		// procedure:
		// send RW CE
		// read D
		// send RW !CE and RW CE
		// send RW !CE

		uint8_t moder = 0xF0 | get_mode(false, true);
		// uint8_t modew = 0x00 | get_mode(false, false);

		std::array txbufw1 = {
			moder,
			static_cast<uint8_t>(moder | static_cast<uint8_t>(LCD_CE_PIN_EN)),
		};
		// read
		std::array txbufw2 = {
			moder,
			static_cast<uint8_t>(moder | static_cast<uint8_t>(LCD_CE_PIN_EN)),
			moder,
			// modew,
		};

		uint8_t datah;

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit_receive(i2c_hdl, txbufw1.data(), txbufw1.size(), &datah, 1, -1),
			TAG, "Failed to i2c_master_transmit_receive!");

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit(i2c_hdl, txbufw2.data(), txbufw2.size(), -1),
			TAG, "Failed to i2c_master_transmit!");

		bf = datah & 0xF0;

		return ESP_OK;
	}

	esp_err_t backlight(bool b) override
	{
		BL = b;

		uint8_t mode = get_mode(false);

		std::array txbuf = {
			mode,
		};

		ESP_RETURN_ON_ERROR(
			i2c_master_transmit(i2c_hdl, txbuf.data(), txbuf.size(), -1),
			TAG, "Failed to i2c_master_transmit!");

		return ESP_OK;
	}

	//

	lcd_mode_t bit_mode() const override
	{
		return LCD_4BIT;
	}
	bool can_read() const override
	{
		return true;
	}
	uint32_t bf_us() const override
	{
		// (addr + 2 write + addr + read + addr + 3 write)
		// x (8 bits + ack)
		// + 2 stops
		return (9 * 9 + 2) * 1'000'000 / clk_hz;
		//(9 * 9 + 2)bit * 1000000us/s / (clk_hz bit/s)
	}

private:
	uint8_t get_mode(bool RS, bool RW = false)
	{
		return static_cast<uint8_t>(RS ? LCD_RS_PIN_DATA : LCD_RS_PIN_CMND) | static_cast<uint8_t>(RW ? LCD_RW_PIN_RD : LCD_RW_PIN_WR) | static_cast<uint8_t>(BL ? LCD_BL_PIN_ON : LCD_BL_PIN_OFF);
	}
};

#endif