#ifndef DISPLAY_ERC12864_H
#define DISPLAY_ERC12864_H

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>
#include "display.h"

#ifdef DISPLAY_EINK_ERC12864

/* the number of horizontal pixels */
#define DISP_SIZE_X 128

/* the number of vertical pixels */
#define DISP_SIZE_Y 64

/* the number of bits per pixel */
#define DISP_BITS_PER_PIXEL 1

esp_err_t display_erc12864_init(void);
esp_err_t display_erc12864_write(const uint8_t *buffer);

#endif // DISPLAY_EINK_ERC12864

#endif // DISPLAY_ERC12864_H
