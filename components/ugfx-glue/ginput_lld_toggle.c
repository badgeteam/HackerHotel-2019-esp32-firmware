/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include <sdkconfig.h>

#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_TOGGLE)
#include "../../../src/ginput/ginput_driver_toggle.h"

#include <badge_input.h>
#include <badge_button.h>

#include "ginput_lld_toggle_config.h"
#include "gdisp/gdisp.h"

GINPUT_TOGGLE_DECLARE_STRUCTURE();

void
ginput_lld_toggle_init(const GToggleConfig *ptc)
{
#ifdef CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG
	ets_printf("ginput_lld_toggle: init()\n");
#endif // CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG

	badge_input_notify = ginputToggleWakeupI;
}

extern bool ugfx_screen_flipped;

unsigned
ginput_lld_toggle_getbits(const GToggleConfig *ptc)
{
#ifdef CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG
	ets_printf("ginput_lld_toggle: getbits()\n");
#endif // CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG

	// drain input queue
	while (badge_input_get_event(0) != 0);

	// rotate UP, DOWN, LEFT, RIGHT
	uint32_t button_state = badge_input_button_state;
	if (GDISP != NULL) {
		unsigned int rotate;
		switch(gdispGetOrientation()) {
		case GDISP_ROTATE_0:
		default:
			rotate = 0;
			break;
		case GDISP_ROTATE_90:
			rotate = 1;
			break;
		case GDISP_ROTATE_180:
			rotate = 2;
			break;
		case GDISP_ROTATE_270:
			rotate = 3;
			break;
		}

		if (ugfx_screen_flipped) rotate ^= 2;

		uint32_t button_tmp = button_state;
		button_state &= ~ ((1 << BADGE_BUTTON_LEFT)|(1 << BADGE_BUTTON_UP)|(1 << BADGE_BUTTON_RIGHT)|(1 << BADGE_BUTTON_DOWN));
		const unsigned int buttons[4] = { BADGE_BUTTON_LEFT, BADGE_BUTTON_UP, BADGE_BUTTON_RIGHT, BADGE_BUTTON_DOWN };
		unsigned int i;
		for (i=0; i<4; i++) {
			if (button_tmp & (1 << buttons[i])) {
				button_state |= 1 << buttons[(i + rotate)&3];
			}
		}
	}

	// pass on button state
	return button_state;
}

#endif /*  GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */
