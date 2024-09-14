#ifndef HD44780_GPIO4BIT_H
#define HD44780_GPIO4BIT_H

#include <array>

#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include <esp_check.h>

#include <driver/gpio.h>
#include <rom/ets_sys.h>

#include "HD44780/HD44780.h"

#include "gpio_multi.h"

#include "bitwise_print.h"

//

// Class for handling connection to the expander
class HD44780_GPIO4BIT : public HD44780_Iface
{
	static constexpr const char *const TAG = "HD44780_GPIO4BIT";
	static constexpr uint32_t AS_DELAY_US = 0;
	static constexpr uint32_t CE_DELAY_US = 1;

	const gpio_num_t RS, RW, CE, BL;
	const std::array<gpio_num_t, 4> D; // 7, 6, 5, 4

public:
	HD44780_GPIO4BIT(gpio_num_t rs, gpio_num_t rw, gpio_num_t ce, const std::array<gpio_num_t, 4> &d, gpio_num_t bl = GPIO_NUM_NC) : RS(rs), RW(rw), CE(ce), BL(bl), D(d)
	{
		ESP_LOGI(TAG, "Constructed with GPIO");
	}
	virtual ~HD44780_GPIO4BIT() = default;

	esp_err_t init()
	{
		ESP_RETURN_ON_FALSE(
			GPIO_IS_VALID_OUTPUT_GPIO(RS),
			ESP_ERR_INVALID_ARG, TAG, "RS is not an output GPIO!");

		ESP_RETURN_ON_FALSE(
			GPIO_IS_VALID_OUTPUT_GPIO(CE),
			ESP_ERR_INVALID_ARG, TAG, "CE is not an output GPIO!");

		for (size_t i = 0; i < D.size(); ++i)
			ESP_RETURN_ON_FALSE(
				GPIO_IS_VALID_OUTPUT_GPIO(D[i]),
				ESP_ERR_INVALID_ARG, TAG, "D%i is not an output GPIO!", 7 - i);

		//

		gpio_config_t gpio_data_cnf = {
			.pin_bit_mask = BIT64(D[0]) | BIT64(D[1]) | BIT64(D[2]) | BIT64(D[3]),
			.mode = GPIO_MODE_INPUT_OUTPUT_OD,
			.pull_up_en = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE,
		};

		gpio_config_t gpio_ctrl_cnf = {
			.pin_bit_mask = BIT64(RS) | BIT64(CE) | (GPIO_IS_VALID_OUTPUT_GPIO(RW) ? BIT64(RW) : 0) | (GPIO_IS_VALID_OUTPUT_GPIO(BL) ? BIT64(BL) : 0),
			.mode = GPIO_MODE_OUTPUT,
			.pull_up_en = GPIO_PULLUP_DISABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE,
		};

		ESP_RETURN_ON_ERROR(
			gpio_config(&gpio_data_cnf),
			TAG, "Failed to gpio_config!");

		ESP_RETURN_ON_ERROR(
			gpio_config(&gpio_ctrl_cnf),
			TAG, "Failed to gpio_config!");

		return ESP_OK;
	}
	esp_err_t deinit()
	{
		gpio_config_t gpio_rst_cfg = {
			.pin_bit_mask = (BIT64(D[0]) | BIT64(D[1]) | BIT64(D[2]) | BIT64(D[3])) | (BIT64(RS) | BIT64(CE) | (GPIO_IS_VALID_OUTPUT_GPIO(RW) ? BIT64(RW) : 0) | (GPIO_IS_VALID_OUTPUT_GPIO(BL) ? BIT64(BL) : 0)),
			.mode = GPIO_MODE_DISABLE,
			.pull_up_en = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type = GPIO_INTR_DISABLE,
		};

		ESP_RETURN_ON_ERROR(
			gpio_config(&gpio_rst_cfg),
			TAG, "Failed to gpio_config!");

		return ESP_OK;
	}

	//

	esp_err_t write_wide(uint8_t data) override
	{
		ESP_RETURN_ON_ERROR(
			set_mode(false, false),
			TAG, "Failed to set_mode!");

		uint8_t datah = data & 0xF0;

		ESP_RETURN_ON_ERROR(
			write_nibble(datah),
			TAG, "Failed to write_nibble!");

		return ESP_OK;
	}

