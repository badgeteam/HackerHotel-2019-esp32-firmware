#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <font.h>
#include <string.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_eink.h>
#include <badge_disobey_samd.h>
#include <badge_fb.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_first_run.h>
#include <badge_ota.h>

#ifdef PIN_NUM_EPD_CLK
#include "imgv2_sha.h"
#include "imgv2_menu.h"
#include "imgv2_nick.h"
#include "imgv2_weather.h"
#include "imgv2_test.h"
#endif

#ifdef I2C_ERC12864_ADDR
#include "imgv2_disobey.h"
#include "imgv2_woezelenpip.h"
#endif

struct menu_item {
  const char *title;
  void (*handler)(void);
};

#include "demo_text1.h"
#include "demo_text2.h"
#include "demo_greyscale1.h"
#include "demo_greyscale2.h"
#include "demo_greyscale_img1.h"
#include "demo_greyscale_img2.h"
#include "demo_greyscale_img3.h"
#include "demo_greyscale_img4.h"
#include "demo_partial_update.h"
#include "demo_dot1.h"
#include "demo_test_adc.h"
#include "demo_leds.h"
#include "demo_ugfx.h"
#include "demo_power.h"
#include "demo_mpr121.h"
#include "demo_sdcard_image.h"

extern void micropython_entry(void);

const struct menu_item demoMenu[] = {
	{"MicroPython", &micropython_entry},
#ifdef I2C_MPR121_ADDR
    {"mpr121 touch demo", &demoMpr121},
#endif // I2C_MPR121_ADDR
#ifdef PIN_NUM_EPD_CLK
    {"text demo 1", &demoText1},
    {"text demo 2", &demoText2},
    {"greyscale 1", &demoGreyscale1},
    {"greyscale 2", &demoGreyscale2},
    {"greyscale image 1", &demoGreyscaleImg1},
    {"greyscale image 2", &demoGreyscaleImg2},
    {"greyscale image 3", &demoGreyscaleImg3},
    {"greyscale image 4", &demoGreyscaleImg4},
	{"demo sd-card image", &demo_sdcard_image},
    {"partial update test", &demoPartialUpdate},
    {"dot 1", &demoDot1},
#endif
#ifndef CONFIG_DISOBEY
    {"ADC test", &demoTestAdc},
#endif
#ifdef PIN_NUM_LEDS
    {"LEDs demo", &demo_leds},
#endif // PIN_NUM_LEDS
    {"uGFX demo", &demoUgfx},
#ifndef CONFIG_DISOBEY
    {"charging demo", &demoPower},
#endif
#ifdef CONFIG_WIFI_USE
    {"OTA update", &badge_ota_update},
#endif // CONFIG_WIFI_USE
    {NULL, NULL},
};

#ifdef I2C_ERC12864_ADDR
	#define MENU_FONT 0
	#define MENU_FONT_SIZE 8
	#define MENU_SIZE 5
#else
	#define MENU_FONT FONT_16PX
	#define MENU_FONT_SIZE 16
	#define MENU_SIZE 7
#endif

void
displayMenu(const char *menu_title, const struct menu_item *itemlist) {
	int num_items = 0;
	while (itemlist[num_items].title != NULL)
		num_items++;

	int scroll_pos = 0;
	int item_pos = 0;
	bool need_redraw = true;
	while (1) {
		/* draw menu */
		if (need_redraw) {
			// init buffer
			draw_font(badge_fb, 0, 0, BADGE_FB_WIDTH, menu_title,
					FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH | FONT_UNDERLINE_2);
			int i;
			for (i = 0; i < MENU_SIZE; i++) {
				int pos = scroll_pos + i;
				draw_font(badge_fb, 0, 16+MENU_FONT_SIZE*i, BADGE_FB_WIDTH,
						(pos < num_items) ? itemlist[pos].title : "",
						MENU_FONT | FONT_FULL_WIDTH |
						((pos == item_pos) ? 0 : FONT_INVERT));
			}
#ifdef I2C_ERC12864_ADDR
			badge_erc12864_write(badge_fb);
#endif
#ifdef PIN_NUM_EPD_CLK
			badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(BADGE_EINK_LUT_NORMAL) );
#endif
			need_redraw = false;
		}

		/* handle input */
		uint32_t button_id;
		if ((button_id = badge_input_get_event(-1)) != 0)
		{

			if (button_id == BADGE_BUTTON_B) {
				ets_printf("Button B handling\n");
				return;
			}

			if (button_id == BADGE_BUTTON_START) {
				ets_printf("Selected '%s'\n", itemlist[item_pos].title);
				if (itemlist[item_pos].handler != NULL) {
					itemlist[item_pos].handler();
				}

				// reset screen
				memset(badge_fb, 0xff, BADGE_FB_WIDTH * BADGE_FB_HEIGHT / 8);
#ifdef I2C_ERC12864_ADDR
			badge_erc12864_write(badge_fb);
#endif
#ifdef PIN_NUM_EPD_CLK
				badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(BADGE_EINK_LUT_NORMAL) | DISPLAY_FLAG_FULL_UPDATE);
#endif

				need_redraw = true;
				ets_printf("Button START handled\n");
				continue;
			}

			if (button_id == BADGE_BUTTON_UP) {
				if (item_pos > 0) {
					item_pos--;
					if (scroll_pos > item_pos)
						scroll_pos = item_pos;
					need_redraw = true;
				}
				ets_printf("Button UP handled\n");
			}

			if (button_id == BADGE_BUTTON_DOWN) {
				if (item_pos + 1 < num_items) {
					item_pos++;
					if (scroll_pos + 6 < item_pos)
						scroll_pos = item_pos - 6;
					need_redraw = true;
				}
				ets_printf("Button DOWN handled\n");
			}
		}
	}
}

