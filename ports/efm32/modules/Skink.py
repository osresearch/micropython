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
			#print("------")
			# discard the weird bytes, not FCS, not sure what they are
			data = b[:-2]
			#print(data)
			ieee = IEEE802154.IEEE802154(data=data)
			#print(ieee)
			if ieee.frame_type != IEEE802154.FRAME_TYPE_DATA:
				continue
			nwk = ZigbeeNetwork.ZigbeeNetwork(data=ieee.payload, aes=aes, validate=False)
			ieee.payload = nwk
			#print(nwk)
			if nwk.frame_type != ZigbeeNetwork.FRAME_TYPE_DATA:
				continue
			aps = ZigbeeApplication.ZigbeeApplication(data=nwk.payload)
			nwk.payload = aps
			#print(aps)
			if aps.frame_type != ZigbeeApplication.FRAME_TYPE_DATA:
				continue

			# join requests are "special" for now
			if aps.cluster == 0x36:
				continue

			zcl = ZigbeeCluster.ZigbeeCluster(data=aps.payload)
			aps.payload = zcl

			print("%02x %04x%c %04x: cluster %04x:%02x/%d" % (
				nwk.seq,
				nwk.src,
				' ' if nwk.src == ieee.src else '+', # repeater
				nwk.dst,
				aps.cluster,
				zcl.command,
				zcl.direction,
			))
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


seq = 99
pan = 0x1a62
mac = Radio.mac()
beacon_packet = IEEE802154.IEEE802154(frame_type=3, seq=99, command=7, dst=0xffff, dst_pan=0xffff)

def tx(msg):
	print("-->", msg)
	b = msg.serialize()
	#print(b)
	Radio.tx(b)

def beacon():
	tx(beacon_packet)

def discover_pan():
	for i in range(10):
		beacon()
		for i in range(1000000):
			b = Radio.rx()
			if b is None:
				continue
			ieee = IEEE802154.IEEE802154(data=b[:-2])
			print("<--", ieee)
			if ieee.frame_type == IEEE802154.FRAME_TYPE_BEACON:
				return ieee.src_pan
	return None
	

def wait_ack():
	for i in range(1000000):
		b = Radio.rx()
		if b is None:
			continue
		ieee = IEEE802154.IEEE802154(data=b[:-2])
		print("<--", ieee)
		if ieee.frame_type == IEEE802154.FRAME_TYPE_ACK:
			return True
	return False

def leave():
	global seq
	tx(IEEE802154.IEEE802154(
		frame_type=IEEE802154.FRAME_TYPE_DATA,
		seq=seq,
		dst=0x0000,
		dst_pan=pan,
		src=0x333d,
		payload=ZigbeeNetwork.ZigbeeNetwork(
			frame_type=ZigbeeNetwork.FRAME_TYPE_CMD,
			version=2,
			radius=1,
			seq=12,
			dst=0xfffd,
			src=mac,
			discover_route=0,
			security=1,
			sec_seq=bytearray(b';\x00\x00\x00'),
			sec_key=1,
			sec_key_seq=0,
			ext_src=mac,
			payload=bytearray(b'\x04\x00')
		),
	))
	seq += 1

def join():
	global seq

	Radio.promiscuous(False)
	pan = discover_pan()
	if pan is None:
		print("No PAN received?")
		return False

	print("PAN=0x%04x" % (pan))

	tx(IEEE802154.IEEE802154(
		frame_type	= IEEE802154.FRAME_TYPE_CMD,
		command		= IEEE802154.COMMAND_JOIN_REQUEST,
		seq		= seq,
		src		= mac,
		src_pan		= 0xFFFF,
		dst		= 0x0000,
		dst_pan		= pan,
		ack_req		= True,
		payload		= b'\x80', # Battery powered, please allocate address
	))
	seq = seq + 1

	if not wait_ack():
		print("No ack received?")
		return False

	tx(IEEE802154.IEEE802154(
		frame_type	= IEEE802154.FRAME_TYPE_CMD,
		command		= IEEE802154.COMMAND_DATA_REQUEST,
		seq		= seq,
		src		= mac,
		dst		= 0x0000,
		dst_pan		= pan,
		payload		= b'',
	))
	seq = seq + 1

	return True
