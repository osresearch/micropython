# Zigbee Application Support Layer packet parser
#   IEEE802154
#   ZigbeeNetwork (NWK)
#   (Optional encryption, with either Trust Center Key or Transport Key)
#-> ZigbeeApplicationSupport (APS)
#   ZigbeeClusterLibrary (ZCL)

MODE_UNICAST = 0x0
MODE_BROADCAST = 0x2
MODE_GROUP = 0x3

FRAME_TYPE_DATA = 0x0
FRAME_TYPE_ACK = 0x1

class ZigbeeApplication:

	# parse the ZigBee Application packet
	# src/dst are endpoints
	def __init__(self,
		data = None,
		frame_type = FRAME_TYPE_DATA,
		mode = MODE_UNICAST,
		src = 0,
		dst = 0,
		cluster = 0,
		profile = 0,
		seq = 0,
		ack_req = False,
	):
		if data is not None:
			self.deserialize(data)
		else:
			self.frame_type = frame_type
			self.mode = mode
			self.src = src
			self.dst = dst
			self.cluster = cluster
			self.profile = profile
			self.seq = seq
			self.ack_req = ack_req

	def __str__(self):
		params = [
			"frame_type=" + str(self.frame_type),
			"cluster=0x%04x" % (self.cluster),
			"profile=0x%04x" % (self.profile),
			"mode=" + str(self.mode),
			"seq=" + str(self.seq),
			"src=0x%02x" % (self.src),
		]

		if self.mode == MODE_GROUP:
			params.append("dst=0x%04x" % (self.dst))
		else:
			params.append("dst=0x%02x" % (self.dst))

		if self.ack_req:
			params.append("ack_req=1")

		params.append("payload=" + str(self.payload))

		return "ZigbeeApplication(" + ", ".join(params) + ")"

	def deserialize(self, b):
		j = 0
		fcf = b[j]; j += 1
		self.frame_type = (fcf >> 0) & 3
		self.mode = (fcf >> 2) & 3
		self.ack_req = (fcf >> 7) & 1

		if self.mode == MODE_GROUP:
			self.dst = (b[j+1] << 8) | (b[j+0] << 0)
			j += 2
		else:
			self.dst = b[j]
			j += 1

		self.cluster = (b[j+1] << 8) | (b[j+0] << 0); j += 2
		self.profile = (b[j+1] << 8) | (b[j+0] << 0); j += 2
		self.src = b[j]; j += 1
		self.seq = b[j]; j += 1

		self.payload = b[j:]

		return self


if __name__ == "__main__":
	import AES
	import IEEE802154
	from ZigbeeNetwork import ZigbeeNetwork
	nwk_key = b"\x01\x03\x05\x07\x09\x0b\x0d\x0f\x00\x02\x04\x06\x08\x0a\x0c\x0d"
	aes = AES.AES(nwk_key)
	ieee = IEEE802154.Packet(data=bytearray(b'A\x88\xacb\x1a\xff\xff\x00\x00\t\x12\xfc\xff\x00\x00\x01\x04\xb1\x9d\xe8\x0b\x00K\x12\x00(\x82\xf9\x04\x00\xb1\x9d\xe8\x0b\x00K\x12\x00\x00:\x0f6y\r7'))
	#print(ieee)
	ieee.payload = ZigbeeNetwork(aes=aes, data=ieee.payload)
	print(ieee)


	print(ZigbeeNetwork(aes=aes, data=bytearray(b'\x08\x02\xfc\xff\x00\x00\x1e\xe1(X\xf9\x04\x00\xb1\x9d\xe8\x0b\x00K\x12\x00\x00W\xd6\xa8\xcc=\x0eo\x95\xee\xa0+\xdf\x1e=\xa3')))

	print("----")
	ieee = IEEE802154.Packet(data=bytearray.fromhex("41883b621affff00004802fdff382b0b432887480400b19de80b004b120000055665c14f13102410078a3d12501ff1"))
	print(ieee)
	ieee.payload = ZigbeeNetwork(aes=aes, data=ieee.payload)
	ieee.payload.payload = ZigbeeApplication(data=ieee.payload.payload)
	print(ieee)
