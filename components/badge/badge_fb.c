#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <esp_err.h>
#include <esp_log.h>

#include "badge_fb.h"

#include "badge_pins.h"

static const char *TAG = "badge_fb";

uint8_t *badge_fb = NULL;

esp_err_t
badge_fb_init(void)
{
	if (badge_fb)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	badge_fb = (uint8_t *) malloc(DISPLAY_FB_LEN);
	if (badge_fb == NULL)
		return ESP_ERR_NO_MEM;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

#ifdef I2C_ERC12864_ADDR

void draw_pixel(uint8_t *buf, int x, int y, uint8_t c)
{
	uint16_t targetByte = ((y/8)*BADGE_ERC12864_WIDTH) + x;
	uint8_t targetBit = y%8;
	//printf("draw_pixel(%d, %d, %d): %d/%d\n", x, y, c, targetByte, targetBit);
	if (c > 128) {
		buf[targetByte] &= ~(1 << targetBit);
	} else {
		buf[targetByte] |= (1 << targetBit);
	}
}

void draw_pixel_8b(uint8_t *buf, int x, int y, uint8_t c)
{
	draw_pixel(buf, x, y, c); //The ERC12864 is monochrome only
}

#else

void draw_pixel(uint8_t *buf, int x, int y, uint8_t c)
{
	if (x < 0 || x > (DISPLAY_FB_WIDTH-1) || y < 0 || y > (DISPLAY_FB_HEIGHT-1))
		return;
	int pos = (x >> 3) + y * (DISPLAY_FB_WIDTH / 8);
	if (c < 128) {
		buf[pos] &= 0xff - (1 << (x&7));
	} else {
		buf[pos] |= (1 << (x&7));
	}
}

void draw_pixel_8b(uint8_t *buf, int x, int y, uint8_t c)
{
	if (x < 0 || x > (DISPLAY_FB_WIDTH-1) || y < 0 || y > (DISPLAY_FB_HEIGHT-1))
		return;
	int pos = x + y * DISPLAY_FB_WIDTH;
	buf[pos] = c;
}

#endif
