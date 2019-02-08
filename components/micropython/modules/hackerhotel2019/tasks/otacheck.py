import wifi, easydraw, badge, time, version

def download_info():
	import urequests as requests
	easydraw.msg("Checking for firmware updates...")
	result = False
	try:
		data = requests.get(version.otacheckurl)
	except:
		easydraw.msg("Error: can not fetch information.")
		time.sleep(5)
		return False
	try:
		result = data.json()
	except:
		data.close()
		easydraw.msg("")
		time.sleep(5)
		return False
	data.close()
	return result

def available(update=False):
	if update:
		if not wifi.status():
			return badge.nvs_get_u8('badge','OTA.ready',0)

		info = download_info()
		if info:
			if info["build"] > version.build:
				badge.nvs_set_u8('badge','OTA.ready',1)
				return True

		badge.nvs_set_u8('badge','OTA.ready',0)
	return badge.nvs_get_u8('badge','OTA.ready',0)
