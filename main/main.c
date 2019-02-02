#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <font.h>
#include <string.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_eink.h>
#include <badge_disobey_samd.h>
#include <badge_fb.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_first_run.h>
#include <badge_ota.h>

extern void micropython_entry(void);

void app_main(void) {
	badge_check_first_run();
	badge_init();

#ifdef I2C_ERC12864_ADDR
	badge_disobey_samd_write_backlight(255);
#endif

	esp_err_t err = badge_fb_init();
	assert( err == ESP_OK );
	
	micropython_entry();
}

//void vPortCleanUpTCB ( void *pxTCB ) {
	// place clean up code here
//}
