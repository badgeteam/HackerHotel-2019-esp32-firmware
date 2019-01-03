/** @file badge_erc12864.h */
#ifndef BADGE_ERC12864_H
#define BADGE_ERC12864_H

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>

#define BADGE_ERC12864_WIDTH  128
#define BADGE_ERC12864_HEIGHT 64

#define BADGE_ERC12864_DATA_LENGTH (BADGE_ERC12864_WIDTH * BADGE_ERC12864_HEIGHT) / 8

__BEGIN_DECLS

extern esp_err_t badge_erc12864_init(void);
extern esp_err_t badge_erc12864_write(const uint8_t *data);
extern void badge_erc12864_set_rotation(bool newFlip);

__END_DECLS

#endif // BADGE_ERC12864_H
