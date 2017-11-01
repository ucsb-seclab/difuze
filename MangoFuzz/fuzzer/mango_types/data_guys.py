from ..utils import rotten_peel
import random

class DataElement(object):

	def __init__(self, name, engine, parent=None):
		self.name = name
		self.engine = engine
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
			rotten_peel("Trying to append an empty child!")

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

	def generate(self):
		import ipdb;ipdb.set_trace()
		raise Exception("Class has no implementation of generate()")

	def getSizeBytes(self):
		raise Exception("Class has no implementation of getSizeBytes()")


class DataModel(object):
	def __init__(self, name, size, etree, etree_dm, engine):
		self.name = name
		# DataElements that makeup the DataModel
		self.elements = []
		# size in bytes
		self.size = size
		# the etree
		self.etree = etree
		# the etree elem
		self.etree_dm = etree_dm
		# engine
		self.engine = engine

		self.parent = None

	def __getitem__(self, idx):
		return self.elements[idx]

	def __len__(self):
		return len(self.elements)

	def copy(self, visited=None):
		if visited is None:
			visited = []

		visited.append(self)
		new_data_model = DataModel(self.name, self.size, self.etree, self.etree_dm, self.engine)
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
			rotten_peel("Trying to add a non DataElement to a DataModel")

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
