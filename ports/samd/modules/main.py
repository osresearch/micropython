import machine, time, sys, os
from machine import Pin, PWM
#print("Hello, world!", time.ticks_us())
#print(machine.unique_id().hex())

def hexdump(addr,len=128):
	for offset in range(0,len,4):
		print("%08x+%02x: %08x" % (addr,offset,machine.mem32[addr+offset]))

import watch
#watch.run()

