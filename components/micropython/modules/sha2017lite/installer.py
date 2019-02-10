import ugfx, time, badge, machine, deepsleep, gc, appglue, easydraw, wifi, term, orientation, uos, json, sys, urequests, gc, woezel

woezel_repo = '/woezel_packages'

orientation.default()

categories = None

easydraw.msg("Loading categories...","Installer", True)

try:
	f = open(woezel_repo+"/categories.json")
	categories = json.loads(f.read())
	f.close()
	gc.collect()
	easydraw.msg("Done.")
except:
	easydraw.msg("Preparing...","Update repo...", True)
	appglue.start_app("update_woezel_repo", False)

categories_list = ugfx.List(0,0,ugfx.width(),ugfx.height()-48)
category_list   = ugfx.List(0,0,ugfx.width(),ugfx.height()-48)

for category in categories:
	categories_list.add_item("%s (%d) >" % (category["name"], category["eggs"]))
categories_list.selected_index(0)

def btn_unhandled(pressed):
	ugfx.flush()

def btn_home(pressed):
	if pressed:
		appglue.home()

def btn_update(pressed):
	if pressed:
		easydraw.msg("Preparing...","Update repo...", True)
		appglue.start_app("update_woezel_repo", False)

def show_categories(pressed=True):
	ugfx.clear(ugfx.WHITE)
	if pressed:
		#Hide category list
		category_list.visible(False)
		category_list.enabled(False)
		#Show categories list
		categories_list.visible(True)
		categories_list.enabled(True)
		#Input handling
		ugfx.input_attach(ugfx.BTN_START, btn_home)
		ugfx.input_attach(ugfx.BTN_SELECT, btn_update)
		ugfx.input_attach(ugfx.BTN_A, show_category)
		ugfx.input_attach(ugfx.BTN_B, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_UP, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_DOWN, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_LEFT, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_RIGHT, btn_unhandled)
		#Hint
		easydraw.disp_string_right_bottom(0, "START:  Home")
		easydraw.disp_string_right_bottom(1, "SELECT: Update")
		easydraw.disp_string_right_bottom(2, "A:      Select")
		#Flush screen
		ugfx.flush()

app = None	

def show_category(pressed=True):
	global category
	if pressed:
		categories_list.visible(False)
		categories_list.enabled(False)
		slug = categories[categories_list.selected_index()]["slug"]
		easydraw.msg("Loading "+slug+"...", "Loading...", True)
		#Clean up list
		while category_list.count() > 0:
			category_list.remove_item(0)
		try:
			print("Loading "+slug+"...")
			f = open(woezel_repo+"/"+slug+".json")
			category = json.loads(f.read())
			f.close()
			gc.collect()
			for package in category:
				category_list.add_item("%s rev. %s" % (package["name"], package["revision"]))
			category_list.selected_index(0)
			category_list.visible(True)
			category_list.enabled(True)
			#Input handling
			ugfx.input_attach(ugfx.BTN_START, btn_home)
			ugfx.input_attach(ugfx.BTN_SELECT, btn_unhandled)
			ugfx.input_attach(ugfx.BTN_A, install_app)
			ugfx.input_attach(ugfx.BTN_B, show_categories)
			ugfx.input_attach(ugfx.JOY_UP, btn_unhandled)
			ugfx.input_attach(ugfx.JOY_DOWN, btn_unhandled)
			ugfx.input_attach(ugfx.JOY_LEFT, btn_unhandled)
			ugfx.input_attach(ugfx.JOY_RIGHT, btn_unhandled)
			#Hint
			easydraw.disp_string_right_bottom(0, "START:  Home")
			easydraw.disp_string_right_bottom(1, "B:      Back")
			easydraw.disp_string_right_bottom(2, "A:      Install")
			#Flush screen
			ugfx.flush()
		except BaseException as e:
			sys.print_exception(e)
			easydraw.msg("", "Error", True)
			time.sleep(1)
			show_categories()

def install_app(pressed=True):
	global category
	slug = category[category_list.selected_index()]["slug"]
	category = None
	gc.collect()
	
	if pressed:
		category_list.visible(False)
		category_list.enabled(False)
		#Input handling
		ugfx.input_attach(ugfx.BTN_START, btn_unhandled)
		ugfx.input_attach(ugfx.BTN_SELECT, btn_unhandled)
		ugfx.input_attach(ugfx.BTN_A, btn_unhandled)
		ugfx.input_attach(ugfx.BTN_B, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_UP, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_DOWN, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_LEFT, btn_unhandled)
		ugfx.input_attach(ugfx.JOY_RIGHT, btn_unhandled)
		if not wifi.status():
			wifi.connect()
			wifi.wait()
			if not wifi.status():
				easydraw.msg("No WiFi.")
				time.sleep(2)
				show_category()
		easydraw.msg(slug,"Installing...", True)
		try:
			woezel.install(slug)
		except woezel.LatestInstalledError:
			easydraw.msg("Already on latest.")
			time.sleep(2)
			show_category()
		except:
			easydraw.msg("Failed.")
			time.sleep(2)
			show_category()
		easydraw.msg("Done.")
		show_category()

show_categories()
