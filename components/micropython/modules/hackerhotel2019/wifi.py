import network, badge, time

sta_if = None
defaultSsid = badge.nvs_get_str('badge', 'wifi.ssid', 'hackerhotel-insecure')
defaultPassword = badge.nvs_get_str('badge', 'wifi.password')
timeout = badge.nvs_get_str('badge', 'wifi.timeout', 5)

def status():
	global sta_if
	if sta_if == None:
		return False
	return sta_if.isconnected()

def connect(ssid=defaultSsid, password=defaultPassword, wait=False, waitDuration=timeout, waitStatus=False):
	init()
	sta_if.active(True)
	if password:
		sta_if.connect(ssid, password)
	else:
		sta_if.connect(ssid)
	if wait:
		return wait(waitDuration, waitStatus)

def disconnect():
	sta_if.disconnect()

def init():
	global sta_if
	if sta_if == None:
		sta_if = network.WLAN(network.STA_IF)

def wait(duration=timeout, showStatus=False):
	if showStatus:
		import easydraw
		easydraw.msg("Connecting to WiFi...")
	while not status():
		time.sleep(1)
		duration -= 1
		if duration < 0:
			if showStatus:
				easydraw.msg("Failed!")
			return False
		if showStatus:
			easydraw.msg("Connected!")
	return True

def ntp(onlyIfNeeded=True):
	if onlyIfNeeded and time.time() > 1482192000:
		return True
	import ntp
	if not status():
		connect()
	if not wait():
		return False
	return ntp.set_NTP_time()
