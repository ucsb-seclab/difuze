from juicer import Juicer
from . import registerJtype
import socket
import time
import random
import struct
from ..utils import *

p = lambda x: struct.pack("<I", x)
u = lambda x: struct.unpack("<I", x)[0]

class TcpJuicer(Juicer):
	"""
		This juicer sends the data over tcp
	"""
	def __init__(self, name, address=None, port=None):
		assert address is not None
		assert port is not None
		Juicer.__init__(self,name)
		self.jtype = 'tcp'
		self.address = address
		self.port = int(port)
		self.socket = None
		self.is_connected = False
		
	def connect(self):
		if self.is_connected:
			rotten_peel("TcpJuicer: %s trying to connect cut already connected!", self.name)
		if self.socket is None:
			self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			raw_peel("TcpJuicer: created socket.")

		for attempt in range(3):
			if self.is_connected:
				break
			try:
				self.socket.connect((self.address, self.port))
				self.is_connected = True
			except Exception:
				raw_peel("TcpJuicer: Failed connecting to %s:%d   Retrying...", self.address, self.port)
				time.sleep(3)

		if self.is_connected:
			raw_peel("TcpJuicer: Sucessfully connected.")
			return 0
		else:
			raw_peel("TcpJuicer: Failed to connect!")
			return -1

	def sendData(self, data):
		if self.is_connected is False:
			self.connect()
			self.is_connected = True

		self.socket.sendall(data)

	def sendDataResp(self, data, resp, size):
		if self.is_connected is False:
			self.connect()
			self.is_connected = True

		self.socket.sendall(data)
		data = self.socket.recv(size)
		if data != resp:
			return -1
			#rotten_peel("Didn't get proper response. Expected: %s. Got: %s", resp, data)
		return 0

	def send(self, blob_arr, map_arr, target_struct, devname, ioctl_id):
		# seed for the executor
		seed = random.randint(0, 2**32-1)
		data = ""
		data += p(seed)
		data += p(ioctl_id)
		data += p(len(map_arr))
		data += p(len(blob_arr))
		data += p(len(devname))
		data += devname

		for entry in map_arr:
			src_idx = entry.src_idx
			data += p(src_idx)

			dst_idx = entry.dst_idx
			data += p(dst_idx)

			offset = entry.offset
			data += p(offset)

		for blob in blob_arr:
			data += p(len(blob))

		for blob in blob_arr:
			data += blob

		size = len(data)
		size = p(size)
		self.sendData(size)
		#self.sendData(data)
		err = self.sendDataResp(data, size, 4)
		if err:
			print "Possible crash!"
			print "seed: %d" % seed
			print "devname: %s" % devname
			print "ioctl_id: %d" % ioctl_id
			print "target_struct: %s" % target_struct
			print "data: %s" % data.encode('hex')
			rotten_peel("Possible crash")


	def juice(self, data):
		to_ret = None
		if self.console_print:
			# TODO: print the output in hex on console
			pass
		self.sendData(data)

	def getName(self):
		return self.name


# Must regsiter our type
registerJtype('tcp', TcpJuicer)
