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
    0b00000000, # blank
    0b01110111, # A
    0b01111111, # B
    0b00111001, # C
    0b00111111, # D
    0b01111001, # E
    0b01110001, # F
    0b00111101, # G
    0b01110110, # H
    0b10001001, # I (only works in position 0)
    0b00001110, # J
    0b01110101, # K
    0b00111000, # L
    0b10110111, # M (only works in position 0)
    0b00110111, # N
    0b00111111, # O
    0b01110011, # P
    0b01100111, # Q
    0b11110111, # R (only works in position 1)
    0b01101101, # S
    0b10000001, # T (only works in position 0; set (1, 12) to make it work in position 1)
    0b00111110, # U
    0b00111110, # V
    0b10111110, # W (only works in position 0)
    0b01111110, # X
    0b01101110, # Y
    0b00011011, # Z
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

def draw(segs, values):
	while segs != 0:
		slcd.set(segs & 0xFF, values & 1)
		segs >>= 8
		values >>= 1

def digit(n,v):
	if v < 0 or v >= len(digits):
		return False
	if n < 0 or n >= len(segments):
		return False

	draw(segments[n], digits[v])

def char(n,c):
	if n < 0 or n >= len(segments):
		return False

	c = ord(c)
	values = digits[10] # blank
	if ord('0') <= c and c <= ord('9'):
		values = digits[c - ord('0')]
	elif ord('A') <= c and c <= ord('Z'):
		values = digits[c - ord('A') + 11]
	draw(segments[n], values)

def colon(v):
	slcd.set(0x50, v)
def pm(v):
	slcd.set(0x91, v)
def h24(v):
	slcd.set(0x90, v)

def python():
	draw(segments[4], 0b01110011)
	draw(segments[5], 0b01101110)
	draw(segments[6], 0b01111000)
	draw(segments[7], 0b01110100)
	draw(segments[8], 0b01011100)
	draw(segments[9], 0b01010100)
