from blender import Blender
from ..utils import *
import random


class StringBlender(Blender):
	"""
		String Blender.
		This generates random string of provided size.
	"""
	supported_types = ["String"]

	def __init__(self, engine_obj):
		super(StringBlender, self).__init__(engine_obj)
		thick_peel("Created a StringBlender")

	def getSupportedTypes(self):
		return list(StringBlender.supported_types)

	def blend(self, old_data, *additional_data):
		# this guy expects the number of bytes to be one
		# of the argument
		if len(additional_data) > 0:
			num_bytes = int(additional_data[0])
		else:
			num_bytes = random.randint(1, 1024)
			thick_peel("Called StringBlender without size, using:%d", num_bytes)
		# generate random string.
		to_ret = ''
		for x in range(num_bytes):
			to_ret += chr(random.randint(0, 0xff))

		return to_ret

	def canHandle(self, target_type):
		return target_type in StringBlender.supported_types
