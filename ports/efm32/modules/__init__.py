import uos

# Map the SPI flash as a block device
from flashbdev import bdev

# Create a VFS mapping of the block device
vfs = uos.VfsLfs2(bdev)

# Mount the VFS on the root
uos.mount(vfs , "/")
