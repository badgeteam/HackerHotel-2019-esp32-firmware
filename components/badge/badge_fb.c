#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <esp_err.h>
#include <esp_log.h>

#include "badge_fb.h"

static const char *TAG = "badge_fb";

uint8_t *badge_fb = NULL;

esp_err_t
badge_fb_init(void)
{
	if (badge_fb)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	badge_fb = (uint8_t *) malloc(BADGE_FB_LEN);
	if (badge_fb == NULL)
		return ESP_ERR_NO_MEM;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}
