import term, badge, deepsleep as ds, appglue as app, version

class UartMenu():
	def __init__(self, gts, pm, safe = False):
		self.gts = gts
		self.menu = self.menu_main
		if (safe):
			self.menu = self.menu_safe
		self.buff = ""
		self.pm = pm
	
	def main(self):
		term.setPowerManagement(self.pm)
		while self.menu:
			self.menu()
		
	def drop_to_shell(self):
		self.menu = False
		term.clear()
		import shell
	
	def menu_main(self):
		items = ["Python shell", "Apps", "Installer", "Settings", "About", "Check for updates", "Power off"]
		callbacks = [self.drop_to_shell, self.opt_launcher, self.opt_installer, self.menu_settings, self.opt_about, self.opt_ota_check, self.go_to_sleep]
		message = "Welcome!\nYour badge is running firmware version "+str(version.build)+": "+version.name+"\n"
		cb = term.menu("Main menu", items, 0, message)
		self.menu = callbacks[cb]
		return
	
	def go_to_sleep(self):
		self.gts()
		
	def opt_change_nickname(self):
		app.start_app("nickname_term")
		
	def opt_installer(self):
		app.start_app("installer_term")
	
	def opt_launcher(self):
		app.start_app("launcher_term")
	
	def opt_configure_wifi(self):
		app.start_app("wifi_setup_term")
		
	def opt_ota(self):
		app.start_ota()
		
	def opt_ota_check(self):
		app.start_app("checkforupdates")
			
	def opt_downloadmorerom(self):
		app.start_app("downloadmorerom")
			
	def opt_about(self):
		app.start_app("about")
		
	
	def menu_settings(self):
		items = ["Change nickname", "Configure WiFi", "Update firmware", "(Reclaim unused flash space)", "< Return to main menu"]
		callbacks = [self.opt_change_nickname, self.opt_configure_wifi, self.opt_ota, self.opt_downloadmorerom, self.menu_main, self.menu_main]
		cb = term.menu("Settings", items)
		self.menu = callbacks[cb]
	
	def menu_safe(self):
		items = ["Main menu"]
		callbacks = [self.menu_main]
		cb = term.menu("You have started the badge in safe mode.\nServices are disabled.", items)
		self.menu = callbacks[cb]
