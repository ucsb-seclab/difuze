from blender import Blender
import random
from ..utils import *


class NumBlender(Blender):
	"""
		Number Blender.
		This generates random number of provided size.
	"""
	supported_types = ["Number"]

	def __init__(self, engine_obj):
		super(NumBlender, self).__init__(engine_obj)
		thick_peel("Created a NumBlender")

	def getSupportedTypes(self):
		return list(NumBlender.supported_types)

	def blend(self, old_data, *additional_data):
		# this guy expects the number of bytes to be one
		# of the argument
		if len(additional_data) > 0:
			num_bits = additional_data[0]
		else:
			# if no size is provided they use a default size
			num_bits = 32
			thick_peel("Called NumBlender without size, defaulting to:%d", num_bits)
		to_ret = None
		small_sizes = [1, 8, 16, 32, 64, 128, 512, 1024, 7, 15, 31, 63, 127, 511, 1012]
		#tiny_sizes = [1, 8, 16, 32, 64, 128, 255, 7, 15, 63, 127]
		tiny_sizes = [128, 127, 64, 63, 32, 31, 16, 15, 8, 7, 4, 3, 2, 1, 0, 255, -1]

		chance = random.random()
		# 10% chance to be 0
		if chance < .10:
			to_ret = 0
		elif chance < .20:
			to_ret = -1
		# 30% chance to be a small number
		elif chance < .50:
			if num_bits == 8:
				to_ret = random.choice(tiny_sizes)
			else:
				to_ret = random.choice(small_sizes)
		# 50% chance to be a bigass number (probably)
		else:
			to_ret = self.getRandNum(num_bits)
		return to_ret

	def canHandle(self, target_type):
		return target_type in NumBlender.supported_types
