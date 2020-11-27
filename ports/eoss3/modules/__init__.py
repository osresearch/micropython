# micropython enviroment startup
print("__init__.py")
import machine
from machine import reset, bootloader
from binascii import hexlify, unhexlify

######
