/** @file badge_fb.h */
#ifndef BADGE_FB_H
#define BADGE_FB_H

#include <stdint.h>
#include <esp_err.h>

#include "badge_pins.h"
#include "badge_eink.h"
#include "badge_erc12864.h"

__BEGIN_DECLS

/** the size of the badge framebuffer */
#ifdef I2C_ERC12864_ADDR
	#define BADGE_FB_WIDTH  BADGE_ERC12864_WIDTH
	#define BADGE_FB_HEIGHT BADGE_ERC12864_HEIGHT
#else
	#define BADGE_FB_WIDTH BADGE_EINK_WIDTH
	#define BADGE_FB_HEIGHT BADGE_EINK_HEIGHT
#endif

#define BADGE_FB_LEN (BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT) //Hack, because the E-INK buffer is larger and not all code has been adapted yet

/** The frame-buffer */
extern uint8_t *badge_fb;

/**
 * Initialize badge framebuffer.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_fb_init(void);

void draw_pixel(uint8_t *buf, int x, int y, int c);
void draw_pixel8b(uint8_t *buf, int x, int y, int c);

__END_DECLS

#endif // BADGE_FB_H
