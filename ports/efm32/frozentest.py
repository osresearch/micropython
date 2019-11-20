
#!/usr/bin/env python
"""
    Copyright (C) 2012 Bo Zhu http://about.bozhu.me

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
"""

Sbox = (
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
)

InvSbox = (
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
    0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
    0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D,
)


# learnt from http://cs.ucsb.edu/~koc/cs178/projects/JT/aes.c
xtime = lambda a: (((a << 1) ^ 0x1B) & 0xFF) if (a & 0x80) else (a << 1)


Rcon = (
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
    0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A,
    0x2F, 0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A,
    0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91, 0x39,
)


def text2matrix(text):
    return [
	[text[0], text[1], text[2], text[3]],
	[text[4], text[5], text[6], text[7]],
	[text[8], text[9], text[10], text[11]],
	[text[12], text[13], text[14], text[15]],
    ]


def matrix2text(matrix):
    text = bytearray(16)
    for i in range(4):
        for j in range(4):
            text[4*i+j] = matrix[i][j]
    return text


class AES:
    def __init__(self, master_key):
        self.change_key(master_key)

    def change_key(self, master_key):
        self.round_keys = text2matrix(master_key)

        for i in range(4, 4 * 11):
            self.round_keys.append([])
            if i % 4 == 0:
                byte = self.round_keys[i - 4][0]        \
                     ^ Sbox[self.round_keys[i - 1][1]]  \
                     ^ Rcon[i >> 2]
                self.round_keys[i].append(byte)

                for j in range(1, 4):
                    byte = self.round_keys[i - 4][j]    \
                         ^ Sbox[self.round_keys[i - 1][(j + 1) % 4]]
                    self.round_keys[i].append(byte)
            else:
                for j in range(4):
                    byte = self.round_keys[i - 4][j]    \
                         ^ self.round_keys[i - 1][j]
                    self.round_keys[i].append(byte)

        # print self.round_keys

    def encrypt(self, plaintext):
        self.plain_state = text2matrix(plaintext)

        self.__add_round_key(self.plain_state, self.round_keys[:4])

        for i in range(1, 10):
            self.__round_encrypt(self.plain_state, self.round_keys[4 * i : 4 * (i + 1)])

        self.__sub_bytes(self.plain_state)
        self.__shift_rows(self.plain_state)
        self.__add_round_key(self.plain_state, self.round_keys[40:])

        return matrix2text(self.plain_state)

    def decrypt(self, ciphertext):
        self.cipher_state = text2matrix(ciphertext)

        self.__add_round_key(self.cipher_state, self.round_keys[40:])
        self.__inv_shift_rows(self.cipher_state)
        self.__inv_sub_bytes(self.cipher_state)

        for i in range(9, 0, -1):
            self.__round_decrypt(self.cipher_state, self.round_keys[4 * i : 4 * (i + 1)])

        self.__add_round_key(self.cipher_state, self.round_keys[:4])

        return matrix2text(self.cipher_state)

    def __add_round_key(self, s, k):
        for i in range(4):
            for j in range(4):
                s[i][j] ^= k[i][j]


    def __round_encrypt(self, state_matrix, key_matrix):
        self.__sub_bytes(state_matrix)
        self.__shift_rows(state_matrix)
        self.__mix_columns(state_matrix)
        self.__add_round_key(state_matrix, key_matrix)


    def __round_decrypt(self, state_matrix, key_matrix):
        self.__add_round_key(state_matrix, key_matrix)
        self.__inv_mix_columns(state_matrix)
        self.__inv_shift_rows(state_matrix)
        self.__inv_sub_bytes(state_matrix)

    def __sub_bytes(self, s):
        for i in range(4):
            for j in range(4):
                s[i][j] = Sbox[s[i][j]]


    def __inv_sub_bytes(self, s):
        for i in range(4):
            for j in range(4):
                s[i][j] = InvSbox[s[i][j]]


    def __shift_rows(self, s):
        s[0][1], s[1][1], s[2][1], s[3][1] = s[1][1], s[2][1], s[3][1], s[0][1]
        s[0][2], s[1][2], s[2][2], s[3][2] = s[2][2], s[3][2], s[0][2], s[1][2]
        s[0][3], s[1][3], s[2][3], s[3][3] = s[3][3], s[0][3], s[1][3], s[2][3]


    def __inv_shift_rows(self, s):
        s[0][1], s[1][1], s[2][1], s[3][1] = s[3][1], s[0][1], s[1][1], s[2][1]
        s[0][2], s[1][2], s[2][2], s[3][2] = s[2][2], s[3][2], s[0][2], s[1][2]
        s[0][3], s[1][3], s[2][3], s[3][3] = s[1][3], s[2][3], s[3][3], s[0][3]

    def __mix_single_column(self, a):
        # please see Sec 4.1.2 in The Design of Rijndael
        t = a[0] ^ a[1] ^ a[2] ^ a[3]
        u = a[0]
        a[0] ^= t ^ xtime(a[0] ^ a[1])
        a[1] ^= t ^ xtime(a[1] ^ a[2])
        a[2] ^= t ^ xtime(a[2] ^ a[3])
        a[3] ^= t ^ xtime(a[3] ^ u)


    def __mix_columns(self, s):
        for i in range(4):
            self.__mix_single_column(s[i])


    def __inv_mix_columns(self, s):
        # see Sec 4.1.3 in The Design of Rijndael
        for i in range(4):
            u = xtime(xtime(s[i][0] ^ s[i][2]))
            v = xtime(xtime(s[i][1] ^ s[i][3]))
            s[i][0] ^= u
            s[i][1] ^= v
            s[i][2] ^= u
            s[i][3] ^= v

        self.__mix_columns(s)

