import ugfx, esp, badge, deepsleep, term, easydraw

def start_app(app, display = True):
	if display:
		easydraw.msg(app, "Loading...", True)
		term.header(True, "Loading application "+app+"...")
	esp.rtcmem_write_string(app)
	deepsleep.reboot()

def home():
	start_app("")

def start_ota():
	term.header(True, "Starting OTA...")
	esp.rtcmem_write(0,1)
	esp.rtcmem_write(1,254)
	deepsleep.reboot()
