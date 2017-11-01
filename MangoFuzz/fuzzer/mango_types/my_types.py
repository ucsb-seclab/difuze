import sys
import struct
import string
import xml.etree.ElementTree as ET
import ipdb

# TODO: import engine..
import random
random.seed(7)

def error(msg):
	print msg
	ipdb.set_trace()
	sys.exit(1)

class JPit():
	def __init__(self, name):
		self.name = name
		self.dms = []

	def __getitem__(self, idx):
		return self.dms[idx]

	def __len__(self):
		return len(self.dms)

	def findDataModel(self, dm_name):
		for dm in self.dms:
			if dm.name == dm_name:
				return dm
		return None

	def addChild(self, dm):
		if isinstance(dm, DataModel):
			self.dms.append(dm)
			dm.parent = self
		else:
			error("Trying to add a non DataModel to a JPit")

class DataModel():
	def __init__(self, name, size, etree, etree_dm):
		self.name = name
		# DataElements that makeup the DataModel
		self.elements = []
		# size in bytes
		self.size = size
		# the etree
		self.etree = etree
		# the etree elem
		self.etree_dm = etree_dm
		
		self.parent = None

	def __getitem__(self, idx):
		return self.elements[idx]

	def __len__(self):
		return len(self.elements)

	def copy(self, visited=None):
		if visited is None:
			visited = []

		visited.append(self)
		new_data_model = DataModel(self.name, self.size, self.etree, self.etree_dm)
		new_data_model.parent = self.parent

		for element in self.elements:
			new_data_model.addChild(element.copy(visited))

		visited.remove(self)
		return new_data_model

	def getRoot(self):
		return self.etree.getroot()

	def addChild(self, data_element):
		if isinstance(data_element, DataElement):
			self.elements.append(data_element)
			data_element.parent = self
		else:
			error("Trying to add a non data_element to a DataModel")

	def generate(self):
		our_data = ''
		our_blobs = {}
		our_mappings = {}
		additional_mappings = {}
		for element in self:
			ele_data, ele_blobs, ele_mappings, ele_blob_mappings = element.generate()
			# element offset within the dm
			ele_offset = len(our_data)
			# add the data
			our_data += ele_data
			# update our_blobs with the element blobs
			our_blobs.update(ele_blobs)
			if len(ele_mappings) > 0:
				for ele_data_offset in ele_mappings:
					our_mappings[ele_offset + ele_data_offset] = ele_mappings[ele_data_offset]

			# add the ele_blob_mappings to our additional mappings
			additional_mappings.update(ele_blob_mappings)

		return our_data, our_blobs, our_mappings, additional_mappings


	def getValue(self):
		cur_data = ''
		for e in self.elements:
			cur_data += e.demandValue()
		return cur_data


class DataElement():
	def __init__(self, name, parent=None):
		self.name = name
		self.parent = parent
		self.value = None

		self.mutable = True
		self.occurs = 1
		self.minOccurs = 1
		self.maxOccurs = 1
		self.children = []

	def __getitem__(self, idx):
		return self.children[idx]

	def __len__(self):
		return len(self.children)

	def copy(self):
		raise Exception("Copy not implemented for this class")

	def generate(self):
		raise Exception("Generate not implemented for this class")

	def getDataModel(self):
		parent = self.parent
		while (isinstance(parent, DataModel) == False):
			parent = parent.parent

		return parent

	def addChild(self, child_node):
		if child_node is None:
			error("Trying to append an empty child!")
		
		# set the childs parent
		child_node.parent = self
		# add it to our children array
		self.children.append(child_node)

	def getValue(self):
		return self.value

	def demandValue(self):
		if self.value is None:
			val = self.generate()
			return val
		else:
			return self.value

	def getSizeBytes(self):
		raise Exception("Class has no implementation of getSizeBytes()")


