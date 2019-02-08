import machine, badge

def start_sleeping(time=0):
	pin = machine.Pin(25)
	rtc = machine.RTC()
	rtc.wake_on_ext0(pin = pin, level = 0)
	badge.eink_busy_wait()
	machine.deepsleep(time)

def reboot():
	badge.eink_busy_wait()
	machine.deepsleep(1)
