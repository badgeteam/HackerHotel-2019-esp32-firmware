#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "uart.h"
#include <font.h>
#include <string.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_button.h>
#include <badge_eink.h>
#include <badge_disobey_samd.h>
#include <badge_fb.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_first_run.h>
#include <badge_ota.h>
#include <badge_eink.h>
#include "rom/rtc.h"
#include "esprtcmem.h"

#include "esp_freertos_hooks.h"

#include <lvgl.h>

#define USE_LV_DEMO

#include "demo.h"

extern void micropython_entry(void);
extern uint32_t reset_cause;
extern bool in_safe_mode;

#define BUTTON_SAFE_MODE ((1 << BADGE_BUTTON_START))

/* LVGL TEST */

static void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(portTICK_RATE_MS);
}

void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_map) {
	printf("Flush %d,%d / %d,%d\n", x1, y1, x2, y2);
	uint8_t* colors = color_map;
	//memset(badge_fb, 0xff, DISPLAY_FB_WIDTH*DISPLAY_FB_HEIGHT);
	for (int32_t x = x1; x <= x2; x++) {
		for (int32_t y = y1; y <= y2; y++) {
			uint8_t color = 255-colors[x+y*(x2-x1+1)];// > 64) * 255;//DISPLAY_FB_WIDTH
			//printf("%d,%d: %d\n", x,y,color);
			draw_pixel(badge_fb, x, y, color);//255-colors[x+y*DISPLAY_FB_WIDTH]);
			//printf("%d, %d: %d\n", x, y,  colors[x+y*DISPLAY_FB_WIDTH]);
		}
	}
	badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(BADGE_EINK_LUT_NORMAL));
	//badge_eink_display_greyscale(badge_fb, DISPLAY_FLAG_8BITPIXEL, 2);
	lv_flush_ready();
}

void lvgl_demo()
{
	lv_init();

	lv_disp_drv_t disp;
	lv_disp_drv_init(&disp);
	disp.disp_flush = display_flush;
    //disp.disp_fill = ili9431_fill;
	lv_disp_drv_register(&disp);

    /*lv_indev_drv_t indev;
    lv_indev_drv_init(&indev);
    indev.read = xpt2046_read;
    indev.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev);*/

	esp_register_freertos_tick_hook(lv_tick_task);

	demo_create();

	while(1) {
		vTaskDelay(1);
		lv_task_handler();
	}
}

/* --------- */


void app_main(void) {
	badge_check_first_run();
	badge_init();
	badge_fb_init();
	lvgl_demo();
	
	uint8_t magic = esp_rtcmem_read(0);
	uint8_t inv_magic = esp_rtcmem_read(1);

	if (magic == (uint8_t)~inv_magic) {
		printf("Magic checked out!\n");
		switch (magic) {
			case 1:
				printf("Starting OTA\n");
				badge_ota_update();
				break;
			case 3:
				badge_first_run();
				break;
			
			default:
				printf("Unsupported magic '%d'!\n", magic);
				micropython_entry();
		}

	} else {
		uint32_t reset_cause = rtc_get_reset_reason(0);
		if (reset_cause != DEEPSLEEP_RESET) {
			badge_init();
			if ((badge_input_button_state & BUTTON_SAFE_MODE) == BUTTON_SAFE_MODE) {
				in_safe_mode = true;
			}
		}
		micropython_entry();
	}
}
