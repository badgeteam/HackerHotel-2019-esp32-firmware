#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <esp_err.h>

#include "badge_pins.h"
#include "badge_spi.h"

/* NOTE:
 *  current implementation doesn't use locking. We are assuming that all calls
 *  are done from the same thread.
 */

esp_err_t (*badge_vspi_release)(void) = NULL;

/** initialize spi sharing
 * @return ESP_OK on success; any other value indicates an error
 */
esp_err_t
badge_vspi_init(void)
{
	// TODO: create mutex

	return ESP_OK;
}

/** force other device to release the vspi
 * @return ESP_OK on success; any other value indicates an error
 */
esp_err_t
badge_vspi_release_and_claim(esp_err_t (*release)(void))
{
	// TODO: grab lock

	esp_err_t res;
	if (badge_vspi_release == NULL) {
		badge_vspi_release = release;
		res = ESP_OK;

	} else {
		res = badge_vspi_release();
		if (badge_vspi_release == NULL && res == ESP_OK) {
			badge_vspi_release = release;

		} else if (res == ESP_OK) {
			res = ESP_FAIL;
		}
	}

	// TODO: release lock

	return res;
}

esp_err_t badge_vspi_freed(void)
{
	// TODO: check if mutex is claimed

	badge_vspi_release = NULL;

	return ESP_OK;
}
