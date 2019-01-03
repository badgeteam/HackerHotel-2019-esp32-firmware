#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_DISOBEY_SAMD_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_DISOBEY_SAMD_DEBUG

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
#include "badge_disobey_samd.h"

#ifdef I2C_DISOBEY_SAMD_ADDR

#define disobey_samd_CMD_LED       0x01
#define disobey_samd_CMD_BACKLIGHT 0x02
#define disobey_samd_CMD_BUZZER    0x03
#define disobey_samd_CMD_OFF       0x04

static const char *TAG = "badge_disobey_samd";

// mutex for accessing badge_disobey_samd_state, badge_disobey_samd_handlers, etc..
xSemaphoreHandle badge_disobey_samd_mux = NULL;

// semaphore to trigger disobey_samd interrupt handling
xSemaphoreHandle badge_disobey_samd_intr_trigger = NULL;

// handlers per disobey_samd port.
badge_disobey_samd_intr_t badge_disobey_samd_handler = NULL;
void* badge_disobey_samd_arg[12] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

int badge_disobey_samd_read_state()
{
	uint8_t state[2];
	esp_err_t res = badge_i2c_read_reg(I2C_DISOBEY_SAMD_ADDR, 0, state, 2);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c read state error %d", res);
		return -1;
	}

	int value = state[0] + (state[1] << 8);

	ESP_LOGD(TAG, "i2c read state 0x%04x", value);

	return value;
}

int badge_disobey_samd_read_touch()
{
	int touch = badge_disobey_samd_read_state() & 0x3F;
	ets_printf("badge_disobey_samd: read touch: 0x%02X\n", touch);
	return touch;
}

int badge_disobey_samd_read_usb()
{
	return (badge_disobey_samd_read_state()>>6) & 0x01;
}

int badge_disobey_samd_read_battery()
{
	return badge_disobey_samd_read_state()>>8 & 0xFF;
}

static inline esp_err_t
badge_disobey_samd_write_reg(uint8_t reg, uint8_t value)
{
	esp_err_t res = badge_i2c_write_reg(I2C_DISOBEY_SAMD_ADDR, reg, value);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write reg(0x%02x, 0x%02x): error %d", reg, value, res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write reg(0x%02x, 0x%02x): ok", reg, value);

	return res;
}

static inline esp_err_t
badge_disobey_samd_write_reg32(uint8_t reg, uint32_t value)
{
	esp_err_t res = badge_i2c_write_reg32(I2C_DISOBEY_SAMD_ADDR, reg, value);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write reg32(0x%08x, 0x%02x): error %d", reg, value, res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write reg32(0x%02x, 0x%08x): ok", reg, value);

	return res;
}

esp_err_t badge_disobey_samd_write_backlight(uint8_t value)
{
	return badge_disobey_samd_write_reg(disobey_samd_CMD_BACKLIGHT, value);
}

esp_err_t badge_disobey_samd_write_led(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t value = led + (r << 8) + (g << 16) + (b << 24);
	return badge_disobey_samd_write_reg32(disobey_samd_CMD_LED, value);
}

esp_err_t badge_disobey_samd_write_buzzer(uint16_t freqency, uint16_t duration)
{
	uint32_t value = freqency + (duration << 16);
	return badge_disobey_samd_write_reg32(disobey_samd_CMD_BUZZER, value);
}

esp_err_t badge_disobey_samd_write_off()
{
	return badge_disobey_samd_write_reg32(disobey_samd_CMD_OFF, 0);
}

void
badge_disobey_samd_intr_task(void *arg)
{
	// we cannot use I2C in the interrupt handler, so we
	// create an extra thread for this..

	int old_state = 0;
	while (1)
	{
		if (xSemaphoreTake(badge_disobey_samd_intr_trigger, portMAX_DELAY))
		{
			int state;
			while (1)
			{
				state = badge_disobey_samd_read_touch();
				if (state != -1)
					break;

				ESP_LOGE(TAG, "failed to read status.");
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

			int pressed = 0;
			int released = 0;

			for (uint8_t i = 0; i<6; i++) {
				if (((state>>i)&0x01)&&(!((old_state>>i)&0x01))) pressed |= (1<<i);
				if (((old_state>>i)&0x01)&&(!((state>>i)&0x01))) released |= (1<<i);
			}

			if (state != old_state) {
				xSemaphoreTake(badge_disobey_samd_mux, portMAX_DELAY);
				badge_disobey_samd_intr_t handler = badge_disobey_samd_handler;
				xSemaphoreGive(badge_disobey_samd_mux);

				if (handler != NULL) {
					handler(pressed, released);
				}
			}

			old_state = state;
		}
	}
}

void
badge_disobey_samd_intr_handler(void *arg)
{ /* in interrupt handler */
	int gpio_state = gpio_get_level(PIN_NUM_DISOBEY_SAMD_INT);

#ifdef CONFIG_SHA_BADGE_DISOBEY_SAMD_DEBUG
	static int gpio_last_state = -1;
	if (gpio_state != -1 && gpio_last_state != gpio_state)
	{
		ets_printf("badge_disobey_samd: I2C Int %s\n", gpio_state == 0 ? "up" : "down");
	}
	gpio_last_state = gpio_state;
#endif // CONFIG_SHA_BADGE_disobey_samd_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_disobey_samd_intr_trigger, NULL);
	}
}

esp_err_t
badge_disobey_samd_init(void)
{
	static bool badge_disobey_samd_init_done = false;

	if (badge_disobey_samd_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

	res = badge_i2c_init();
	if (res != ESP_OK)
		return res;

	badge_disobey_samd_mux = xSemaphoreCreateMutex();
	if (badge_disobey_samd_mux == NULL)
		return ESP_ERR_NO_MEM;

	badge_disobey_samd_intr_trigger = xSemaphoreCreateBinary();
	if (badge_disobey_samd_intr_trigger == NULL)
		return ESP_ERR_NO_MEM;

	res = gpio_isr_handler_add(PIN_NUM_DISOBEY_SAMD_INT, badge_disobey_samd_intr_handler, NULL);
	if (res != ESP_OK)
		return res;

	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_ANYEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = 1LL << PIN_NUM_DISOBEY_SAMD_INT,
		.pull_down_en = 0,
		.pull_up_en   = 0,
	};
	res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;

	xTaskCreate(&badge_disobey_samd_intr_task, "disobey_samd interrupt task", 4096, NULL, 10, NULL);

	xSemaphoreGive(badge_disobey_samd_intr_trigger);

	badge_disobey_samd_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

void
badge_disobey_samd_set_interrupt_handler(badge_disobey_samd_intr_t handler)
{
	if (badge_disobey_samd_mux == NULL)
	{ // allow setting handlers when badge_disobey_samd is not initialized yet.
		badge_disobey_samd_handler = handler;
	}
	else
	{
		xSemaphoreTake(badge_disobey_samd_mux, portMAX_DELAY);

		badge_disobey_samd_handler = handler;

		xSemaphoreGive(badge_disobey_samd_mux);
	}
}

esp_err_t
badge_disobey_samd_get_touch_info(struct badge_disobey_samd_touch_info *info)
{
	int value = badge_disobey_samd_read_touch();
	if (value == -1)
		return ESP_FAIL; // need more-specific error?
	info->touch_state = value;

	return ESP_OK;
}

#endif // I2C_DISOBEY_SAMD_ADDR
