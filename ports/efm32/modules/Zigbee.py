# Zigbee packet parser
# IEEE802154
# ZigbeeNetwork (NWK)
# (Optional encryption, with either Trust Center Key or Transport Key)
# ZigbeeApplicationSupport (APS)
# ZigbeeClusterLibrary (ZCL)

# Zigbee Trust Center key: 5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39
FRAME_TYPE_DATA = 1
import CCM

class ZigbeeNetwork:

	# parse the ZigBee network layer
	def __init__(self,
		frame_type = FRAME_TYPE_DATA,
		version = 2,
		src = 0,
		dst = 0,
		ext_src = None,
		ext_dst = None,
		data = None,
		aes = None,
		security = False,
		sec_seq = 0,
		sec_src = None,
		sec_key = 0,
		sec_key_seq = 0
	):
		self.aes = aes

		if data is not None:
			self.deserialize(data)
		else:
			self.frame_type = frame_type
			self.version = version
			self.src = src
			self.dst = dst
			self.ext_src = ext_src
			self.ext_dst = ext_dst

			self.security = security
			self.sec_seq = sec_seq
			self.sec_src = sec_src
			self.sec_key = sec_key
			self.sec_key_seq = sec_key_seq

	def __str__(self):
		params = [
			"frame_type=" + str(self.frame_type),
			"version=" + str(self.version),
			"radius=" + str(self.radius),
			"seq=" + str(self.seq),
			"dst=0x%04x" % (self.dst),
			"src=0x%04x" % (self.src),
		]

		if self.ext_src is not None:
			params.append("ext_src=" + str(self.ext_src))

		if self.security:
			params.extend([
				"security=True",
				"sec_seq=" + str(self.sec_seq),
				"sec_key=" + str(self.sec_key),
				"sec_key_seq=" + str(self.sec_key_seq)
			])
			if self.sec_src is not None:
				params.append("sec_src=" + str(self.sec_src))
			if not self.valid:
				params.append("valid=FALSE")

		params.append("payload=" + str(self.payload))

		return "Zigbee.ZigbeeNetwork(" + ", ".join(params) + ")"

	def deserialize(self, b):
		j = 0
		fcf = (b[j+1] << 8) | (b[j+0] << 0); j += 2
		self.frame_type = (fcf >> 0) & 3
		self.version = (fcf >> 2) & 0xF
		self.security = (fcf >> 9) & 1
		dst_mode = (fcf >> 11) & 1
		src_mode = (fcf >> 12) & 1

		self.dst = (b[j+1] << 8) | (b[j+0] << 0); j += 2
		self.src = (b[j+1] << 8) | (b[j+0] << 0); j += 2
		self.radius = b[j]; j += 1
		self.seq = b[j]; j += 1

#		if frame_type == 0x0:
#			self.frame_type = "ZDAT"
#		elif frame_type == 0x1:
#			self.frame_type = "ZCMD"
#		elif frame_type == 0x2:
#			self.frame_type = "ZRSV"
#		elif frame_type == 0x3:
#			self.frame_type = "ZPAN"

		if dst_mode:
			# extended dest is present
			self.ext_dst = b[j:j+8]
			j += 8
		else:
			self.ext_dst = None

		if src_mode:
			# extended source is present
			self.ext_src = b[j:j+8]
			j += 8
		else:
			self.ext_src = None

		if not self.security:
			# the rest of the packet is the payload
			self.payload = b[j:]
		else:
			# security header is present, attempt to decrypt
			# and validate the message.
			self.ccm_decrypt(b, j)
		return self

	# security header is present; b contains the entire Zigbee NWk header
	# so that the entire MIC can be computed
	def ccm_decrypt(self, b, j):
		# the security control field is not filled in correctly in the header,
		# so it is necessary to patch it up to contain ZBEE_SEC_ENC_MIC32
		# == 5. Not sure why, but wireshark does it.
		sec_hdr = b[j]
		sec_hdr = (sec_hdr & ~0x07) | 0x05
		b[j] = sec_hdr # modify in place
		j += 1
		self.sec_key = (sec_hdr >> 3) & 3
		self.sec_seq = b[j:j+4] ; j += 4

		# 8 byte ieee address, used in the extended nonce
		if sec_hdr & 0x20: # extended nonce bit
			self.sec_src = b[j:j+8]
			j += 8
		else:
			# hopefully the one in the header was present
			self.sec_src = None

		# The key seq tells us which key is used;
		# should always be zero?
		self.sec_key_seq = b[j] ; j += 1

		# section 4.20 says extended nonce is
		# 8 bytes of IEEE address, little endian
		# 4 bytes of counter, little endian
		# 1 byte of patched security control field
		nonce = bytearray(16)
		nonce[0] = 0x01
		nonce[1:9] = self.sec_src
		nonce[9:13] = self.sec_seq
		nonce[13] = sec_hdr # modified sec hdr!
		nonce[14] = 0x00
		nonce[15] = 0x00

		# authenticated data is everything up to the message
		# which includes the network header and the unmodified sec_hdr value
		auth = b[0:j]
		C = b[j:-4]  # cipher text
		M = b[-4:]   # message integrity code

		if not CCM.decrypt(auth, C, M, nonce, self.aes):
			print("BAD DECRYPT: ", b)
			#print("message=", C)
			self.payload = C
			self.valid = False
		else:
			self.payload = C
			self.valid = True


if __name__ == "__main__":
	import AES
	import IEEE802154
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
	print(ieee)
