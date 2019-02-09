import ugfx, time, badge, machine, deepsleep, gc, appglue, easydraw, wifi, term, orientation, uos, json, sys, urequests, gc

woezel_repo = '/woezel_packages'

orientation.default()

try:
	uos.mkdir(woezel_repo)
except:
	pass

easydraw.msg("Connecting to WiFi...","Update repo...", True)

wifi.connect()
wifi.wait()

easydraw.msg("Downloading categories...")

try:
	categories = urequests.get("https://badge.team/eggs/categories/json", timeout=30)
	easydraw.msg("Saving file...")
	categories_file = open(woezel_repo+'/categories.json', 'w')
	categories_file.write(categories.text)
	categories_file.close()
	easydraw.msg("Loading file...")
	categories = categories.json()
	for category in categories:
		gc.collect()
		slug = category["slug"]
		easydraw.msg("Loading '"+category["name"]+"'...")
		f = urequests.get("https://badge.team/eggs/category/%s/json" % slug, timeout=30)
		f_file = open(woezel_repo+'/'+slug+'.json', 'w')
		f_file.write(f.text)
		f_file.close()

	easydraw.msg("Done!")
	appglue.start_app('installer', False)
except:
	easydraw.msg("Failed!")
	time.sleep(2)
	appglue.start_app('program_main', False)