class Block(DataElement):
	def __init__(self, name, parent=None, ref=None, occurs=1):
		DataElement.__init__(self, name, parent)
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
		else:
			self.is_array = False

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_block = Block(self.name, self.parent, self.ref, self.occurs)

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
			return self.ref.generate()
		
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

		return our_data, to_ret_blobs, our_mappings, additional_mappings

	def getSizeBytes(self):
		if self.is_ref:
			return self.ref.size
		else:
			size = 0
			for child in self:
				size += child.getSizeBytes()
			return size

class String(DataElement):
	def __init__(self, name, length, parent=None):
		DataElement.__init__(self, name, parent)
		self.data_type = "String"
		# length in bytes
		self.length = length

	def copy(self, visited=None):
		new_string = String(self.name, self.length, self.parent)
		new_string.data_type = "String"
		return new_string

	def generate(self):
		val = ''
		for x in range(self.length):
			val += chr(random.randint(0, 0xff))
		self.value = val

		# target_data, additional_blobs, target_mappings, child_mappings
		return val, {}, {}, {}

	def getSizeBytes(self):
		return self.length


class Number(DataElement):
	def __init__(self, name, size, parent=None):
		DataElement.__init__(self, name, parent)
		self.data_type = "Number"
		# size in bits
		if size%8 != 0:
			error("Odd sized number: %s %s" % (name, size))

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
		new_number = Number(self.name, self.bit_size, self.parent)
		new_number.data_type = "Number"
		new_number.has_default_val = self.has_default_val
		new_number.default_val = self.default_val
		return new_number

	def generate(self):
		# If there's a default value from the jpit, return that
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

	def getSizeBytes(self):
		size_bytes = self.bit_size/8
		return size_bytes

	def setDefaultValue(self, value):
		self.default_value = value
		self.value = value
		self.has_default_val = True


class Choice(DataElement):
	def __init__(self, name, num_choices, choice_type, parent=None):
		DataElement.__init__(self, name, parent)
		self.data_type = "Choice"
		# Number of choices
		self.num_choices = num_choices
		# enum or union
		self.choice_type = choice_type

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_choice = Choice(self.name, self.num_choices, self.choice_type, self.parent)
		new_choice.data_type = "Choice"

		for thing in self:
			new_choice.addChild(thing.copy(visited))
		visited.remove(self)
		return new_choice

	def getSizeBytes(self):
		some_choice = self.children[0]
		return some_choice.getSizeBytes()


class Enum(Choice):
	def __init__(self, name, num_choices, choice_type, parent=None):
		Choice.__init__(self, name, num_choices, choice_type, parent)
		self.data_type = "Enum"

	# all enum choices should have default vals
	def generate(self):
		# 5% chance to be something other than one of the predefined vals
		if random.random() < 0.05:
			size = self[0][0].bit_size
			val = random.randint(0, 2**size-1)

		else:
			val = random.choice(self)[0].default_value

		self.value = val
		if val < 1:
			fmt = "<I"
		else:
			fmt = "<i"

		# pack it for generation
		val = struct.pack(fmt, val)
		return val, {}, {}, {}

class Union(Choice):
	def __init__(self, name, num_choices, choice_type, parent=None):
		Choice.__init__(self, name, num_choices, choice_type, parent)
		self.data_type = "Union"

	def generate(self):
		pick = random.choice(self)
		print "PICKED %s" % pick.name
		return pick.generate()


class Pointer(DataElement):
	def __init__(self, name, ptr_to, ptr_depth, length, parent=None):
		DataElement.__init__(self, name, parent)
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

	def copy(self, visited=None):
		if visited is None:
			visited = []

		new_pointer = Pointer(self.name, self.ptr_to, self.ptr_depth, self.length, self.parent)
		new_pointer.is_recursive = self.is_recursive

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

		for x in range(self.length):
			our_data += chr(random.randint(0, 0xff))
		self.value = our_data

		# Check if we're recursive. If so, don't generate
		if self.is_recursive:
			return our_data, {}, {}, {}

		# Check if we're a generic ptr, if so, generate some generic data..
		if self.is_generic_ptr:
			#ipdb.set_trace()
			data = "A"*20
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
	
	def getSizeBytes(self):
		return self.length


