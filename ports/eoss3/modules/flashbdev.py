from machine import SPIFlash
from binascii import hexlify
import uos

class FlashBdev:

    SEC_SHIFT	= 12 # 4096 bytes per sector
    SEC_SIZE	= 1 << SEC_SHIFT
    START_SEC	= 0x100000 >> SEC_SHIFT
    NUM_BLK	= 0x10000 >> SEC_SHIFT

    def __init__(self, blocks=NUM_BLK):
        self.blocks = blocks
	self.flash = SPIFlash()

    def readblocks(self, n, buf, off=0):
        print("readblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        self.flash.read(((n + self.START_SEC) << self.SEC_SHIFT) + off, buf)

    def writeblocks(self, n, buf, off=None):
        print("writeblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        #assert len(buf) <= self.SEC_SIZE, len(buf)
        if off is None:
            self.flash.erase(n + self.START_SEC)
            off = 0
        self.flash.write(((n + self.START_SEC) << self.SEC_SHIFT) + off, buf)

    def ioctl(self, op, arg):
        #print("ioctl(%d, %r)" % (op, arg))
        if op == 4:  # MP_BLOCKDEV_IOCTL_BLOCK_COUNT
            return self.blocks
        if op == 5:  # MP_BLOCKDEV_IOCTL_BLOCK_SIZE
            return self.SEC_SIZE
        if op == 6:  # MP_BLOCKDEV_IOCTL_BLOCK_ERASE
            print("erase(%d)" % (arg))
            self.flash.erase(arg + self.START_SEC)
            return 0

    def format(self, key = 0):
        if key != 0xDEAD:
		print("Wrong key, pass in 0xDEAD to be sure")
		return

	try:
		print("UNMOUNTING FLASH")
		uos.umount("/")
	except:
		pass

#        print("ERASING THE FLASH")
#	for block in range(0, self.blocks):
#		print("erase(%d)" % (block))
#		self.flash.erase(self.START_SEC + block)

	print("\nMAKING LFS2 FILESYSTEM")
	uos.VfsLfs2.mkfs(self)

	print("MOUNTING LFS2 FILESYSTEM")
	uos.mount(uos.VfsLfs2(self), "/")

    # helper to read from the flash and return it
    def read(self, addr, length):
	b = bytearray(length)
	self.readblocks(addr >> self.SEC_SHIFT, b, off=addr & (self.SEC_SIZE-1))
	return b

size = 0x100000 >> 12
bdev = FlashBdev(size) # could reserve space
