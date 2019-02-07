/** @file badge_spi.h */
#ifndef BADGE_SPI_H
#define BADGE_SPI_H

#include <stdint.h>
#include <esp_err.h>

__BEGIN_DECLS

esp_err_t (*badge_vspi_release)(void);

/** initialize spi sharing
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_vspi_init(void);

/** force other device to release the vspi
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_vspi_release_and_claim(esp_err_t (*release)(void));

extern esp_err_t badge_vspi_freed(void);

__END_DECLS

#endif // BADGE_SPI_H
