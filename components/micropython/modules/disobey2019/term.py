import sys, version

def goto(x,y):
	sys.stdout.write(u"\u001b["+str(y)+";"+str(x)+"H")

def home():
	goto(1,1)

def clear():
	sys.stdout.write(u"\u001b[2J")
	home()
	
def color(fg=37, bg=40, style=0):
	sys.stdout.write(u"\u001b["+str(style)+";"+str(fg)+";"+str(bg)+"m")
	
def header(cls = False, text = ""):
	if cls:
		clear()
	else:
		home()
	if text:
		text = "- "+text
	color(37, 44, 1)
	print(version.badge_name+" "+text+u"\r\n")
	color()
	
def draw_menu(title, items, selected=0, text=""):
	header(False, title)
	if len(text)>0:
		print(text)
		print("")
	for i in range(0, len(items)):
		if (selected == i):
			color(30, 47, 0)
		else:
			color()
		print(items[i])
	color()
		
def menu(title, items, selected = 0, text=""):
	clear()
	while True:
		draw_menu(title, items, selected, text)
		key = sys.stdin.read(1)
		feedPm()
		if (ord(key)==0x1b):
			key = sys.stdin.read(1)
			if (key=="["):
				key = sys.stdin.read(1)
				if (key=="A"):
					if (selected>0):
						selected -= 1
				if (key=="B"):
					if (selected<len(items)-1):
						selected += 1
		if (ord(key)==0x01):
			import tasks.powermanagement as pm, badge
			pm.disable()
			badge.rawrepl()
			draw_menu(title, items, selected, text)
			pm.resume()
			
		if (ord(key)==0xa):
			return selected

def prompt(prompt, x, y, buff = ""):
	running = True
	while running:
		goto(x, y)
		sys.stdout.write(prompt+": ")
		color(30, 47, 0)
		sys.stdout.write(buff)
		if len(buff) < 64:
			sys.stdout.write(" "*(64-len(buff)))
		color()
		last = sys.stdin.read(1)
		if last == '\n' or last == '\r':
			return buff
		if ord(last) >= 32 and ord(last) < 127:
			buff += last
		if ord(last) == 127:
			buff = buff[:-1]

def empty_lines(count = 10):
	for i in range(0,count):
		print("")

# Functions for feeding the power management task

powerManagement = None

def setPowerManagement(pm):
	global powerManagement
	powerManagement = pm
	
def feedPm():
	if powerManagement != None:
		powerManagement.set_timeout(300000) #Set timeout to 5 minutes
		powerManagement.feed()
