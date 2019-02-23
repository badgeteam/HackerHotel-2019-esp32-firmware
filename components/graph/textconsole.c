#include <textconsole.h>

// can use the lower <n> lines for showing measurements
void disp_line(const char *line, int flags)
{
	printf("# %s", line);
	static int next_line = 0;
	while (1)
	{
		int height = (flags & FONT_16PX) ? 2 : 1;
		while (next_line >= NUM_DISP_LINES - height)
		{ // scroll up
			next_line--;
			memmove(badge_fb, &badge_fb[BADGE_FB_WIDTH], (NUM_DISP_LINES-1)*BADGE_FB_WIDTH);
			memset(&badge_fb[(NUM_DISP_LINES-1)*BADGE_FB_WIDTH], COLOR_WHITE, BADGE_FB_WIDTH);
		}
		int len = draw_font(badge_fb, 0, 8*next_line, BADGE_FB_WIDTH, line, (FONT_FULL_WIDTH|FONT_INVERT)^flags);
		if (height == 2)
			next_line++;
		if ((flags & NO_NEWLINE) == 0)
		{
			next_line++;
			draw_font(badge_fb, 0, 8*next_line, BADGE_FB_WIDTH, "_", FONT_FULL_WIDTH|FONT_INVERT);
		}

		if (len == 0 || line[len] == 0)
		{
#ifdef I2C_ERC12864_ADDR
		badge_erc12864_write(badge_fb);
#endif
#ifdef PIN_NUM_EPD_CLK
		badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(2));
#endif
			return;
		}

		line = &line[len];
	}
}

bool load_png(int x, int y, const char *filename)
{
	struct lib_file_reader *fr = lib_file_new(filename, 1024);
	if (fr == NULL)
	{
		fprintf(stderr, "file not found (or out of memory).\n");
		return false;
	}

	struct lib_png_reader *pr = lib_png_new((lib_reader_read_t) &lib_file_read, fr);
	if (pr == NULL)
	{
		fprintf(stderr, "out of memory.\n");
		lib_file_destroy(fr);
		return false;
	}

	int res = lib_png_load_image(pr, &badge_fb[x + y*BADGE_FB_WIDTH], 0, 0, BADGE_FB_WIDTH-x, BADGE_FB_HEIGHT-y, BADGE_FB_WIDTH);
	lib_png_destroy(pr);
	lib_file_destroy(fr);

	if (res < 0)
	{
		fprintf(stderr, "failed to load image: res = %i\n", res);
		return false;
	}
	return true;
}

void console_init(void)
{
#ifdef CONFIG_DISOBEY
	esp_err_t err = badge_i2c_init();
	assert( err == ESP_OK );
	err = badge_disobey_samd_init();
	assert( err == ESP_OK );
	err = badge_erc12864_init();
	err = badge_fb_init();
	assert( err == ESP_OK );
	memset(badge_fb, 0x00, BADGE_FB_LEN);
	badge_disobey_samd_write_backlight(255);
	badge_erc12864_write(badge_fb);
#else
	esp_err_t err = badge_eink_init(BADGE_EINK_DEFAULT);
	assert( err == ESP_OK );
	err = badge_fb_init();
	assert( err == ESP_OK );
	// start with white screen
	memset(badge_fb, 0xff, BADGE_FB_LEN);
	badge_eink_display(badge_fb, DISPLAY_FLAG_LUT(0));
#endif
}
