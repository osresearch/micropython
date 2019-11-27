# Process IEEE 802.15.4 packets and return an unpacked form of them
# The Zigbee specific stuff should be split out into a separate file
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

		#print("l_c=", l_m, C)

		# pad the cipher text to 16 bit block
		while len(C) % 16 != 0:
			C.append(0)

		# decrypt the MIC block in place
		xor = self.aes.encrypt(nonce)
		for j in range(l_M):
			M[j] ^= xor[j]

		# decrypt each 16-byte block in counter mode
		for i in range(0, l_m, 16):
			block_len = l_m - i
			if block_len > 16:
				block_len = 16

			# increment the counter word,
			# should be two bytes, but never more than 256
			nonce[15] += 1
			xor = self.aes.encrypt(nonce)
			for j in range(block_len):
				C[i+j] ^= xor[j]

		# remove any padding from the decrypted message
		# and store the clear text payload
		m = C[0:l_m];
		#print("l_m=", l_m, m)

		# check the message integrity code with the messy CCM*
		# algorithm

		# Generate the first cipher block B0
		block = bytearray(16)
		block[0] |= 1 << 3 # int((4 - 2)/2) << 3
		if l_a != 0:
			block[0] |= 0x40
		block[0] |= 1
		block[1:14] = nonce[1:14] # src addr and counter
		block[14] = (l_m >> 8) & 0xFF
		block[15] = (l_m >> 0) & 0xFF
		block = self.aes.encrypt(block)

		# process the auth length and auth data blocks
		j = 0
		if l_a > 0:
			block[0] ^= (l_a >> 8) & 0xFF
			block[1] ^= (l_a >> 0) & 0xFF
			j = 2
			for i in range(l_a):
				if (j == 16):
					block = self.aes.encrypt(block)
					j = 0
				block[j] ^= auth[i]
				j += 1
			# pad out the rest of this block with 0
			j = 16

		# process the clear text message blocks
		if l_m > 0:
			for i in range(l_m):
				if (j == 16):
					block = self.aes.encrypt(block)
					j = 0
				block[j] ^= m[i]
				j += 1
		if j != 0:
			block = self.aes.encrypt(block)

		if block[0:l_M] == M:
			#print("Good MAC:", M)
			self.payload = m
			self.mic = 1
		else:
			print("Bad MAC:",block[0:l_M], b._data)
			self.payload = C
			self.mic = 0


	def show(self):
		print(self.frame_type,
			"fcf=%04x seq=%02x" % (self.fcf, self.seq),
			"dst=%04x/%04x src=%04x/%04x" %
			   (self.dst, self.dst_pan, self.src, self.src_pan),
			self.payload
		)
