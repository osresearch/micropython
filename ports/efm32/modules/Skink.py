import Radio
import AES
import IEEE802154
import ZigbeeNetwork
import ZigbeeApplication
import ZigbeeCluster

# This is the "well known" zigbee2mqtt key.
# The Ikea gateway uses a different key that has to be learned
# by joining the network (not yet implemented)
nwk_key = b"\x01\x03\x05\x07\x09\x0b\x0d\x0f\x00\x02\x04\x06\x08\x0a\x0c\x0d"
aes = AES.AES(nwk_key)

# Only need one AES object in ECB mode since there is no
# state to track

Radio.init()

def loop(sniff):
	if sniff:
		Radio.promiscuous(True)
	#parser = IEEE802154(AES(nwk_key))
	#data = Packet()
	while True:
		# the radio is a global from the micropython environment
		b = Radio.rx()
		if b is None:
			continue

		try:
			print("------")
			# discard the weird bytes
			data = b[:-2]
			print(data)
			ieee = IEEE802154.Packet(data=data)
			print(ieee)
			if ieee.frame_type != IEEE802154.FRAME_TYPE_DATA:
				continue
			nwk = ZigbeeNetwork.ZigbeeNetwork(aes=aes, data=ieee.payload)
			print(nwk)
			if nwk.frame_type != ZigbeeNetwork.FRAME_TYPE_DATA:
				continue
			aps = ZigbeeApplication.ZigbeeApplication(data=nwk.payload)
			print(aps)
			if aps.frame_type != ZigbeeApplication.FRAME_TYPE_DATA:
				continue
			zcl = ZigbeeCluster.ZigbeeCluster(data=aps.payload)
			print(zcl)
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


def join():
	Radio.promiscuous(False)
	Radio.tx(IEEE802154.serialize())
