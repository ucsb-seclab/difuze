from blender import Blender
import random
from ..utils import *


class BlobBlender(Blender):
	"""
		Blob Blender.
		This generates random blobs of different data types (arrays).
		This is primarly to be used for creating blobs for generic data pointers.
	"""
	supported_types = ["Blob"]

	def __init__(self, engine_obj):
		super(BlobBlender, self).__init__(engine_obj)
		thick_peel("Created a BlobBlender")

	def getSupportedTypes(self):
		return list(BlobBlender.supported_types)

	def blend(self, old_data, *additional_data):
		# we expect the base type to be an arg
		if len(additional_data) == 2:
			base_type = additional_data[0]
			elem_size = additional_data[1]
		else:
			rotten_peel("BlobBlender called with incorrect number of args!")

		data = None
		# for now, just be stupid
		if base_type == "void":
			data = "\xcc"*20

		else:
			size = random.randint(1,256)
			data = self.getRandBytes(size)

		return data

	def canHandle(self, target_type):
		return target_type in BlobBlender.supported_types
