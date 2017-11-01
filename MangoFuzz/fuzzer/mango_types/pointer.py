from data_guys import DataElement
import random

class Pointer(DataElement):

	def __init__(self, name, ptr_to, ptr_depth, length, engine, parent=None):
		DataElement.__init__(self, name, engine, parent)
		self.name = name
		self.ptr_to = ptr_to
		if ptr_to in ['Number', 'String']:
			self.is_generic_ptr = True
		else:
			self.is_generic_ptr = False
		self.ptr_depth = ptr_depth
		self.length = length
		self.resolved = None
		self.is_recursive = False

		self.ptr_type = None
		self.elem_size = None

	def copy(self, visited=None):
		if visited is None:
			visited = []

		new_pointer = Pointer(self.name, self.ptr_to, self.ptr_depth, self.length, self.engine, self.parent)
		new_pointer.is_recursive = self.is_recursive
		new_pointer.ptr_type = self.ptr_type
		new_pointer.elem_size = self.elem_size

		if self in visited:
			new_pointer.is_recursive = True
			return new_pointer

		visited.append(self)
		if self.resolved is not None:
			new_pointer.resolved = self.resolved.copy(visited)
		visited.remove(self)
		return new_pointer

	def generate(self):
		our_data = ''
		our_mappings = {}
		to_ret_blobs = {}
		additional_mappings = {}
		'''
		for x in range(self.length):
			our_data += chr(random.randint(0, 0xff))
		'''
		# easy to spot
		our_data = "A"*self.length
		self.value = our_data

		# Check if we're recursive. If so, don't generate
		if self.is_recursive:
			our_data = "\x00"*self.length
			self.value = our_data
			return our_data, {}, {}, {}

		# Check if we're a generic ptr, if so, generate some generic data..
		if self.is_generic_ptr:
			matching_blenders = self.engine.blender_factory.getMatchingBlenders("Blob")
			if len(matching_blenders) == 0:
				rotten_peel("No matching blenders for type Blob!")
			blender_pick = random.choice(matching_blenders)
			data = blender_pick.blend(self.value, self.ptr_type, self.elem_size)
			data_id = self.name + '_' + str(id(self))
			our_mappings[0] = data_id
			to_ret_blobs[data_id] = data

		# complex ptr
		else:
			# Okay, we have our actual field data, now call generate() on self.resolved
			# this will be additional_blob(s) and the mappings will go in target_mappings
			if self.resolved is None:
				error("Trying to generate a non resolved pointer!")

			ptr_to = self.resolved.name
			resolved_data, resolved_additional_blobs, resolved_mappings, resolved_additional_mappings = self.resolved.generate()

			# store the resolved data into our to_ret_blobs
			resolved_blob_id = ptr_to + '_' + str(id(self.resolved))
			to_ret_blobs[resolved_blob_id] = resolved_data

			# update to_ret_blobs with any additional blobs we got from the resolve
			to_ret_blobs.update(resolved_additional_blobs)

			# With respect to us, the resolved data is at offset 0
			our_mappings[0] = resolved_blob_id

			if len(resolved_mappings) > 0:
				additional_mappings[resolved_blob_id] = resolved_mappings

			additional_mappings.update(resolved_additional_mappings)

		return our_data, to_ret_blobs, our_mappings, additional_mappings

	def blend(self):
		# random string
		target_data = self.engine_obj.getRandomString(self.length)
		# here get the required blobs
		assert self.resolved is not None, "Pointer should be of certain type"
		# Now blend the resolved type
		dst_data, dst_blobs, dst_data_mappings, dst_blob_mappings = self.resolved.blend()

		# generate random id for the blob
		blob_id = self.engine_obj.getRandomString(5)
		to_ret_blobs = {}
		# sanity, ensure that each blob has different ids.
		assert blob_id not in dst_blobs
		# Add the data of the resolved element into blobs
		to_ret_blobs[blob_id] = dst_data
		curr_data_mappings = dict()
		# Now add the mapping so that the blob is at offset 0
		curr_data_mappings[0] = blob_id

		# OK. Now add all dst_blobs to ret blobs
		to_ret_blobs.update(dst_blobs)
		# Add all mappings
		to_ret_blob_mappings = dict()
		to_ret_blob_mappings[blob_id] = dst_data_mappings
		to_ret_blob_mappings.update(dst_blob_mappings)

		return target_data, to_ret_blobs, curr_data_mappings, to_ret_blob_mappings
	
	def getSizeBytes(self):
		return self.length


