import ugfx, term, easydraw, badge, appglue

easydraw.msg("This app can only be controlled using the USB-serial connection.", "Notice", True)

def step2():
	term.header(True, "Security question")
	key = term.prompt("Name of the hacker event that took place between christmas 2018 and the start of 2019?\nHint: CCC organizes this event. It takes place in Leipzig.\n\n4 characters", 1, 5, "")
	if (key!="35C3") and (key!="35c3"):
		message  = "Invalid answer."
		items = ["Exit"]
		callbacks = [appglue.home]
		callbacks[term.menu("Download more memory!", items, 0, message)]()
	if badge.remove_bpp_partition(int("0x"+key)):
		message  = "The partition table has been updated succesfully. Enjoy the extra storage space!"
		items = ["Exit"]
		callbacks = [appglue.home]
		callbacks[term.menu("Download more memory!", items, 0, message)]()
	else:
		print("Failure in badge.remove_bpp_partition()! Good luck.")

if (badge.remove_bpp_partition(0x00)):
	message  = "You have already updated the partition table. Enjoy the extra storage space!"
	items = ["Exit"]
	callbacks = [appglue.home]
	callbacks[term.menu("Download more memory!", items, 0, message)]()
else:
	message  = "This application updates the partition table to allocate more flash space for storage."
	message += "\n"
	message += "Warning:\n"
	message += "Powering off the badge during the update process will result in a brick.\n"
	message += "You need to re-flash the badge using a computer in order to recover from such a brick.\n"
	message += "We provide no guarantee that nothing will go wrong. This new feature is largely UNTESTED.\n"
	message += "\n"
	message += " - Make sure you have inserted full batteries\n"
	message += " - Leave your badge connected to a computer while running this function\n"
	message += " - This function updates the partition table, it might result in a brick\n"
	

	items = ["Cancel", "Continue"]
	callbacks = [appglue.home, step2]
	callbacks[term.menu("Download more memory!", items, 0, message)]()

