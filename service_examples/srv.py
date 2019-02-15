import ugfx, virtualtimers

seconds = 0
task_active = False

def init(cb):
	global redraw
	redraw = cb
	print("This is the service init function.")

def draw(position):
	global seconds
	print("This is the service draw function.")
	ugfx.string(5, position, "Hello world", "Roboto_Regular18", ugfx.BLACK)
	ugfx.string(5, position+24, str(seconds), "Roboto_Regular18", ugfx.BLACK)

def focus(in_focus):
	global task_active, seconds
	print("Focus changed to", in_focus)
	if in_focus:
		if not task_active:
			virtualtimers.new(0, counterTask, False)
			task_active = True
	else:
		if task_active:
			virtualtimers.delete(counterTask)
			task_active = False
		seconds = 0

def counterTask():
	global seconds, redraw
	seconds += 1
	print("CounterTask", seconds)
	redraw()
	return 1000
