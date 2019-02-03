# This file is executed on every boot (including wake-boot from deepsleep)
import badge, machine, esp, ugfx, sys, time
badge.init()
ugfx.init()

esp.rtcmem_write(0,0)
esp.rtcmem_write(1,0)

# setup timezone
#timezone = badge.nvs_get_str('system', 'timezone', 'CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00')
#time.settimezone(timezone)

if badge.safe_mode():
	splash = 'splash'
else:
	splash = badge.nvs_get_str('boot','splash','splash')

if machine.wake_reason() == (7, 0):
	print('[BOOT] Cold boot')
else:
	if machine.wake_reason() == (3, 4):
		print("[BOOT] Wake from sleep (timer)")
	elif machine.wake_reason() == (3, 1):
		print("[BOOT] Wake from sleep (button)")
	else:
		(reset_cause, wake_reason) = machine.wake_reason()
		(reset_cause_desc, wake_reason_desc) = machine.wake_description()
		print("[BOOT] "+str(reset_cause)+": "+reset_cause_desc+", "+str(wake_reason)+": "+wake_reason_desc)
	load_me = esp.rtcmem_read_string()
	if load_me:
		splash = load_me
		print("starting %s" % load_me)
		esp.rtcmem_write_string("")

try:
	if not splash=="shell":
		if splash.startswith('bpp '):
			splash = splash[4:len(splash)]
			badge.mount_bpp()
		elif splash.startswith('sdcard '):
			splash = splash[7:len(splash)]
			badge.mount_sdcard()
		__import__(splash)
	else:
		ugfx.clear(ugfx.WHITE)
		ugfx.flush(ugfx.LUT_FULL)
except BaseException as e:
	sys.print_exception(e)
	import easydraw
	easydraw.msg("","Fatal error", True)

	# if we started the splash screen and it is not the default splash screen,
	# then revert to original splash screen.
	if splash == badge.nvs_get_str('boot', 'splash', 'splash') and splash != 'splash':
		easydraw.msg("Disabling custom splash screen.")
		easydraw.msg("")
		badge.nvs_erase_key('boot', 'splash')

	easydraw.msg("Guru meditation:")
	easydraw.msg(str(e))
	easydraw.msg("")
	easydraw.msg("Rebooting in 5 seconds...")
	import time
	time.sleep(5)
	import appglue
	appglue.home()
