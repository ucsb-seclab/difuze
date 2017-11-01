from data_guys import DataElement
import random

class Block(DataElement):

	def __init__(self, name, engine, parent=None, ref=None, occurs=1):
		DataElement.__init__(self, name, engine, parent)
		self.data_type = "Block"
		# is this a reference to another DM?
		self.ref = ref
		if ref is None:
			self.is_ref = False
		else:
			self.is_ref = True
			self.ref.parent = self

		self.occurs = occurs
		if occurs > 1:
			self.is_array = True
			self.min_occurs = occurs
			self.max_occurs = occurs
		else:
			self.is_array = False

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_block = Block(self.name, self.engine, self.parent, self.ref, self.occurs)

		for child in self:
			new_block.addChild(child.copy(visited))

		visited.remove(self)
		return new_block

	def generate(self):
		# target_data, additional_blobs, target_mappings, child_mappings
		our_data = ''
		to_ret_blobs = {}
		our_mappings = {}
		additional_mappings = {}

		# if we're a ref, call generate() on whatever DM we're a ref to
		if self.is_ref:
			#target_data, additional_blobs, target_mappings, child_mappings = self.ref.generate()
			stuff = self.ref.generate()
			self.value = stuff[0]
			return stuff
		
		for child in self:
			child_data, child_additional_blobs, child_data_mappings, child_blob_mappings = child.generate()
			# get the child offset in our data
			child_offset = len(our_data)
			# add the child data to our data
			our_data += child_data
			# get a blob id for the child data
			child_blob_id = child.name + '_' + str(id(child))
			# update to_ret_blobs with any additional child_blobs
			to_ret_blobs.update(child_additional_blobs)
			if len(child_data_mappings) > 0:
				for child_mapping_offset in child_data_mappings:
					our_mappings[child_offset+child_mapping_offset] = child_data_mappings[child_mapping_offset]

			# add the childs blob mappings to our additional mappings
			additional_mappings.update(child_blob_mappings)
		self.value = our_data
		return our_data, to_ret_blobs, our_mappings, additional_mappings

	def blend(self):
		target_data = ''
		to_ret_blobs = dict()
		target_data_mappings = dict()
		target_blob_mappings = dict()
		for child in self:
			child_data, child_blobs, child_data_mappings, child_blob_mappings = child.blend()
			child_index = len(target_data)
			# add child data
			target_data += child_data
			# add blobs
			to_ret_blobs.update(child_blobs)
			# if there are any mappings for the child data
			if len(child_data_mappings) != 0:
				# update the mapping using child index.
				for curr_offset in child_data_mappings:
					target_data_mappings[curr_offset + child_index] = child_data_mappings[curr_offset]

			# Add child blob mappings
			target_blob_mappings.update(child_blob_mappings)

		return target_data, to_ret_blobs, target_data_mappings, target_blob_mappings

	def getSizeBytes(self):
		if self.is_ref:
			return self.ref.size
		else:
			size = 0
			for child in self:
				size += child.getSizeBytes()
			return size

