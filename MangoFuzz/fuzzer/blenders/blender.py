import random
import string

class Blender(object):

	def __init__(self, engine_obj):
		"""
			Create the new Blender object.
		:param engine_obj: the target engine object to be used by the Blender.
		"""
		self.curr_engine = engine_obj

	def getRandBytes(self, num_bytes=None):
		if num_bytes is None:
			num_bytes = random.randint(1, 1024)
		data = ''
		for x in range(num_bytes):
			data += chr(random.randint(0, 0xff))
		return data

	def getRandNum(self, bit_size):
		val = random.randint(0, 2**bit_size-1)
		return val

	def getRandString(self, str_len):
		target_len = int(str_len)
		if target_len <= 0:
			target_len = 4
		return ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits)
					   for _ in range(target_len))


	def getSupportedTypes(self):
		"""
			Gets all the supported types by this Blender.
		:return: list of type names it supports.
		"""
		raise NotImplementedError("Not implemented")

	def blend(self, old_data, *additional_data):
		"""
			Performs the mutation.
			Given old value and optional additional data,
			it performs mutation and returns new value.
		:param old_data: old data or None
		:param additional_data: optional var args specific to each mutator.
		:return: New mutated data.
		"""
		raise NotImplementedError("Not implemented")

	def canHandle(self, target_type):
		"""
			This function checks if this Punker can handle the provided type or not.
		:param target_type: interested type.
		:return: true/false depending on whether the mutator can handle this or not.
		"""
		raise NotImplementedError("Not implemented")
