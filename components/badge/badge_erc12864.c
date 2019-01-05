#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_ERC12864_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_ERC12864_DEBUG

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
#include "badge_erc12864.h"

#ifdef I2C_ERC12864_ADDR

static const char *TAG = "badge_erc12864";

#ifndef ERC12864_FLIP
	#define ERC12864_FLIP false
#endif
bool flip = ERC12864_FLIP;

static inline esp_err_t set_page_address(uint8_t page)
{
	esp_err_t res = badge_i2c_write_reg(I2C_ERC12864_ADDR, 0x38, 0xb0 | page);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write page(0x%02x): error %d", page, res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write page(0x%02x): ok", page);

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

	ESP_LOGD(TAG, "i2c write column(0x%02x): ok", column);

	return res;
}

esp_err_t badge_erc12864_init(void)
{
	static bool badge_erc12864_init_done = false;

	if (badge_erc12864_init_done)
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
	badge_erc12864_write(buffer); //Clear screen

	badge_erc12864_init_done = true;
	ESP_LOGD(TAG, "init done");
	return ESP_OK;
}

unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

esp_err_t badge_erc12864_write(const uint8_t *buffer)
{
	esp_err_t res;

	for (uint8_t page = 0; page < 8; page++) {
		res = set_page_address(page);
		if (res != ESP_OK) break;
		for (uint8_t num = 0; num < 4; num++) {
			res = set_column(num*0x20);
			if (res != ESP_OK) break;
			uint16_t offset = 128*page + 32*num;
			if (flip) {
				uint8_t data[32] = {0};
				for (uint8_t i = 0; i < 32; i++) {
					data[i] = reverse(buffer[1023-offset-i]);
				}
				res = badge_i2c_write_buffer(I2C_ERC12864_ADDR+1, data, 32);
			} else {
				res = badge_i2c_write_buffer(I2C_ERC12864_ADDR+1, buffer+offset, 32);
			}
			if (res != ESP_OK) break;
		}
	}

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write data error %d", res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write data ok");
	return res;

	ESP_LOGD(TAG, "erc12864 write stub");
	return ESP_OK;
}

esp_err_t badge_erc12864_write_8bitcompat(const uint8_t *buffer)
{
	uint8_t newBuffer[BADGE_ERC12864_1BPP_DATA_LENGTH];
	memset(newBuffer, 0, BADGE_ERC12864_1BPP_DATA_LENGTH);
	
	for (uint8_t x = 0; x < BADGE_ERC12864_WIDTH; x++) {
		for (uint8_t y = 0; y < BADGE_ERC12864_HEIGHT/8; y++) {
			uint16_t targetByte = (y*BADGE_ERC12864_WIDTH) + x;
			newBuffer[targetByte] = 0;
			for (uint8_t targetBit = 0; targetBit < 8; targetBit++) {
				uint16_t fromByte = ((y*8+targetBit)*BADGE_ERC12864_WIDTH) + x;
				if (buffer[fromByte] < 128) newBuffer[targetByte] += (1<<targetBit);
			}
		}
	}
	return badge_erc12864_write(newBuffer);
}

void badge_erc12864_set_rotation(bool newFlip)
{
	flip = newFlip;
	ESP_LOGW(TAG, "erc12864 flip: %s", flip ? "enabled" : "disabled");
}

#endif // I2C_ERC12864_ADDR
