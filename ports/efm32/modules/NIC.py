# Turn the Gecko device into a serial NIC
# Receives packets from the wire and prints the hex format to the console
# Reads hex from the serial port and sends it on the wire

import gc
import os
import sys
import machine
from ubinascii import hexlify, unhexlify
import Radio
Radio.init()
Radio.promiscuous(True)

incoming = ''
machine.zrepl(False)

while True:
	pkt = Radio.rx()
	if pkt is not None:
		# throw away the extra b' and ' on the string
		# representation
		x = repr(hexlify(pkt))[2:-1]
		print(x)
		x = None
		gc.collect()
	if machine.stdio_poll():
		y = sys.stdin.read(1)
		if y == '\r':
			continue
		if y != '\n':
			incoming += y
			continue
		print("SEND:", incoming)
		try:
			x = unhexlify(incoming)
			Radio.tx(x)
		except:
			print("FAIL")
		incoming = ''
		x = None
		gc.collect()
	
