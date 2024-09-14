// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_check.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>

// #include "QMC5883L.h"
// #include "BME280.h"
// #include "MCP23008.h"
#include "HD44780/PCF8574.h"
#include "HD44780/GPIO4BIT.h"
// #include "HD44780.h"

static const char *TAG = "Test";

spi_host_device_t init_spi()
{
	spi_bus_config_t bus_cfg = {
		.mosi_io_num = GPIO_NUM_23,
		.miso_io_num = GPIO_NUM_19, // GPIO_NUM_19, GPIO_NUM_NC
		.sclk_io_num = GPIO_NUM_18,
		.quadwp_io_num = GPIO_NUM_NC,
		.quadhd_io_num = GPIO_NUM_NC,
		.data4_io_num = GPIO_NUM_NC,
		.data5_io_num = GPIO_NUM_NC,
		.data6_io_num = GPIO_NUM_NC,
		.data7_io_num = GPIO_NUM_NC,
		.max_transfer_sz = 0,
		.flags = SPICOMMON_BUSFLAG_MASTER,
		.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
		.intr_flags = 0,
	};

	spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_DISABLED); // SPI_DMA_DISABLED // SPI_DMA_CH_AUTO

	return SPI3_HOST;
}

i2c_master_bus_handle_t init_i2c()
{
	i2c_master_bus_config_t bus_cfg = {
		.i2c_port = -1,
		.sda_io_num = GPIO_NUM_21,
		.scl_io_num = GPIO_NUM_22,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		.intr_priority = 0,
		.trans_queue_depth = 0,
		.flags = {
			.enable_internal_pullup = true,
		},
	};

	i2c_master_bus_handle_t bus_handle;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

	return bus_handle;
}

void i2c_probe(i2c_master_bus_handle_t bus_handle, uint16_t limit = 128)
{
	for (uint16_t address = 0; address < limit; ++address)
	{
		esp_err_t ret = i2c_master_probe(bus_handle, address, -1);
		switch (ret)
		{
		case ESP_OK:
			ESP_LOGD("I2C_Probe", "Addr: %04" PRIu16 " found device", address);
			break;
		case ESP_ERR_NOT_FOUND:
			ESP_LOGD("I2C_Probe", "Addr: %04" PRIu16 " NACK", address);
			break;
		case ESP_ERR_TIMEOUT:
			ESP_LOGD("I2C_Probe", "Addr: %04" PRIu16 " timeout", address);
			break;
		default:
			ESP_LOGD("I2C_Probe", "Addr: %04" PRIu16 " shit", address);
			break;
		}
	}
}

//

void test_write_string(HD44780 &disp)
{
	disp.write_cstr("Now thats a pretty really longass string no cap fr, now let it over", 0); // write itself lmao ongod
}

void test_write_string_special(HD44780 &disp)
{
	disp.write_cstr("This\tis\tTAB\nAnd boom newline.", 10);
}

void test_write_cgram(HD44780 &disp)
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

	uint8_t buf[] = {0, 1, 2, 3, 4, 5, 6, 7};
	disp.write_bffr(buf, 8);
}

void test_write_increasing(HD44780 &disp)
{
	uint8_t i = 0;
	do
	{
		disp.write_char(i);
		vTaskDelay(pdMS_TO_TICKS(100));
	} while (++i != 0);
}

//

extern "C" void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_DEBUG); // ESP_LOG_VERBOSE

	spi_host_device_t spi_host = init_spi();
	i2c_master_bus_handle_t i2c_host = init_i2c();

	/*/
	HD44780_PCF8574 comm(i2c_host, 0b111, 400000);
	comm.init();
	HD44780 disp(&comm, {4, 20}, LCD_2LINE);
	disp.init();
	disp.backlight(true);
	/*/
	HD44780_GPIO4BIT comm(GPIO_NUM_25, GPIO_NUM_15, GPIO_NUM_26, {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27}, GPIO_NUM_NC); // {GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22}
	comm.init();
	HD44780 disp(&comm, {2, 16}, LCD_2LINE);
	disp.init();
	//*/

	while (1)
	{
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

		vTaskDelay(pdMS_TO_TICKS(1000));
		test_write_increasing(disp);
		vTaskDelay(pdMS_TO_TICKS(3000));
		disp.clear();

		ESP_LOGI(TAG, "Written");
	}

	/*/
		QMC5883L mag(i2c_host);
		mag.init();

		mag.set_mode(QMC5883L_MODE_CNTNS);
		mag.set_rate(QMC5883L_RATE_200HZ);
		mag.set_range(QMC5883L_RANGE_8G);
		mag.set_samples(QMC5883L_SAMPLES_64);

		while (true)
		{
			mag.read_3d();
			vTaskDelay(pdMS_TO_TICKS(1000));
		}

		mag.deinit();
	//*/
}
