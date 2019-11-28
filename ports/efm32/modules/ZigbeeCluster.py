# Zigbee Cluster Library packet parser
#   IEEE802154
#   ZigbeeNetwork (NWK)
#   (Optional encryption, with either Trust Center Key or Transport Key)
#   ZigbeeApplicationSupport (APS)
#-> ZigbeeClusterLibrary (ZCL)

FRAME_TYPE_CLUSTER_SPECIFIC = 1
DIRECTION_TO_SERVER = 0
DIRECTION_TO_CLIENT = 1

class ZigbeeCluster:

	def __init__(self,
		data = None,
		frame_type = FRAME_TYPE_CLUSTER_SPECIFIC,
		direction = DIRECTION_TO_SERVER,
		manufacturer_specific = False,
		disable_default_response = False,
		seq = 0,
		command = 0,
		payload = None,
	):
		if data is not None:
			self.deserialize(data)
		else:
			self.frame_type = frame_type
			self.direction = direction
			self.seq = seq
			self.disable_default_response = disable_default_response
			self.manufacturer_specific = manufacturer_specific
			self.command = command
			self.payload = payload

	def __str__(self):
		params = [
			"command=0x%02x" % (self.command),
			"seq=%d" % (self.seq),
			"direction=%d" % (self.direction),
		]

		if self.disable_default_response:
			params.append("disable_default_response=1")
		if self.manufacturer_specific:
			params.append("manufacturer_specific=1")

		params.append("payload=" + str(self.payload))

		return "ZigbeeCluster(" + ", ".join(params) + ")"

	def deserialize(self, b):
		j = 0
		fcf = b[j]; j += 1
		self.frame_type = (fcf >> 0) & 3
		self.manufacturer_specific = (fcf >> 2) & 1
		self.direction = (fcf >> 3) & 1
		self.disable_default_response = (fcf >> 4) & 1

		self.seq = b[j]; j += 1
		self.command = b[j];  j += 1
		self.payload = b[j:]

		return self


if __name__ == "__main__":
	import AES
	import IEEE802154
	import ZigbeeNetwork
	import ZigbeeApplication
	import ZigbeeCluster
	nwk_key = b"\x01\x03\x05\x07\x09\x0b\x0d\x0f\x00\x02\x04\x06\x08\x0a\x0c\x0d"
	aes = AES.AES(nwk_key)
	data = bytearray(b'A\x88\xc6b\x1a\xff\xff\x00\x00\x08\x02\xfc\xff\x00\x00\x1e\xed(\x97\n\x05\x00\xb1\x9d\xe8\x0b\x00K\x12\x00\x004\x042v\x18Q\x9a\xf4N\x14I\xb8\x0bE\x8c')
	ieee = IEEE802154.IEEE802154(data=data)
	print(ieee)
	nwk = ZigbeeNetwork.ZigbeeNetwork(aes=aes, data=ieee.payload)
	print(nwk)
	aps = ZigbeeApplication.ZigbeeApplication(data=nwk.payload)
	print(aps)
	zcl = ZigbeeCluster.ZigbeeCluster(data=aps.payload)
	print(zcl)
