# Pre-populate the repl with these common useful things
from machine import Pin, PWM, reset
from time import sleep_ms
import micropython
micropython.alloc_emergency_exception_buf(100)

def mount():
	try:
		import uos
		# Map the SPI flash as a block device
		from flashbdev import bdev
	except:
		print("unable to create flash block device: check pintout")
		return

	try:
		# Create a VFS mapping of the block device
		vfs = uos.VfsLfs2(bdev)
	except:
		uos.VfsLfs2.umount(bdev)
		print("""
lfs2 failed. to format the flash run these commands:

from flashbdev import bdev
import uos
uos.VfsLfs2.mkfs(bdev)
uos.mount(uos.VfsLfs2(bdev), '/')
""")
		return

	try:
		# Mount the VFS on the root
		uos.mount(vfs , "/")
	except:
		print("lfs2 flash block dev mount failed")
		return

	#print("SPI block device mounted on /")


mount()

def writefile(name, echo=True):
	from sys import stdin
	f = open(name, "w")
	line = ''
	while True:
		c = stdin.read(1)
		if c == chr(0x04):
			break
		if echo:
			print(c, end='')
		f.write(c)
	f.close()
