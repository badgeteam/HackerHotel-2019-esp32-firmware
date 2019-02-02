# File: splash.py
# Version: 6
# Description: Homescreen for SHA2017 badge
# License: MIT
# Authors: Renze Nicolai <renze@rnplus.nl>
#          Thomas Roos   <?>

import ugfx, time, badge, machine, deepsleep, gc
import appglue, virtualtimers
import easydraw, easywifi, easyrtc

import tasks.powermanagement as pm
#import tasks.otacheck as otac
#import tasks.resourcescheck as resc
#import tasks.sponsorscheck as spoc
import tasks.services as services

# Graphics

def draw(mode, goingToSleep=False):
	if mode:
		# We flush the buffer and wait
		ugfx.flush(ugfx.GREYSCALE)
	else:
		# We prepare the screen refresh
		ugfx.clear(ugfx.WHITE)
	if goingToSleep:
		info1 = 'Sleeping...'
		info2 = 'Press any key to wake up'
	else:
		info1 = 'Press start to open the launcher'
		info2 = ''
		#if otac.available(False):
		#	info2 = 'Press select to start OTA update'
		#else:
		#	info2 = ''

	def disp_string_right(y, s):
		l = ugfx.get_string_width(s,"Roboto_Regular12")
		ugfx.string(296-l, y, s, "Roboto_Regular12",ugfx.BLACK)

	disp_string_right(0, info1)
	disp_string_right(12, info2)

	if badge.safe_mode():
		disp_string_right(92, "Safe Mode - services disabled")
		disp_string_right(104, "Sleep disabled - will drain battery quickly")
		disp_string_right(116, "Press Reset button to exit")
		
	easydraw.nickname()
	
	on_usb = pm.usb_attached()
	vBatt = badge.battery_volt_sense()
	vBatt += vDrop
	charging = badge.battery_charge_status()

	easydraw.battery(on_usb, vBatt, charging)
	
	if vBatt>500:
		ugfx.string(52, 0, str(round(vBatt/1000, 1)) + 'v','Roboto_Regular12',ugfx.BLACK)


# Button input

def splash_input_start(pressed):
	print("Start", pressed)
	# Pressing start always starts the launcher
	if pressed:
		appglue.start_app("launcher", False)

def splash_input_a(pressed):
	print("A", pressed)
	if pressed:
		pm.feed()

def splash_input_select(pressed):
	print("Select", pressed)
	if pressed:
		#if otac.available(False):
		#	appglue.start_ota()
		pm.feed()

def splash_input_other(pressed):
	print("Other", pressed)
	if pressed:
		pm.feed()

def splash_input_init():
	print("[SPLASH] Inputs attached")
	ugfx.input_init()
	ugfx.input_attach(ugfx.BTN_START, splash_input_start)
	ugfx.input_attach(ugfx.BTN_A, splash_input_a)
	ugfx.input_attach(ugfx.BTN_B, splash_input_other)
	ugfx.input_attach(ugfx.BTN_SELECT, splash_input_select)
	ugfx.input_attach(ugfx.JOY_UP, splash_input_other)
	ugfx.input_attach(ugfx.JOY_DOWN, splash_input_other)
	ugfx.input_attach(ugfx.JOY_LEFT, splash_input_other)
	ugfx.input_attach(ugfx.JOY_RIGHT, splash_input_other)

# Power management

def onSleep(idleTime):
	draw(False, True)
	services.force_draw(True)
	draw(True, True)

### PROGRAM

# Calibrate battery voltage drop
if badge.battery_charge_status() == False and pm.usb_attached() and badge.battery_volt_sense() > 2500:
	badge.nvs_set_u16('splash', 'bat.volt.drop', 5200 - badge.battery_volt_sense()) # mV
	print('Set vDrop to: ' + str(4200 - badge.battery_volt_sense()))
vDrop = badge.nvs_get_u16('splash', 'bat.volt.drop', 1000) - 1000 # mV

splash_input_init()

# post ota script
import post_ota

setupState = badge.nvs_get_u8('badge', 'setup.state', 0)
if setupState == 0: #First boot
	print("[SPLASH] First boot (start setup)...")
	appglue.start_app("setup")
#elif setupState == 1: # Second boot: Show sponsors
#	print("[SPLASH] Second boot (show sponsors)...")
#	badge.nvs_set_u8('badge', 'setup.state', 2)
#	spoc.show(True)
#elif setupState == 2: # Third boot: force OTA check
#	print("[SPLASH] Third boot (force ota check)...")
#	badge.nvs_set_u8('badge', 'setup.state', 3)
#	if not easywifi.failure():
#		otac.available(True)
#else: # Normal boot
#    print("[SPLASH] Normal boot... ("+str(machine.reset_cause())+")")
#    if (machine.reset_cause() != machine.DEEPSLEEP_RESET):
#        print("... from reset: checking for ota update")
#        if not easywifi.failure():
#            otac.available(True)

# RTC ===
if time.time() < 1482192000:
	easyrtc.configure()
# =======
		
#if not easywifi.failure():
#	resc.check()        # Check resources
#if not easywifi.failure():
#	spoc.show(False)    # Check sponsors

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

print("----")
print("WARNING: POWER MANAGEMENT ACTIVE")
print("To use shell type 'import shell' within 5 seconds.")
print("----")
