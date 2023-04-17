
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "PCF8574.h"
#include "HD44780.h"

static const char *TAG = "LCD_Test";

void init_i2c()
{
	const i2c_config_t i2c_cfg = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = GPIO_NUM_21,
		.scl_io_num = GPIO_NUM_22,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master = {.clk_speed = 100'000},
		.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
	};

	ESP_LOGD(TAG, "Installing i2c driver");
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0); // no buffer for master
	i2c_param_config(I2C_NUM_0, &i2c_cfg);
}

extern "C" void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_VERBOSE);
	ESP_LOGI(TAG, "H E N L O B E N C");
	vTaskDelay(pdMS_TO_TICKS(1000));

	init_i2c();
	ESP_LOGI(TAG, "I2C inited");
	PCF8574 comm(I2C_NUM_0);
	ESP_LOGI(TAG, "PCF8574 constr");
	HD44780<PCF8574> disp(comm, 4, 20);
	ESP_LOGI(TAG, "HD44780 constr");

	disp.write_string("Now thats a pretty really longass string no cap fr");

	ESP_LOGI(TAG, "Written");
}
