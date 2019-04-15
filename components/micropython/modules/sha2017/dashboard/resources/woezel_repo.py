import time, machine, gc, easydraw, term, uos, json, urequests, gc, sys, badge
class Repository():
	def __init__(self, path="/woezel_packages"):
		self.path = path
		self.categories = []
		self.lastUpdate = 0
		try:
			uos.mkdir(self.path)
		except:
			pass

	def _showProgress(self, msg, error=False, icon_wifi=False):
		term.header(True, "Installer")
		print(msg)
		if error:
			easydraw.messageCentered("ERROR\n\n"+msg, True, "/media/alert.png")
		else:
			if icon_wifi:
				easydraw.messageCentered("PLEASE WAIT\n\n"+msg, True, "/media/wifi.png")
			else:
				easydraw.messageCentered("PLEASE WAIT\n\n"+msg, True, "/media/busy.png")

	def update(self):
		import wifi
		if not wifi.status():
			self._showProgress("Connecting to WiFi...", False, True)
			wifi.connect()
			if not wifi.wait():
				self._showProgress("Failed to connect to WiFi.", True, False)
				return False
		self._showProgress("Downloading categories...")
		try:
			request = urequests.get("https://badge.team/eggs/categories/json", timeout=30)
			self._showProgress("Saving categories...")
			categories_file = open(self.path+'/categories.json', 'w')
			categories_file.write(request.text)
			categories_file.close()
			self._showProgress("Parsing categories...")
			self.categories = request.json()
			for category in self.categories:
				gc.collect()
				slug = category["slug"]
				self._showProgress("Downloading '"+category["name"]+"'...")
				f = urequests.get("https://badge.team/basket/sha2017/category/%s/json" % slug, timeout=30)
				f_file = open(self.path+'/'+slug+'.json', 'w')
				f_file.write(f.text)
				f_file.close()
			self.lastUpdate = int(time.time())
			f = open(self.path+"/lastUpdate", 'w')
			f.write(str(self.lastUpdate))
			f.close()
			self._showProgress("Done!")
			gc.collect()
			return True
		except BaseException as e:
			sys.print_exception(e)
			self._showProgress("Failed!", True)

			badge.raminfo()
			print("MEM FREE:",gc.mem_free())
			
			gc.collect()
		return False

	def load(self):
		try:
			f = open(self.path+"/lastUpdate", 'r')
			data = f.read()
			f.close()
			#print("Last update at",data)
			self.lastUpdate = int(data)
			f = open(self.path+"/categories.json")
			self.categories = json.loads(f.read())
			f.close()
			gc.collect()
			if (self.lastUpdate + 900) < int(time.time()):
				print("Too old", self.lastUpdate + 900, "<", int(time.time()))
				time.sleep(2)
				return False
			return True
		except BaseException as e:
			sys.print_exception(e)
			
			badge.raminfo()
			print("MEM FREE:",gc.mem_free())
			
			time.sleep(2)
			return False

	def getCategory(self, slug):
		f = open(self.path+"/"+slug+".json")
		data = json.loads(f.read())
		f.close()
		gc.collect()
		return data
