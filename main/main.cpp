#include "settings.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

#include <lcd.h>

static const char *TAG = "[" __TIME__ "]🅱 rzedwornidza";

lcd_handle_t lcd_handle;

void setup_lcd()
{
	const i2c_config_t i2c_cfg = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = GPIO_NUM_21,
		.scl_io_num = GPIO_NUM_22,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master = {.clk_speed = 100'000},
		// .slave = {},
		.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
	};

	ESP_LOGD(TAG, "Installing i2c driver");
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0); // no buffer for master
	i2c_param_config(I2C_NUM_0, &i2c_cfg);

	lcd_handle = {
		.i2c_port = I2C_NUM_0,
		.address = 0x3f,
		.columns = 20,
		.rows = 4,
		.display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS,
		.display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF,
		.display_mode = LCD_ENTRY_INCREMENT | LCD_ENTRY_DISPLAY_NO_SHIFT,
		.cursor_column = 0,
		.cursor_row = 0,
		.backlight = LCD_BACKLIGHT_ON,
		.initialized = false,
	};

	// Initialise LCD
	ESP_ERROR_CHECK(lcd_init(&lcd_handle));
}

extern "C" void app_main(void)
{
	ESP_LOGI(TAG, "H E N L O B E N C, Wait");

	esp_log_level_set("*", ESP_LOG_VERBOSE);

	vTaskDelay(pdMS_TO_TICKS(1000));

	setup_lcd();

	lcd_write_str(&lcd_handle, "Start... ");

	// for turning PSU ON
	gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(GPIO_NUM_32, GPIO_PULLUP_ONLY);
	gpio_set_level(GPIO_NUM_32, 1);
	// for reading if PSU OK
	gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);

	// CH: oh, ol; ih, il	// order of driver inputs
	// PN:  5, 17; 16,  4	// order of pins on board
	// FB: ih, il; oh, ol	// order of cnstrctr args
	FullBridge driver(GPIO_NUM_16, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_17);

	lcd_write_str(&lcd_handle, "ustawiony... ");

	vTaskDelay(pdMS_TO_TICKS(10000));

	gpio_set_level(GPIO_NUM_32, 0);

	lcd_write_str(&lcd_handle, "odpalony...");

	while (!gpio_get_level(GPIO_NUM_35))
		vTaskDelay(pdMS_TO_TICKS(10));

	lcd_write_str(&lcd_handle, "gotowy.");

	driver.setRatio(1);

	while (true)
		vTaskDelay(pdMS_TO_TICKS(1000));

	// what the heck you stupid formatter stop stealing my newline faggot
}