// pictures
#define NUM_PICTURES 5
const uint8_t *pictures[NUM_PICTURES] = {
#ifdef PIN_NUM_EPD_CLK
	imgv2_sha,
	imgv2_menu,
	imgv2_nick,
	imgv2_weather,
	imgv2_test,
#endif
#ifdef I2C_ERC12864_ADDR
	imgv2_disobey,
	imgv2_woezelenpip,
#endif
};

void
display_picture(int picture_id, int selected_lut)
{
#ifdef I2C_ERC12864_ADDR
	memcpy(badge_fb, pictures[picture_id], BADGE_FB_WIDTH*BADGE_FB_HEIGHT);
	ets_printf("Showing picture %d\n", picture_id);
#else
	memcpy(badge_fb, pictures[picture_id], BADGE_FB_WIDTH*BADGE_FB_HEIGHT/8);
	char str[30];
	if (selected_lut == -1)
		sprintf(str, "[ pic %d, full update ]", picture_id);
	else
		sprintf(str, "[ pic %d, lut %d ]", picture_id, selected_lut);
	draw_font(badge_fb, 8, 4, BADGE_FB_WIDTH, str, FONT_INVERT);
#endif

#ifdef I2C_ERC12864_ADDR
	badge_erc12864_write(badge_fb);
#endif
#ifdef PIN_NUM_EPD_CLK
	badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(selected_lut));
#endif
}

void
app_main(void) {
	badge_check_first_run();
	badge_init();

#ifdef I2C_ERC12864_ADDR
	badge_disobey_samd_write_backlight(255);
#endif

	esp_err_t err = badge_fb_init();
	assert( err == ESP_OK );

  int picture_id = 0;
#if 0
	// simple test-mode
	ets_printf("start drawing image\n");
	display_picture(picture_id, -1);
	ets_printf("done drawing image\n");

	while (1) {
		display_picture(picture_id, -1);
		if (picture_id + 1 < NUM_PICTURES) {
			picture_id++;
		} else {
			picture_id=0;
		}
		ets_delay_us(5000000);
	}
#else
  display_picture(picture_id, -1);
  int selected_lut = BADGE_EINK_LUT_NORMAL;

  while (1) {
    uint32_t button_id;
	if ((button_id = badge_input_get_event(-1)) != 0)
	{
      if (button_id == BADGE_BUTTON_B) {
        ets_printf("Button B handling\n");
        /* redraw with default LUT */
		display_picture(picture_id, -1);
      }
      if (button_id == BADGE_BUTTON_START) {
        ets_printf("Button START handling\n");
        /* open menu */
        displayMenu("Demo menu", demoMenu);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_UP) {
        ets_printf("Button UP handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + 1) % (BADGE_EINK_LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_DOWN) {
        ets_printf("Button DOWN handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + BADGE_EINK_LUT_MAX) % (BADGE_EINK_LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_LEFT) {
        ets_printf("Button LEFT handling\n");
        /* previous picture */
        if (picture_id > 0) {
          picture_id--;
		  display_picture(picture_id, selected_lut);
        }
      }
      if (button_id == BADGE_BUTTON_RIGHT) {
        ets_printf("Button RIGHT handling\n");
        /* next picture */
        if (picture_id + 1 < NUM_PICTURES) {
          picture_id++;
		  display_picture(picture_id, selected_lut);
        }
      }
    }
  }
#endif
}

//void vPortCleanUpTCB ( void *pxTCB ) {
	// place clean up code here
//}