	esp_err_t write(uint8_t data, bool rs = false) override
	{
		ESP_RETURN_ON_ERROR(
			set_mode(rs, false),
			TAG, "Failed to set_mode!");

		uint8_t datah = data >> 4;
		uint8_t datal = data & 0xF;

		ESP_RETURN_ON_ERROR(
			write_nibble(datah),
			TAG, "Failed to write_nibble!");

		ESP_RETURN_ON_ERROR(
			write_nibble(datal),
			TAG, "Failed to write_nibble!");

		return ESP_OK;
	}

	esp_err_t read(uint8_t &data, bool rs = false) override
	{
		ESP_RETURN_ON_ERROR(
			set_mode(rs, true),
			TAG, "Failed to set_mode!");

		ESP_RETURN_ON_ERROR(
			gpio_set_level_multi_high(D.data(), 4),
			TAG, "Failed to gpio_set_level_multi!");

		uint8_t datah;
		uint8_t datal;

		ESP_RETURN_ON_ERROR(
			read_nibble(datah),
			TAG, "Failed to read_nibble!");

		ESP_RETURN_ON_ERROR(
			read_nibble(datal),
			TAG, "Failed to read_nibble!");

		data = datah << 4 | datal;

		return ESP_OK;
	}

	esp_err_t check_bf(bool &bf) override
	{
		ESP_RETURN_ON_ERROR(
			set_mode(false, true),
			TAG, "Failed to set_mode!");

		ESP_RETURN_ON_ERROR(
			gpio_set_level_multi_high(D.data(), 4),
			TAG, "Failed to gpio_set_level_multi_high!");

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 1),
			TAG, "Failed to gpio_set_level!");

		ets_delay_us(CE_DELAY_US);

		bf = gpio_get_level(D[0]);

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 0),
			TAG, "Failed to gpio_set_level!");

		// ets_delay_us(HD44780_GPIO4BIT_BITBANG_DELAY_US);

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 1),
			TAG, "Failed to gpio_set_level!");

		ets_delay_us(CE_DELAY_US);

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 0),
			TAG, "Failed to gpio_set_level!");

		return ESP_OK;
	}

	//

	esp_err_t backlight(bool b) override
	{
		ESP_RETURN_ON_FALSE(
			GPIO_IS_VALID_OUTPUT_GPIO(BL),
			ESP_ERR_NOT_ALLOWED, TAG, "BL is not an output GPIO!");

		ESP_RETURN_ON_ERROR(
			gpio_set_level(BL, b),
			TAG, "Failed to gpio_set_level!");

		return ESP_OK;
	}

	//

	lcd_mode_t bit_mode() const override
	{
		return LCD_4BIT;
	}
	bool can_read() const override
	{
		return GPIO_IS_VALID_OUTPUT_GPIO(RW);
	}
	uint32_t bf_us() const override
	{
		return 5;
	}

private:
	esp_err_t write_nibble(uint8_t nib)
	{
		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 1),
			TAG, "Failed to gpio_set_level!");

		bool vals[4] = {static_cast<bool>(nib & 0b1000), static_cast<bool>(nib & 0b0100), static_cast<bool>(nib & 0b0010), static_cast<bool>(nib & 0b0001)};

		ESP_RETURN_ON_ERROR(
			gpio_set_level_multi(D.data(), vals, 4),
			TAG, "Failed to gpio_set_level_multi!");

		ets_delay_us(CE_DELAY_US);

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 0),
			TAG, "Failed to gpio_set_level!");

		// ets_delay_us(CE_DELAY_US);

		return ESP_OK;
	}

	esp_err_t read_nibble(uint8_t &nib)
	{
		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 1),
			TAG, "Failed to gpio_set_level!");

		ets_delay_us(CE_DELAY_US);

		bool vals[4] = {false, false, false, false};

		ESP_RETURN_ON_ERROR(
			gpio_get_level_multi(D.data(), vals, 4),
			TAG, "Failed to gpio_get_level_multi!");

		nib = (vals[0] << 3) | (vals[1] << 2) | (vals[2] << 1) | (vals[3] << 0);

		ESP_RETURN_ON_ERROR(
			gpio_set_level(CE, 0),
			TAG, "Failed to gpio_set_level!");

		return ESP_OK;
	}

	esp_err_t set_mode(bool rs, bool rw)
	{
		bool vals[2] = {rs, rw};
		static const gpio_num_t pins[2] = {RS, RW};

		ESP_RETURN_ON_ERROR(
			gpio_set_level_multi(pins, vals, 2),
			TAG, "Failed to gpio_set_level_multi!");

		ets_delay_us(AS_DELAY_US);

		return ESP_OK;
	}
};

#endif