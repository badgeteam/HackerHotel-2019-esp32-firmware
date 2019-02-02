/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * SHA2017 Badge Firmware https://wiki.sha2017.org/w/Projects:Badge/MicroPython
 *
 * Based on work by EMF for their TiLDA MK3 badge
 * https://github.com/emfcamp/micropython/tree/tilda-master/stmhal
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
 * Copyright (c) 2016 Damien P. George
 * Copyright (c) 2017 Anne Jan Brouwer
 * Copyright (c) 2017 Christian Carlowitz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <badge_eink.h>
#include <badge_button.h>
#include <badge_input.h>


#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "py/nlr.h"
#include "py/runtime.h"

#ifdef UNIX
#include "modfreedomgfx_sdl.h"
#else
#include "modfreedomgfx_eink.h"
#endif

#define imgSize (BADGE_EINK_WIDTH*BADGE_EINK_HEIGHT)
static uint8_t* img = 0;

#define PX(x,y,v) \
	if( x>=0 && y>=0 && x < BADGE_EINK_WIDTH && y < BADGE_EINK_HEIGHT) \
		img[((x)+(y)*BADGE_EINK_WIDTH)] = (v)

#define ABS(x) (((x)<0)?-(x):(x))

static void gfx_input_poll(uint32_t btn);

