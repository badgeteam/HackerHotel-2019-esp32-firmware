#ifndef DISPLAY_FB_H
#define DISPLAY_FB_H

#include <stdint.h>
#include <esp_err.h>

#include "badge_pins.h"
#include "badge_eink.h"
#include "badge_erc12864.h"

__BEGIN_DECLS

/** the size of the badge framebuffer */
#ifdef I2C_ERC12864_ADDR
	#define DISPLAY_FB_WIDTH  BADGE_ERC12864_WIDTH
	#define DISPLAY_FB_HEIGHT BADGE_ERC12864_HEIGHT
#else
	#define DISPLAY_FB_WIDTH BADGE_EINK_WIDTH
	#define DISPLAY_FB_HEIGHT BADGE_EINK_HEIGHT
#endif

#define DISPLAY_FB_LEN (BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT) //Hack, because the E-INK buffer is larger and not all code has been adapted yet

/** The frame-buffer */
extern uint8_t *badge_fb;

/**
 * Initialize badge framebuffer.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_fb_init(void);

void draw_pixel(uint8_t *buf, int x, int y, uint8_t c);
void draw_pixel_8b(uint8_t *buf, int x, int y, uint8_t c);

__END_DECLS

#endif // DISPLAY_FB_H
