#include <sdkconfig.h>

#include <string.h>
#include <fcntl.h>

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <esp_partition.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <wear_levelling.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_mpr121.h>
#include <badge_eink.h>
#include <badge_fb.h>
#include <badge_leds.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_power.h>
#include <badge_sdcard.h>
#include <badge_i2c.h>
#include <badge_disobey_samd.h>
#include <badge_erc12864.h>
#include <badge_ota.h>

#include <file_reader.h>
#include <flash_reader.h>
#include <deflate_reader.h>
#include <png_reader.h>

#include <font.h>

#ifndef TEXTCONSOLE_H
#define TEXTCONSOLE_H

#ifdef I2C_ERC12864_ADDR
	#define NUM_DISP_LINES 8
	#define COLOR_WHITE 0xFF
	#define COLOR_BLACH 0x00
#else
	#define NUM_DISP_LINES 12
	#define COLOR_WHITE 0x00
	#define COLOR_BLACH 0xFF
#endif

#define NO_NEWLINE 0x80

void disp_line(const char *line, int flags);
bool load_png(int x, int y, const char *filename);
void console_init(void);

#endif
