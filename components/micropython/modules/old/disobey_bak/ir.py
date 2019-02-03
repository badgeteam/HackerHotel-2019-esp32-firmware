from machine import Pin, PWM
from time import sleep_ms

optics_sleep = 50

pin_rx_enable = Pin(21, Pin.OUT)
pin_rx        = Pin(18, Pin.IN)
pin_tx        = Pin(19, Pin.OUT)
pwm_tx        = PWM(pin_tx, freq=38000, duty=0)

# Functions for switching power to the receiver on and off

def enableRx(state=True):
	pin_rx_enable.value(state)
	
def disableRx():
	pin_rx_enable.value(False)

# Functions for directly talking to the receiver and LED

def rawTx(p):
	pwm_tx.duty(512*p)
	
def rawRx():
	return not pin_rx.value()

# Helper functions for the (proof of concept) text transmission feature

def _txByte(byte):
	for bit in range(8):
		rawTx((byte>>(7-bit))&1)
		sleep_ms(optics_sleep)

buf = 0

def _rxBit():
	global buf
	buf = ((buf<<1) + rawRx()*1)&0xFFFF
	sleep_ms(optics_sleep)

def _rxByte():
	global buf
	for i in range(8):
		_rxBit()
	return buf & 0xFF

def _rxWait(timeout=100):
	global buf
	cnt = 0
	while True:
		_rxBit()
		if buf&0xFFFF == 0b1110101010101011:
			buf = 0
			return True
		#rxDebug()
		cnt+=1
		if cnt > timeout:
			return False

# Functions for text transmission (proof of concept)

def tx(data):
	rawTx(False)
	sleep_ms(optics_sleep*8)
	_txByte(0b11101010)
	_txByte(0b10101011)
	for char in data:
		txByte(ord(char))
	rawTx(False)

def rx(timeout=100):
	pin_rx_enable.value(True)
	global buf
	string = ""
	buf = 0
	if not _rxWait(timeout):
		pin_rx_enable.value(False)
		return None
	while True:
		b = _rxByte()
		if b < 1:
			pin_rx_enable.value(False)
			return string
		string += chr(b)
