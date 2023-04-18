
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "PCF8574.h"
#include "HD44780.h"

// #include "lcd.h"

static const char *TAG = "LCD_Test";

//

// lcd_handle_t lcd_handle = LCD_HANDLE_DEFAULT_CONFIG();

// void init_lcd()
// {
// 	lcd_handle.i2c_port = I2C_NUM_0;
// 	lcd_handle.address = 0x3F;
// 	lcd_handle.columns = 20;
// 	lcd_handle.rows = 4;
// 	lcd_handle.backlight = LCD_BACKLIGHT_ON;

// 	lcd_init(&lcd_handle);
// }

//

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

//

void test_write_string(HD44780<PCF8574> &disp)
{
	disp.write_cstr("Now thats a pretty really longass string no cap fr, now let it overwrite itself lmao ongod", 33);
}

void test_write_string_special(HD44780<PCF8574> &disp)
{
	disp.write_cstr("This\tis\tTAB\nAnd boom newline.", 100);
}

void test_write_cgram(HD44780<PCF8574> &disp)
{
	lcd_symbol_t symbols[8] = {
		{0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F},
		{0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F},
		{0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F},
		{0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C},
		{0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F},
		{0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F},
	};

	for (uint8_t i = 0; i < 8; ++i)
		disp.write_cgram(i, symbols[i]);

	char buf[] = {0, 1, 2, 3, 4, 5, 6, 7};
	disp.write_bffr(buf, 8);
}

//

extern "C" void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_VERBOSE);
	ESP_LOGI(TAG, "H E N L O B E N C");
	vTaskDelay(pdMS_TO_TICKS(1000));

	init_i2c();
	ESP_LOGI(TAG, "I2C inited");

	PCF8574 comm(I2C_NUM_0, 0x3F);
	ESP_LOGI(TAG, "PCF8574 constr");
	HD44780<PCF8574> disp(comm, {4, 20}, LCD_2LINE);
	ESP_LOGI(TAG, "HD44780 constr");

	ESP_LOGI(TAG, "Writing...");

	vTaskDelay(pdMS_TO_TICKS(1000));
	test_write_string(disp);
	vTaskDelay(pdMS_TO_TICKS(3000));

	disp.clear();

	vTaskDelay(pdMS_TO_TICKS(1000));
	test_write_string_special(disp);
	vTaskDelay(pdMS_TO_TICKS(3000));

	disp.clear();

	vTaskDelay(pdMS_TO_TICKS(1000));
	test_write_cgram(disp);
	vTaskDelay(pdMS_TO_TICKS(3000));

	disp.clear();

	ESP_LOGI(TAG, "Written");
}
