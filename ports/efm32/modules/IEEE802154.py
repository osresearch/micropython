# Process IEEE 802.15.4 packets and return an unpacked form of them
# The Zigbee specific stuff should be split out into a separate file
import CCM

class IEEE802154:
	def __init__(self, aes):
		self.aes = aes

	# parse an incoming 802.15.4 message
	# b should be a "Packet"
	def parse(self,b):
		self.fcf = b.u16()
		self.seq = b.u8()
		self.frame_type = (self.fcf >> 0) & 0x7

		dst_mode = (self.fcf >> 10) & 0x3
		src_mode = (self.fcf >> 14) & 0x3
		self.src = 0
		self.dst = 0
		self.dst_pan = 0
		self.src_pan = 0
		self.mic = None

		if dst_mode == 2:
			# short destination addresses
			self.dst_pan = b.u16()
			self.dst = b.u16()
		elif dst_mode == 3:
			# long addresses
			self.dst_pan = b.u16()
			self.dst_addr = b.data(8)

		if src_mode == 2:
			# short source addressing
			if (self.fcf >> 6) & 1:
				# pan compression, use the dst_pan
				self.src_pan = self.dst_pan
			else:
				# pan is in the message
				self.src_pan = b.u16()
			self.src = b.u16()
		elif src_mode == 3:
			if (self.fcf >> 6) & 1:
				# pan compression, use the dst_pan
				self.src_pan = self.dst_pan
			else:
				# pan is in the message
				self.src_pan = b.u16()
			self.src_addr = b.data(8)

		if self.frame_type == 0:
			self.frame_type = "BCN"
			self.payload = b.data(b.remaining())
		elif self.frame_type == 1:
			self.frame_type = "DAT"
			self.nwk_parse(b)
		elif self.frame_type == 2:
			self.frame_type = "ACK"
			self.payload = b.data(b.remaining())
		elif self.frame_type == 3:
			self.frame_type = "CMD"
			self.cmd_parse(b)
		else:
			self.frame_type = "%03b" % (self.frame_type)
			self.payload = b.data(b.remaining())

	# parse an IEEE802.15.4 command
	def cmd_parse(self, b):
		cmd = b.u8()
		if cmd == 0x04:
			self.payload = "Data request"
		elif cmd == 0x07:
			self.payload = "Beacon request"
		else:
			self.payload = "Command %02x" % (cmd)

	# parse the ZigBee network layer
	def nwk_parse(self, b):
		auth_start = b.offset()
		fcf = b.u16()
		dst = b.u16()
		src = b.u16()
		radius = b.u8()
		seq = b.u8()

		frame_type = (fcf >> 0) & 3
		if frame_type == 0x0:
			self.frame_type = "ZDAT"
		elif frame_type == 0x1:
			self.frame_type = "ZCMD"
		elif frame_type == 0x2:
			self.frame_type = "ZRSV"
		elif frame_type == 0x3:
			self.frame_type = "ZPAN"

		if (fcf >> 11) & 1:
			# extended dest is present
			self.dst_addr = b.data(8)
		if (fcf >> 12) & 1:
			# extended source is present
			self.src_addr = b.data(8)

		if fcf & 0x0200:
			# security header is present, attempt to decrypt
			# the message and
			self.ccm_decrypt(b, auth_start)
		else:
			# the rest of the packet is the payload
			self.payload = b.data(b.remaining())

	# security header is present; auth_start points to the start
	# of the network header so that the entire MIC can be computed
	def ccm_decrypt(self, b, auth_start):
		# the security control field is not filled in correctly in the header,
		# so it is necessary to patch it up to contain ZBEE_SEC_ENC_MIC32
		# == 5. Not sure why, but wireshark does it.
		b._data[b.offset()] \
		  = (b._data[b.offset()] & ~0x07) | 0x05
		sec_hdr = b.u8()
		sec_counter = b.data(4)
		# 8 byte ieee address, used in the extended nonce
		if sec_hdr & 0x20: # extended nonce bit
			self.src_addr = b.data(8)
		else:
			# hopefully the one in the header was present
			pass

		# The key seq tells us which key is used;
		# should always be zero?
		key_seq = b.u8()

		# section 4.20 says extended nonce is
		# 8 bytes of IEEE address, little endian
		# 4 bytes of counter, little endian
		# 1 byte of patched security control field
		nonce = bytearray(16)
		nonce[0] = 0x01
		nonce[1:9] = self.src_addr
		nonce[9:13] = sec_counter
		nonce[13] = sec_hdr
		nonce[14] = 0x00
		nonce[15] = 0x00

		# authenticated data is everything up to the message
		# which includes the network header.  we stored the
		# offset to the start of the header before processing it,
		# which allows us to extract all that data now.
		l_a = b.offset() - auth_start
		#print("auth=", auth_start, "b=", b._data)
		auth = b._data[auth_start:auth_start + l_a]
		#print("l_a=",l_a, auth)

		# Length of the message is everything that is left,
		# except the four bytes for the message integrity code
		l_M = 4
		l_m = b.len() - (auth_start + l_a) - l_M
		C = b.data(l_m) # cipher text of length l_m
		M = b.data(l_M) # message integrity code of length l_M

		if not CCM.decrypt(auth, C, M, nonce, self.aes):
			print("BAD DECRYPT: ", b._data )
			print("message=", C)
			self.payload = b''
			self.mic = 0
		else:
			self.payload = C
			self.mic = 1

	def show(self):
		print(self.frame_type,
			"fcf=%04x seq=%02x" % (self.fcf, self.seq),
			"dst=%04x/%04x src=%04x/%04x" %
			   (self.dst, self.dst_pan, self.src, self.src_pan),
			self.payload
		)
