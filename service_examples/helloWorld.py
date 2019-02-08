import ugfx

def init():
	print("This is the service init function.")

def draw(position):
	print("This is the service draw function.")
	ugfx.string(5, position, "Hello world", "Roboto_Regular18", ugfx.BLACK)
