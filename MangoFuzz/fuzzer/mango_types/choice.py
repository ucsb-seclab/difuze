from data_guys import DataElement
import random
import struct

class Choice(DataElement):

	def __init__(self, name, num_choices, choice_type, engine, parent=None):
		DataElement.__init__(self, name, engine, parent)
		self.data_type = "Choice"
		# Number of choices
		self.num_choices = num_choices
		# enum or union
		self.choice_type = choice_type

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_choice = Choice(self.name, self.num_choices, self.choice_type, self.engine, self.parent)
		new_choice.data_type = "Choice"

		for thing in self:
			new_choice.addChild(thing.copy(visited))
		visited.remove(self)
		return new_choice

	def getSizeBytes(self):
		some_choice = self.children[0]
		return some_choice.getSizeBytes()


class Enum(Choice):

	def __init__(self, name, num_choices, choice_type, engine, parent=None):
		Choice.__init__(self, name, num_choices, choice_type, engine, parent)
		self.data_type = "Enum"

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_enum = Enum(self.name, self.num_choices, self.choice_type, self.engine, self.parent)
		new_enum.data_type = "Enum"

		for thing in self:
			new_enum.addChild(thing.copy(visited))
		visited.remove(self)
		return new_enum

	# all enum choices should have default vals
	def generate(self):
		# 5% chance to be something other than one of the predefined vals
		if random.random() < 0.05:
			size = self[0][0].bit_size
			val = random.randint(0, 2**size-1)

		else:
			val = random.choice(self)[0].default_val

		self.value = val
		if val < 1 or val >= 0x7fffffff:
			fmt = "<I"
		else:
			fmt = "<i"

		# pack it for generation
		self.value = val
		val = struct.pack(fmt, val)
		return val, {}, {}, {}



class Union(Choice):

	def __init__(self, name, num_choices, choice_type, engine, parent=None):
		Choice.__init__(self, name, num_choices, choice_type, engine, parent)
		self.data_type = "Union"

	def copy(self, visited=None):
		if visited is None:
			visited = []
		visited.append(self)
		new_union = Union(self.name, self.num_choices, self.choice_type, self.engine, self.parent)
		new_union.data_type = "Union"

		for thing in self:
			new_union.addChild(thing.copy(visited))
		visited.remove(self)
		return new_union

	def generate(self):
		pick = random.choice(self)
		#print "PICKED %s" % pick.name
		stuff = pick.generate()
		self.value = stuff[0]
		return stuff
