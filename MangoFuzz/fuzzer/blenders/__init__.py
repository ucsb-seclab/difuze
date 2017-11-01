"""
This package contains all Blenders that are available.
"""
from ..utils import rotten_peel
from num_blender import NumBlender
from string_blender import StringBlender
from blob_blender import BlobBlender


class BlenderFactory(object):
	"""
	Class that implements factory pattern for set of Blenders.
	This is used to access different Blenders.
	"""
	all_blenders = []

	def __init__(self, engine_obj):
		"""
			Initialize all available blenders with the provided engine object.
		:param engine_obj: The engine object that should be used to
				initialize the available blenders.
		:return: None
		"""
		assert engine_obj is not None, "Engine object cannot be None"
		if len(BlenderFactory.all_blenders) == 0:
			# add number blender
			curr_blender = NumBlender(engine_obj)
			BlenderFactory.all_blenders.append(curr_blender)
			# add string blender.
			curr_blender = StringBlender(engine_obj)
			BlenderFactory.all_blenders.append(curr_blender)
			# add blob blender
			curr_blender = BlobBlender(engine_obj)
			BlenderFactory.all_blenders.append(curr_blender)
		else:
			rotten_peel("Blender factory is already initialized.")

	def getAllBlenders(self):
		"""
			Get list of all available Blenders
		:return: list of Blender objects.
		"""
		return list(BlenderFactory.all_blenders)

	def getMatchingBlenders(self, type_name):
		"""
			Get the list of Blenders which can handle provided type.
		:param type_name: Interested type name
		:return: list of Blenders that can handle the type.
		"""
		to_ret = list(filter(lambda x: x is not None and x.canHandle(type_name), BlenderFactory.all_blenders))
		return to_ret

