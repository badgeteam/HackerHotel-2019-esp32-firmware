#include <sdkconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_i2c.h"

#include "ERC12864.h"

#ifdef I2C_ERC12864_ADDR

static const char *TAG = "display_erc12864";

/* Internal functions */

static inline esp_err_t set_page_address(uint8_t page)
{
	esp_err_t res = badge_i2c_write_reg(I2C_ERC12864_ADDR, 0x38, 0xb0 | page);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write page(0x%02x): error %d", page, res);
		return res;
	}

	//ESP_LOGD(TAG, "i2c write page(0x%02x): ok", page);

	return res;
}

static inline esp_err_t set_column(uint8_t column)
{
  uint8_t buffer[] = {0x38, 0x10 | (column>>4), (0x0f&column) | 0x04};

	esp_err_t res = badge_i2c_write_buffer(I2C_ERC12864_ADDR, buffer, 3);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write column(0x%02x): error %d", column, res);
		return res;
	}

	//ESP_LOGD(TAG, "i2c write column(0x%02x): ok", column);

	return res;
}

/* Public functions */

esp_err_t display_erc12864_init(void)
{
	static bool display_erc12864_init_done = false;

	if (display_erc12864_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

	res = badge_i2c_init();
	if (res != ESP_OK)
		return res;

	uint8_t initSeq[] = {0x38, 0x2f, 0xA2, 0xA1, 0xC8, 0x24, 0x81, 37, 0x40, 0xAF};

	res = badge_i2c_write_buffer(I2C_ERC12864_ADDR, initSeq, 10);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write init: error %d", res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write init: ok");

	uint8_t buffer[1024] = {0};
	display_erc12864_write(buffer); //Clear screen

	display_erc12864_init_done = true;
	ESP_LOGD(TAG, "init done");
	return ESP_OK;
}

esp_err_t display_erc12864_write(const uint8_t *buffer)
{
	esp_err_t res;

	for (uint8_t page = 0; page < 8; page++) {
		res = set_page_address(page);
		if (res != ESP_OK) break;
		for (uint8_t num = 0; num < 4; num++) {
			res = set_column(num*0x20);
			if (res != ESP_OK) break;
			uint16_t offset = 128*page + 32*num;
			res = badge_i2c_write_buffer(I2C_ERC12864_ADDR+1, buffer+offset, 32);
			if (res != ESP_OK) break;
		}
	}

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write data error %d", res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write data ok");
	return res;
}

#endif // I2C_ERC12864_ADDR
