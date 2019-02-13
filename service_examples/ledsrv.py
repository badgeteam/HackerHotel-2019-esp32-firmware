import badge

def init():
  print("LED init")
  global ledStep
  ledStep = 0

def step():
  global ledStep
  if ledStep >= 6:
	ledStep = 0
  data = [0]*6*4 #6 leds, 4 colors
  data[ledStep*4] = 100
  badge.leds_send_data(bytes(data))
  ledStep += 1
  return 100
