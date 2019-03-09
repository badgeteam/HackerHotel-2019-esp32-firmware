import badge, easydraw, woezel, wifi

lvl = badge.nvs_get_u8('ota', 'fixlvl', 0)

if lvl<5:
	# Up-to-date with Hackerhotel badge fixlvl
	badge.nvs_set_u8('ota', 'fixlvl', 5)
