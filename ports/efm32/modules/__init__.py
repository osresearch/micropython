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

	print("SPI block device mounted on /")


mount()

