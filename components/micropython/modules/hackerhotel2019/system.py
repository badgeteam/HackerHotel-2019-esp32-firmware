# Power management

def reboot():
	import machine
	machine.deepsleep(2)

def sleep(duration=0, status=False):
	import machine, time
	machine.RTC().wake_on_ext0(pin = machine.Pin(25), level = 0)
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

def isWakeup(fromTimer=True,fromButton=True):
	import machine
	if fromButton and machine.wake_reason() == (3, 4):
		return True
	if fromTimer and machine.wake_reason() == (3, 1):
		return True
	return False

# Application launching

def start(app, status=False):
	import esp
	if status:
		import term, easydraw
		if app == "" or app == "launcher":
			term.header(True, "Loading menu...")
			easydraw.msg("Loading menu...", "Please wait", True)
		else:
			term.header(True, "Loading application "+app+"...")
			easydraw.msg("Starting '"+app+"'...", "Please wait", True)
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
		term.header(True, "Starting OTA...")
		easydraw.msg("Starting update...", "Please wait", True)
	esp.rtcmem_write(0,1)
	esp.rtcmem_write(1,254)
	reboot()

