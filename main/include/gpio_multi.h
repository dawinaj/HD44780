#ifndef GPIO_MULTI_H
#define GPIO_MULTI_H

#include <soc/gpio_reg.h>
#include <esp_log.h>
#include <esp_check.h>

#include <driver/gpio.h>

esp_err_t gpio_set_level_multi(const gpio_num_t *gpio_num, const bool *level, size_t count)
{
	uint32_t regs[4] = {0, 0, 0, 0}; // c0, c1, s0, s1

	for (size_t i = 0; i < count; ++i)
	{
		gpio_num_t pin = gpio_num[i];
		uint32_t bit = BIT(pin & 31);
		size_t idx = (pin >= 32) + 2 * level[i];
		regs[idx] |= bit;
	}

	if (regs[0])
		REG_WRITE(GPIO_OUT_W1TC_REG, regs[0]);
	if (regs[1])
		REG_WRITE(GPIO_OUT1_W1TC_REG, regs[1]);
	if (regs[2])
		REG_WRITE(GPIO_OUT_W1TS_REG, regs[2]);
	if (regs[3])
		REG_WRITE(GPIO_OUT1_W1TS_REG, regs[3]);

	return ESP_OK;
}

esp_err_t gpio_get_level_multi(const gpio_num_t *gpio_num, bool *level, size_t count)
{
	uint32_t regs[2] = {REG_READ(GPIO_IN_REG), REG_READ(GPIO_IN1_REG)};

	for (size_t i = 0; i < count; ++i)
	{
		gpio_num_t pin = gpio_num[i];
		uint32_t bit = BIT(pin & 31);
		size_t idx = (pin >= 32);
		level[i] = regs[idx] & bit;
	}

	return ESP_OK;
}

esp_err_t gpio_set_level_multi_high(const gpio_num_t *gpio_num, size_t count)
{
	uint32_t regs[2] = {0, 0}; // s0, s1

	for (size_t i = 0; i < count; ++i)
	{
		gpio_num_t pin = gpio_num[i];
		uint32_t bit = BIT(pin & 31);
		size_t idx = (pin >= 32);
		regs[idx] |= bit;
	}

	if (regs[0])
		REG_WRITE(GPIO_OUT_W1TS_REG, regs[0]);
	if (regs[1])
		REG_WRITE(GPIO_OUT1_W1TS_REG, regs[1]);

	return ESP_OK;
}

esp_err_t gpio_set_level_multi_low(const gpio_num_t *gpio_num, size_t count)
{
	uint32_t regs[2] = {0, 0}; // c0, c1

	for (size_t i = 0; i < count; ++i)
	{
		gpio_num_t pin = gpio_num[i];
		uint32_t bit = BIT(pin & 31);
		size_t idx = (pin >= 32);
		regs[idx] |= bit;
	}

	if (regs[0])
		REG_WRITE(GPIO_OUT_W1TC_REG, regs[0]);
	if (regs[1])
		REG_WRITE(GPIO_OUT1_W1TC_REG, regs[1]);

	return ESP_OK;
}

#endif