STATIC mp_obj_t gfx_init(void) {
	img = freedomgfxInit();
	memset(img, 0xff, imgSize);
	freedomgfxDraw();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(gfx_init_obj, gfx_init);

STATIC mp_obj_t gfx_deinit(void) {
	freedomgfxDeinit();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(gfx_deinit_obj, gfx_deinit);

STATIC mp_obj_t gfx_poll(void) {
  uint32_t evt = freedomgfxPoll();
  gfx_input_poll(evt);
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(gfx_poll_obj, gfx_poll);

STATIC mp_obj_t gfx_line(mp_uint_t n_args, const mp_obj_t *args) {
	int x0 = mp_obj_get_int(args[0]);
	int y0 = mp_obj_get_int(args[1]);
	int x1 = mp_obj_get_int(args[2]);
	int y1 = mp_obj_get_int(args[3]);
	int col = mp_obj_get_int(args[4]);

	PX(x0,y0,col);

	// algorithm: https://de.wikipedia.org/wiki/Bresenham-Algorithmus
	int dx =  ABS(x1-x0);
	int sx = x0<x1 ? 1 : -1;
	int dy = -ABS(y1-y0);
	int sy = y0<y1 ? 1 : -1;
	int err = dx+dy;
	int err2;

	while(1){
		PX(x0,y0,col);
		if ((x0 == x1) && (y0 == y1))
			break;
		err2 = 2*err;
		if (err2 > dy)
		{
			err += dy;
			x0 += sx;
		}
		if (err2 < dx)
		{
			err += dx;
			y0 += sy;
		}
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_line_obj, 5, 5, gfx_line);

STATIC mp_obj_t gfx_area(mp_uint_t n_args, const mp_obj_t *args) {
  int x0 = mp_obj_get_int(args[0]);
  int y0 = mp_obj_get_int(args[1]);
  int a = mp_obj_get_int(args[2]);
  int b = mp_obj_get_int(args[3]);
  int col = mp_obj_get_int(args[4]);

  for(int i = 0; i < a; i++)
  {
	  for(int j = 0; j < b; j++)
		  PX(x0+i,y0+j,col);
  }

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_area_obj, 5, 5, gfx_area);


static const uint8_t tm12x6_font[] = {
#include "fonts/Lat2-Terminus12x6.inc"
};

STATIC mp_obj_t gfx_string(mp_uint_t n_args, const mp_obj_t *args) {
  mp_uint_t len;
  const unsigned char *data = (const unsigned char*) mp_obj_str_get_data(args[2], &len);
  int x0 = mp_obj_get_int(args[0]);
  int y0 = mp_obj_get_int(args[1]);
  int col = mp_obj_get_int(args[4]);

  const uint8_t* font = tm12x6_font;
  int clen = ((int*)font)[5];
  int cheight = ((int*)font)[6];
  int cwidth = ((int*)font)[7];

  int xoffs = 0;
  while(*data != 0)
  {
	  for(int i = 0; i < cheight; i++)
	  {
		  for(int j = 0; j < cwidth; j++) // TODO: only works for single byte rows
		  {
			  if( font[4*8+(int)(*data)*clen+i] & (1<<(7-j)))
				  PX(x0+j+xoffs,y0+i,col);
		  }
	  }
	  data++;
	  xoffs += cwidth;

	  if(*data == '\n')
	  {
		  xoffs = 0;
		  y0 += cheight;
		  data++;
	  }
  }

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_string_obj, 5, 5, gfx_string);


STATIC mp_obj_t gfx_flush(mp_uint_t n_args, const mp_obj_t *args) {
	freedomgfxDraw();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_flush_obj, 0, 1, gfx_flush);


// callback system

STATIC bool button_init_done = false;
STATIC mp_obj_t button_callbacks[1+BADGE_BUTTONS];

static void gfx_input_poll(uint32_t btn)
{
	if(button_init_done && btn > 0 && btn <= BADGE_BUTTONS)
	{
		if(button_callbacks[btn] != mp_const_none)
			mp_sched_schedule(button_callbacks[btn], mp_obj_new_bool(0), NULL);
	}
}

STATIC mp_obj_t gfx_input_init(mp_uint_t n_args, const mp_obj_t *args) {
	for(size_t i = 0; i <= BADGE_BUTTONS; i++){
		button_callbacks[i] = mp_const_none;
	}
	button_init_done = true;
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_input_init_obj, 0, 1, gfx_input_init);

STATIC mp_obj_t gfx_input_attach(mp_uint_t n_args, const mp_obj_t *args) {
  int button = mp_obj_get_int(args[0]);
  if(button > 0 && button <= BADGE_BUTTONS)
	  button_callbacks[button] = args[1];
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gfx_input_attach_obj, 2, 2, gfx_input_attach);


// Module globals

STATIC const mp_rom_map_elem_t freedomgfx_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_freedomgfx)},

    {MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&gfx_init_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_deinit), (mp_obj_t)&gfx_deinit_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_poll), (mp_obj_t)&gfx_poll_obj},

    {MP_OBJ_NEW_QSTR(MP_QSTR_line), (mp_obj_t)&gfx_line_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_area), (mp_obj_t)&gfx_area_obj},
	{MP_OBJ_NEW_QSTR(MP_QSTR_string), (mp_obj_t)&gfx_string_obj},
	{MP_OBJ_NEW_QSTR(MP_QSTR_flush), (mp_obj_t)&gfx_flush_obj},

	{MP_OBJ_NEW_QSTR(MP_QSTR_input_init), (mp_obj_t)&gfx_input_init_obj},
	{MP_OBJ_NEW_QSTR(MP_QSTR_input_attach), (mp_obj_t)&gfx_input_attach_obj},

    {MP_OBJ_NEW_QSTR(MP_QSTR_BLACK), MP_OBJ_NEW_SMALL_INT(0x00)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_WHITE), MP_OBJ_NEW_SMALL_INT(0xff)},

    {MP_OBJ_NEW_QSTR(MP_QSTR_JOY_UP), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_UP)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_JOY_DOWN), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_DOWN)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_JOY_LEFT), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_LEFT)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_JOY_RIGHT), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_RIGHT)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BTN_A), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_A)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BTN_B), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_B)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BTN_SELECT), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_SELECT)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BTN_START), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_START)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BTN_FLASH), MP_OBJ_NEW_SMALL_INT(BADGE_BUTTON_FLASH)},

};

STATIC MP_DEFINE_CONST_DICT(freedomgfx_module_globals, freedomgfx_module_globals_table);

const mp_obj_module_t freedomgfx_module = {
    .base = {&mp_type_module}, .globals = (mp_obj_dict_t *)&freedomgfx_module_globals,
};

