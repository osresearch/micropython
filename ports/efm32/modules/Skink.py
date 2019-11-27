import Radio
from AES import AES
from IEEE802154 import IEEE802154
from Packet import Packet

# This is the "well known" zigbee2mqtt key.
# The Ikea gateway uses a different key that has to be learned
# by joining the network (not yet implemented)
nwk_key = b"\x01\x03\x05\x07\x09\x0b\x0d\x0f\x00\x02\x04\x06\x08\x0a\x0c\x0d"

# Only need one AES object in ECB mode since there is no
# state to track

Radio.init()

def loop(sniff):
	if sniff:
		Radio.promiscuous(True)
	parser = IEEE802154(AES(nwk_key))
	data = Packet()
	while True:
		# the radio is a global from the micropython environment
		b = Radio.rx()
		if b is None:
			continue

		# discard the weird bytes
		data.init(b[:-2])
		try:
			parser.parse(data)
			parser.show()
		except:
			print(b[:-2])
			raise

def sniff():
	Radio.promiscuous(True)
	while True:
		bytes = Radio.rx();
		if bytes is None:
			continue
		for c in bytes:
			print("%02x" % (c), end='')
		print()
