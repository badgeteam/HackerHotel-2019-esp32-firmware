# Power management

def reboot():
	try:
		import badge
		badge.eink_busy_wait()
	except:
		pass
	import machine
	machine.deepsleep(2)

def sleep(duration=0, status=False):
	import machine, time
	machine.RTC().wake_on_ext0(pin = machine.Pin(25), level = 0)
	machine.RTC().wake_on_ext1([machine.Pin(12, machine.Pin.IN, machine.Pin.PULL_UP)], 0)
	if (duration >= 86400000): #One day
		duration = 0
	if status:
		import term
		if duration < 1:
			term.header(True, "Sleeping until touchbutton is pressed...")
		else:
			term.header(True, "Sleeping for "+str(duration)+"ms...")
	time.sleep(0.05)
	try:
		import badge
		badge.eink_busy_wait()
	except:
		pass
	machine.deepsleep(duration)

def isColdBoot():
	import machine
	if machine.wake_reason() == (7, 0):
		return True
	return False

def isWakeup(fromTimer=True,fromButton=True, fromIr=True, fromUlp=True):
	import machine
	if fromButton and machine.wake_reason() == (3, 1):
		return True
	if fromIr     and machine.wake_reason() == (3, 2):
		return True
	if fromTimer  and machine.wake_reason() == (3, 4):
		return True
	if fromUlp    and machine.wake_reason() == (3, 5):
		return True
	return False

# Application launching

def start(app, status=False):
	import esp
	if status:
		import term, easydraw
		if app == "" or app == "launcher":
			term.header(True, "Loading menu...")
			easydraw.messageCentered("PLEASE WAIT\nStarting the menu...", True, "/media/busy.png")
		else:
			term.header(True, "Loading application "+app+"...")
			easydraw.messageCentered("PLEASE WAIT\nStarting '"+app+"'...", True, "/media/busy.png")
	esp.rtcmem_write_string(app)
	reboot()

def home(status=False):
	start("", status)

def launcher(status=False):
	start("launcher", status)

def shell(status=False):
	start("shell", status)

# Over-the-air updating

def ota(status=False):
	import esp, deepsleep
	if status:
		import term, easydraw
		term.header(True, "Starting update...")
		easydraw.messageCentered("PLEASE WAIT\nStarting update...", True, "/media/busy.png")
	esp.rtcmem_write(0,1)
	esp.rtcmem_write(1,254)
	reboot()

def serialWarning():
	import easydraw
	easydraw.messageCentered("NOTICE\n\nThis app can only be controlled using the USB-serial connection.", True, "/media/crown.png")
