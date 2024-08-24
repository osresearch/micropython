# LCD display drivers for the SAML22
import slcd
import time

# Each segment is mapped:
#    00
#   5  1
#    66
#   4  2
#    33
#
#  Display is mapped:
#          0 1     2 3
#      4 5 : 6 7 : 8 9
#
# values copied from watch-library/shared/watch/watch_private_display.h
segments = [
    0x4e4f0e8e8f8d4d0d, # Position 0, mode
    0xc8c4c4c8b4b4b0b,  # Position 1, mode (Segments B and C shared, as are segments E and F)
    0xc049c00a49890949, # Position 2, day of month (Segments A, D, G shared; missing segment F)
    0xc048088886874707, # Position 3, day of month
    0xc053921252139352, # Position 4, clock hours (Segments A and D shared)
    0xc054511415559594, # Position 5, clock hours
    0xc057965616179716, # Position 6, clock minutes (Segments A and D shared)
    0xc041804000018a81, # Position 7, clock minutes
    0xc043420203048382, # Position 8, clock seconds
    0xc045440506468584, # Position 9, clock seconds
]

digits = [
    0b00111111, # 0
    0b00000110, # 1
    0b01011011, # 2
    0b01001111, # 3
    0b01100110, # 4
    0b01101101, # 5
    0b01111101, # 6
    0b00000111, # 7
    0b01111111, # 8
    0b01101111, # 9
]


slcd.init()

def animate(delay=0.1):
	slcd.clear()
	for com in range(0,coms):
		for seg in range(0,3):
			slcd.on(com,seg)
			time.sleep(delay)
	
	for com in range(0,coms):
		for seg in range(0,24):
			slcd.off(com,seg)
			time.sleep(delay)

def digit(n,v):
	if v < 0 or v > 9:
		return False
	if n < 0 or n >= len(segments):
		return False
	font = digits[v]
	segs = segments[n]

	while segs != 0:
		slcd.set(segs & 0xFF, font & 1)
		segs >>= 8
		font >>= 1


def showtime():
	now = time.localtime()
	digit(4, now[3] // 10)
	digit(5, now[3] %  10)
	digit(6, now[4] // 10)
	digit(7, now[4] %  10)
	digit(8, now[5] // 10)
	digit(9, now[5] %  10)

	day = now[2]
	if day > 10:
		digit(2, day // 10)
	digit(3, day % 10)
