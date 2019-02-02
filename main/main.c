#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "uart.h"
#include <font.h>
#include <string.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_button.h>
#include <badge_eink.h>
#include <badge_disobey_samd.h>
#include <badge_fb.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_first_run.h>
#include <badge_ota.h>
#include "bpp_init.h"
#include "rom/rtc.h"
#include "esprtcmem.h"

extern void micropython_entry(void);
extern uint32_t reset_cause;
extern bool in_safe_mode;

#define BUTTON_SAFE_MODE ((1 << BADGE_BUTTON_START))

void do_bpp_bgnd() {
    // Kick off bpp
    bpp_init();

    printf("Bpp inited.\n");

    // immediately abort and reboot when touchpad detects something
    while (badge_input_get_event(1000) == 0) { }

    printf("Touch detected. Exiting bpp, rebooting.\n");
    esp_restart();
}

void app_main(void) {
	badge_check_first_run();
	badge_init();

/*#ifdef I2C_ERC12864_ADDR
	badge_disobey_samd_write_backlight(255);
#endif*/

	/*esp_err_t err = badge_fb_init();
	assert( err == ESP_OK );*/
	
	uint8_t magic = esp_rtcmem_read(0);
	uint8_t inv_magic = esp_rtcmem_read(1);

	if (magic == (uint8_t)~inv_magic) {
		printf("Magic checked out!\n");
		switch (magic) {
			case 1:
				printf("Starting OTA\n");
				badge_ota_update();
				break;

#ifdef CONFIG_SHA_BPP_ENABLE
			case 2:
				badge_init();
				if (badge_input_button_state == 0) {
					printf("Starting bpp.\n");
					do_bpp_bgnd();
				} else {
					printf("Touch wake after bpp.\n");
					micropython_entry();
				}
				break;
#endif

			case 3:
				badge_first_run();
				break;
			
			default:
				printf("Unsupported magic '%d'!\n", magic);
				micropython_entry();
		}

	} else {
		uint32_t reset_cause = rtc_get_reset_reason(0);
		if (reset_cause != DEEPSLEEP_RESET) {
			badge_init();
			if ((badge_input_button_state & BUTTON_SAFE_MODE) == BUTTON_SAFE_MODE) {
				in_safe_mode = true;
			}
		}
		micropython_entry();
	}
}

//void vPortCleanUpTCB ( void *pxTCB ) {
	// place clean up code here
//}
