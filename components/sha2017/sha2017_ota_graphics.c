#include "sdkconfig.h"
#include <gfx.h>
#include "sha2017_ota_graphics.h"
#include "badge_eink.h"
#include "badge_eink_dev.h"

font_t fontPercentage;
font_t fontTitle;
font_t fontStatus;

void sha2017_ota_percentage_init() {
  gfxInit();
#ifdef CONFIG_DISOBEY
  fontTitle = gdispOpenFont("Roboto_Regular18");
  fontStatus = gdispOpenFont("Roboto_Regular12");
  fontPercentage = gdispOpenFont("Roboto_Regular18");
#else
  fontTitle = gdispOpenFont("Roboto_BlackItalic24");
  fontStatus = gdispOpenFont("PermanentMarker22");
  fontPercentage = gdispOpenFont("PermanentMarker36");
#endif

	target_lut = BADGE_EINK_LUT_FASTER;
}

#ifdef CONFIG_DISOBEY
void show_percentage(char *name, uint8_t percentage, bool show_percentage, bool force) {
  if ((!force)&&(badge_eink_dev_is_busy())) return;

  color_t front = Black;
  color_t back = White;

  gdispClear(back);
  gdispDrawString(0, 0, "OTA UPDATE", fontTitle, front);
  gdispDrawString(0, 20, name, fontStatus, front);
  if (show_percentage) {
    char perc[10];
    sprintf(perc, "Progress: %d%%", percentage);
    gdispDrawString(0, 40, perc, fontPercentage, front);
  }
  gdispFlush();
}
#else
void show_percentage(char *name, uint8_t percentage, bool show_percentage, bool force) {
  if ((!force)&&(badge_eink_dev_is_busy())) return;

  color_t front = White;
  color_t back = Black;

  gdispClear(back);

  if (show_percentage) {
    target_lut = 2;
    char perc[10];
    sprintf(perc, "%d%%", percentage);
    gdispDrawString(30, 45, perc, fontPercentage, front);
  } else {
    target_lut = 0;
  }

  gdispDrawString(show_percentage ? 150 : 60, 25, "STILL", fontTitle, front);
  gdispDrawString(show_percentage ? 130 : 40, 46, name, fontStatus, front);
  // underline:
  gdispDrawLine(show_percentage ? 130 : 40, 72,
                (show_percentage ? 130 : 40) + gdispGetStringWidth(name, fontStatus) + 14, 72,
                front);
  // cursor:
  gdispDrawLine((show_percentage ? 130 : 40) + gdispGetStringWidth(name, fontStatus) + 10, 50 + 2,
                (show_percentage ? 130 : 40) + gdispGetStringWidth(name, fontStatus) + 10, 50 + 22 - 2,
                front);
  gdispDrawString(140, 75, "Anyway", fontTitle, front);
  gdispFlush();
}
#endif