# This is the "well-known" zigbee2mqtt key
# The ikea key is not known, might be per gateway
nwk_key = b"\x01\x03\x05\x07\x09\x0b\x0d\x0f\x00\x02\x04\x06\x08\x0a\x0c\x0d"

# Only need one AES object in ECB mode since there is no
# state to track
aes = AES(nwk_key)

class packet:
	def __init__(self):
		self._offset = 0
		self._data = None
	def init(self, data):
		self._offset = 0
		self._data = data
	def len(self):
		return len(self._data)
	def offset(self):
		return self._offset
	def remaining(self):
		return self.len() - self.offset()
	def u8(self):
		x = self._data[self._offset]
		self._offset += 1
		return x
	def u16(self):
		x = 0 \
		  | self._data[self._offset+1] << 8 \
                  | self._data[self._offset+0] << 0
		self._offset += 2
		return x
	def u32(self):
		x = 0 \
		  | self._data[self._offset+3] << 24 \
		  | self._data[self._offset+2] << 16 \
		  | self._data[self._offset+1] << 8 \
                  | self._data[self._offset+0] << 0
		self._offset += 4
		return x
	def data(self, len):
		x = self._data[self._offset:self._offset+len]
		self._offset += len
		return x


class ieee802154:
	def __init__(self):
		pass

	# parse an incoming 802.15.4 message
	def parse(self,b):
		self.fcf = b.u16()
		self.seq = b.u8()
		self.frame_type = (self.fcf >> 0) & 0x7

		dst_mode = (self.fcf >> 10) & 0x3
		src_mode = (self.fcf >> 14) & 0x3
		self.src = 0
		self.dst = 0
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
		xor = aes.encrypt(nonce)
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
			xor = aes.encrypt(nonce)
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
		block = aes.encrypt(block)

		# process the auth length and auth data blocks
		j = 0
		if l_a > 0:
			block[0] ^= (l_a >> 8) & 0xFF
			block[1] ^= (l_a >> 0) & 0xFF
			j = 2
			for i in range(l_a):
				if (j == 16):
					block = aes.encrypt(block)
					j = 0
				block[j] ^= auth[i]
				j += 1
			# pad out the rest of this block with 0
			j = 16

		# process the clear text message blocks
		if l_m > 0:
			for i in range(l_m):
				if (j == 16):
					block = aes.encrypt(block)
					j = 0
				block[j] ^= m[i]
				j += 1
		if j != 0:
			block = aes.encrypt(block)

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

def loop():
	parser = ieee802154()
	data = packet()
	while True:
		b = radio.get()
		if b is None:
			continue
		data.init(b[:-2]) # discard the weird bytes
		try:
			parser.parse(data)
			parser.show()
		except:
			print(b[:-2])
			raise

def sniff():
	while True:
		bytes = radio.get();
		if bytes is None:
			continue
		for c in bytes:
			print("%02x" % (c), end='')
		print()
