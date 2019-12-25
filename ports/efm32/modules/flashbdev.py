from machine import Pin, SPI, SPIFlash
from ubinascii import hexlify

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

	# try to read the chip id
	self.cs.off()
	self.spi.write(b'\x9f')
	print("SPI device ID:", hexlify(self.spi.read(3)))
	self.cs.on()


    def readblocks(self, n, buf, off=0):
        #print("readblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
        self.flash.read(((n + self.START_SEC) << self.SEC_SHIFT) + off, buf)

    def writeblocks(self, n, buf, off=None):
        #print("writeblocks(%s, %x(%d), %d)" % (n, id(buf), len(buf), off))
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

size = 0x100000
bdev = FlashBdev(size) # could reserve space
