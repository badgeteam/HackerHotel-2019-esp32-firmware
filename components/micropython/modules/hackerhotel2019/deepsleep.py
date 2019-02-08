import machine, badge, term, time

def start_sleeping(sleepTime=0):
	machine.RTC().wake_on_ext0(pin = machine.Pin(25), level = 0)
	
	term.header(True, "Going to sleep...")
	if (sleepTime >= 86400000): #One day
		sleepTime = 0
	if (sleepTime < 1):
		print("Sleeping until touchbutton is pressed...")
	else:
		print("Sleeping for "+str(sleepTime)+"ms...")
	time.sleep(0.05)
	badge.eink_busy_wait()
	machine.deepsleep(sleepTime)

def reboot():
    machine.deepsleep(2)
