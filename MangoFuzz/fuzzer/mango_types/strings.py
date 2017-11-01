from data_guys import DataElement
from ..utils import rotten_peel
import random

class String(DataElement):

	def __init__(self, name, length, engine, parent=None):
		DataElement.__init__(self, name, engine, parent)
		self.data_type = "String"
		# length in bytes
		self.length = length

	def copy(self, visited=None):
		new_string = String(self.name, self.length, self.engine, self.parent)
		new_string.data_type = "String"
		return new_string

	def generate_old(self):
		val = ''
		for x in range(self.length):
			val += chr(random.randint(0, 0xff))
		self.value = val

		# target_data, additional_blobs, target_mappings, child_mappings
		return val, {}, {}, {}

	def generate(self):
		# get string blenders
		matching_blenders = self.engine.blender_factory.getMatchingBlenders(self.data_type)
		if len(matching_blenders) == 0:
			rotten_peel("No blenders available for data type: %s", self.data_type)
		# randomly choose one
		blender_pick = random.choice(matching_blenders)
		val = blender_pick.blend(self.value, self.length)
		self.value = val

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
			target_data = blender_pick.blend(self.value, self.length)
			# assign old data.
			self.value = target_data

		return target_data, {}, {}, {}

	def getSizeBytes(self):
		return self.length

