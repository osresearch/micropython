from machine import Pin, SPI, SPIFlash
from ubinascii import hexlify
import uos

class FlashBdev:

    SEC_SHIFT = 12 # 4096 bytes per sector
    SEC_SIZE = 1 << SEC_SHIFT
    START_SEC = 0
    NUM_BLK = 0x100000 >> SEC_SHIFT

    def __init__(self, blocks=NUM_BLK):
        self.blocks = blocks
        self.cs = Pin(12)
	self.cs.on()

        self.spi = SPI(
		sck=Pin(13),
		miso=Pin(14),
		mosi=Pin(15),
	)

	# These are the pins for the SiLabs gecko board.
	# Ikea matched the same pinout when they laid out the On/Off switch
	self.flash = SPIFlash(self.cs, self.spi)

	#print("SPI device ID:", self.rdid())


    def readblocks(self, n, buf, off=0):
        #print("readblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
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
            self.flash.erase(arg + self.START_SEC)
            return 0

    def all_ff(self, buf):
        for x in buf:
            if x != 0xFF:
                return False
        return True

    def format(self, key = 0):
        if key != 0xDEAD:
		print("Wrong key, pass in 0xDEAD to be sure")
		return

	try:
		print("UNMOUNTING FLASH")
		uos.umount("/")
	except:
		pass

        print("ERASING THE FLASH")
	self.chip_erase()

	print("\nMAKING LFS2 FILESYSTEM")
	uos.VfsLfs2.mkfs(self)

	print("MOUNTING LFS2 FILESYSTEM")
	uos.mount(uos.VfsLfs2(self), "/")

    # try to read the chip id
    def rdid(self):
	self.cs.off()
	self.spi.write(b'\x9f')
	jdec_id = hexlify(self.spi.read(3))
	self.cs.on()
	return jdec_id

    def _wren(self):
	self.cs.off()
	self.spi.write(b'\x06')
	self.cs.on()

    def wren(self):
        for i in range(8):
		self._wren()
                sr = self.sr()
                if sr & 2:
			return True
	print("WREN never set?")
	return False

    def chip_erase(self):
	if not self.wren():
		return
	self.cs.off()
	self.spi.write(b'\xc7')
	self.cs.on()

	if (self.sr() & 1) == 0:
		print("WIP not set?")

	for i in range(10000):
		if (self.sr() & 1) == 0:
			break;
		if i % 1000 == 0:
			print(".", end='')

	print("DONE")

    def sr(self):
	self.cs.off()
	self.spi.write(b'\x05')
        x = self.spi.read(1)
	self.cs.on()
        return x[0]
	
    # helper to read from the flash and return it
    def read(self, addr, length):
	b = bytearray(length)
	self.readblocks(addr >> self.SEC_SHIFT, b, off=addr & (self.SEC_SIZE-1))
	return b

size = 0x100000 >> 12
bdev = FlashBdev(size) # could reserve space
