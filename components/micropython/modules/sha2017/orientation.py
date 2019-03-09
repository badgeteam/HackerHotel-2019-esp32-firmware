import badge, version, ugfx

def default():
	ugfx.orientation(badge.nvs_get_u16('badge', 'orientation', version.default_orientation))

def getDefault():
	return ugfx.orientation(badge.nvs_get_u16('badge', 'orientation', version.default_orientation))

def isLandscape(value=None):
	if value == None:
		value = getDefault()
	if value==90 or value==270:
		return False
	return True

def isPortrait(value=getDefault()):
	if value==90 or value==270:
		return True
	return False

def landscape():
	ugfx.orientation(0)

def portrait():
	ugfx.orientation(90)

def setDefault(value):
	if value == 0 or value == 90 or value == 180 or value == 270:
		badge.nvs_set_u16('badge', 'orientation', value)
		return True
	return False
