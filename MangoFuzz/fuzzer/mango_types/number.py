from data_guys import DataElement
from ..utils import rotten_peel
import random
import string
import struct

class Number(DataElement):
	def __init__(self, name, size, engine, parent=None):
		DataElement.__init__(self, name, engine, parent)
		self.data_type = "Number"
		# size in bits
		if size%8 != 0:
			rotten_peel("Odd sized number: %s %s",name, size)

		self.bit_size = size
		self.has_default_val = False
		self.default_val = None
		self.signed = False

		# preset vals
		self.small_sizes = [1,8,16,32,64,128,512,1024,7,15,31,63,127,511,1012]
		self.tiny_sizes = [1,8,16,32,64,128,255,7,15,63,127]

		# fmts
		self.pack_formats = {8:'B', 16:'H', 32:'I', 64:'Q'}

	def copy(self, visited=None):
		new_number = Number(self.name, self.bit_size, self.engine, self.parent)
		new_number.data_type = "Number"
		new_number.has_default_val = self.has_default_val
		new_number.default_val = self.default_val
		return new_number

	def generate_old(self):
		# If there's a default value from the jpit, use that
		if self.default_val is not None:
			val =  self.default_val

		else:
			# reset signed to False
			self.signed = False
			chance = random.random()
			# 5% chance to be 0
			if chance < 0.05:
				self.value = 0
				val =  0

			# 5% chance to be -1
			elif chance < 0.10:
				self.value = -1
				self.signed = True
				val = -1

			# 30% chance to be a "small" number
			elif chance < 0.40:
				if self.bit_size == 8:
					val = random.choice(self.tiny_sizes)
				else:
					val = random.choice(self.small_sizes)
				self.value = val

			# 60% chance most likely a big ass number
			else:
				val = random.randint(0, 2**self.bit_size-1)
				self.value = val

		# pack the data
		fmt = self.pack_formats[self.bit_size]
		if self.signed:
			fmt = string.lower(fmt)

		# TODO: add big endian support
		# little endian by default
		fmt = '<' + fmt
		try:
			val = struct.pack(fmt, val)
		except:
			print fmt
			print val
			import ipdb;ipdb.set_trace()

		# target_data, additional_blobs, target_mappings, child_mappings
		return val, {}, {}, {}

	def generate(self):
		# If there's a default value from the jpit, use that
		if self.default_val is not None:
			val =  self.default_val

		else:
			matching_blenders = self.engine.blender_factory.getMatchingBlenders(self.data_type)
			if len(matching_blenders) == 0:
				rotten_peel("No blenders available for data type: %s", self.data_type)

			blender_pick = random.choice(matching_blenders)
			val = blender_pick.blend(self.value, self.bit_size)

		# update our value
		self.value = val
		# pack the data
		fmt = self.pack_formats[self.bit_size]
		# signed
		if val < 0:
			fmt = string.lower(fmt)

		# TODO: add big endian support
		# little endian by default
		fmt = '<' + fmt
		try:
			val = struct.pack(fmt, val)
		except:
			print fmt
			print val
			import ipdb;ipdb.set_trace()

		# target_data, additional_blobs, target_mappings, child_mappings
		self.value = val
		return val, {}, {}, {}

	def blend(self):
		target_data = None
		# get all available blenders.
		all_available_blenders = self.blender_factory.getMatchingBlenders(self.data_type)
		if len(all_available_blenders) == 0:
			rotten_peel("No blenders available for data type: %s", self.data_type)
		else:
			# pick a random blender.
			blender_pick = self.engine_obj.getRandomPick(all_available_blenders)
			# get the blender value
			target_data = blender_pick.blend(self.value, self.bit_size/8)
			# assign old data.
			self.value = target_data

		return target_data, {}, {}, {}

	def getSizeBytes(self):
		size_bytes = self.bit_size/8
		return size_bytes

	def setDefaultValue(self, value):
		self.default_val = value
		self.value = value
		self.has_default_val = True

