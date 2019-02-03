import ugfx, time, badge, machine, deepsleep, gc
import appglue, virtualtimers
import easydraw, easywifi, easyrtc

import tasks.powermanagement as pm
import tasks.otacheck as otac
import tasks.services as services
import term, term_menu

# Graphics

def disobeyLogo(x,y):
	badge.eink_png(x,y,b'\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00 \x00\x00\x00 \x01\x03\x00\x00\x00I\xb4\xe8\xb7\x00\x00\x00\x06PLTE\xff\xff\xff\x00\x00\x00U\xc2\xd3~\x00\x00\x00\tpHYs\x00\x00\x0e\xc4\x00\x00\x0e\xc4\x01\x95+\x0e\x1b\x00\x00\x00{IDAT\x08\x99M\x8e1\n\x840\x14D\x9f\x04\xb4\x91\xb8\x07\x08\xec\x15,\x7f\xe5^e!\x17\xc8\xf6\x82\x9e\xc03\xa5\xca9\x02[\xd8ZZH\xd8\x984\xcb\x0c\xaf\x9cy0\x1c\xc0\x92\xa09\x9f+\xca+O\x07\x11=\x1c\x0e\xf3\xba\x04\x99\'\xc3\x18\xbb\x9e\x8fW\x1bvm\x02\x16\x02;\xb9\x01\xbe7\xec\x1f<\xa8\x8aH^{C[\xe1$iFI=\xf2\xc0\xe4 hp\xf5\xb2\x9c\x17\x8d"t\xab\xfd\x00\x7f\x18%\\\x904\xdd\x82\x00\x00\x00\x00IEND\xaeB`\x82')

def ledAnimationDisplay(i):
	if i > 5:
		i = 5-i
	for j in range(6):
		if j==i:
			badge.led(j, 255, 0, 0)
		else:
			if j==5-i:
				badge.led(j, 0, 255, 0)
			else:
				badge.led(j, 0, 0, 0)
	
ledAnimationStep=0
ledAnimationCount=0

def ledAnimationTask():
    global ledAnimationStep
    global ledAnimationCount
    ledAnimationStep +=1
    if ledAnimationStep<11:
        ledAnimationCount+=1
        ledAnimationDisplay(ledAnimationStep)
        return 100
    elif ledAnimationCount<4:
        ledAnimationStep=0
        for j in range(6):
            badge.led(j, 0, 0, 0)
        return 100
    else:
        for j in range(6):
            badge.led(j, 0, 0, 0)
        ledAnimationStep=0
        return 600000

def draw(mode, goingToSleep=False):
	info1 = ''
	info2 = ''
	if mode:
		# We flush the buffer and wait
		ugfx.flush(ugfx.GREYSCALE)
	else:
		# We prepare the screen refresh
		ugfx.clear(ugfx.WHITE)
		easydraw.nickname()
		if goingToSleep:
			info = 'Sleeping...'
		elif badge.safe_mode():
			info = "(Services disabled!)"
		elif otac.available(False):
			info = 'Update available!'
		else:
			info = ''
		easydraw.disp_string_right_bottom(0, info)
		disobeyLogo(0, ugfx.height()-32)

# Button input

def splash_input_start(pressed):
	# Pressing start always starts the launcher
	if pressed:
		appglue.start_app("launcher", False)

def splash_input_other(pressed):
	if pressed:
		pm.feed()

def splash_input_init():
	print("[SPLASH] Inputs attached")
	ugfx.input_init()
	ugfx.input_attach(ugfx.BTN_START, splash_input_start)
	ugfx.input_attach(ugfx.BTN_B, splash_input_other)
	ugfx.input_attach(ugfx.JOY_UP, splash_input_other)
	ugfx.input_attach(ugfx.JOY_DOWN, splash_input_other)
	ugfx.input_attach(ugfx.JOY_LEFT, splash_input_other)
	ugfx.input_attach(ugfx.JOY_RIGHT, splash_input_other)

# Power management

def goToSleep():
	onSleep(virtualtimers.idle_time())

def onSleep(idleTime):
	draw(False, True)
	services.force_draw(True)
	draw(True, True)

### PROGRAM

badge.backlight(255)

splash_input_init()

# post ota script
import post_ota

#setupState = badge.nvs_get_u8('badge', 'setup.state', 0)
#if setupState < 3: #First boot (3 for SHA compat)
#	print("[SPLASH] Force ota check...")
#	badge.nvs_set_u8('badge', 'setup.state', 3)
#	if not easywifi.failure():
#		otac.available(True)
#else: # Normal boot
#	print("[SPLASH] Normal boot... ("+str(machine.reset_cause())+")")
#	#if (machine.reset_cause() != machine.DEEPSLEEP_RESET):
#	#	print("... from reset: checking for ota update")
#	#	if not easywifi.failure():
#	#		otac.available(True)

# RTC ===
#if time.time() < 1482192000:
#	easyrtc.configure()
# =======

if badge.safe_mode():
	draw(False)
	services.force_draw()
	draw(True)
else:
	have_services = services.setup(draw) # Start services
	if not have_services:
		draw(False)
		services.force_draw()
		draw(True)

easywifi.disable()
gc.collect()

virtualtimers.activate(25)
pm.callback(onSleep)
pm.feed()

virtualtimers.new(10, ledAnimationTask)

umenu = term_menu.UartMenu(goToSleep, pm, badge.safe_mode())
umenu.main()
