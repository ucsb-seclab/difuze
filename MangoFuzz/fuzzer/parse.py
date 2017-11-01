import xml.etree.ElementTree as ET
#from my_types import *
from mango_types import *
import juicers
from utils import *

class Parser():
	def __init__(self, engine):
		self.engine = engine
		self.pit_file = None
		self.main_struct = None
		self.last_parsed = None
		self.cur_parsing = None
		self.parsed_dms = []
		# will need to call copy() on these
		# once the parsing of the DM is done
		self.recursive_resolves = []
		self.jspec = None
		self.jspec_tree = None

	def lookupRef(self, ref_to):
		resolved_ref = None
		for dm in self.parsed_dms:
			if dm.name == ref_to:
				resolved_ref = dm

		if resolved_ref is None:
			# recursive reference
			if ref_to == self.cur_parsing.name:
				return self.cur_parsing
			# Couldn't find it
			else:
				rotten_peel("Couldn't find reference to: %s", ref_to)
		else:
			return resolved_ref


	def HandleNumber(self, num_node, parent):
		name = num_node.get('name')
		size = int(num_node.get('size'))
		value = num_node.get('value')
		
		number = Number(name, size, self.engine)
		if value is not None:
			value = int(value)
			number.setDefaultValue(value)
			if value < 0:
				number.signed = True
		
		return number

	def HandleString(self, num_node, parent):
		name = num_node.get('name')
		length = int(num_node.get('length'))

		my_string = String(name, length, self.engine)
		return my_string


	def HandleBlock(self, block_node, parent):
		name = block_node.get('name')
		ref = block_node.get('ref')

		if ref is not None:
			resolved_ref = self.lookupRef(ref).copy()
		else:
			resolved_ref = None

		# array
		minOccurs = block_node.get('minOccurs')
		maxOccurs = block_node.get('maxOccurs')
		
		occurs = 1
		if minOccurs is not None:
			occurs = minOccurs

		block = Block(name, self.engine, parent, resolved_ref, occurs)

		# if this block contains children
		children = block_node.getchildren()
		# if we're an array
		if occurs > 1:
			for x in range(int(occurs)):
				for child in children:
					child_copy = child.copy()
					child_copy.set('name', child.get('name')+'_'+str(x))
					self.ParseElement(child_copy, block)

		else:
			for child in children:
				self.ParseElement(child, block)

		return block

	def HandleChoice(self, choice_node, parent):
		name = choice_node.get('name')
		choice_type = choice_node.get('choice_type')
		num_choices = len(choice_node)

		if choice_type == 'union':
			choice = Union(name, num_choices, choice_type, self.engine)
		elif choice_type == 'enum':
			choice = Enum(name, num_choices, choice_type, self.engine)
		else:
			rotten_peel("Unknown choice type: %s", choice_type)

		# handle children
		children = choice_node.getchildren()
		for child in children:
			self.ParseElement(child, choice)

		return choice
		
	def HandlePointer(self, pointer_node, parent):
		name = pointer_node.get('name')
		ptr_to = pointer_node.get('ptr_to')
		ptr_depth = pointer_node.get('ptr_depth')
		length = int(pointer_node.get('length'))
		
		pointer = Pointer(name, ptr_to, ptr_depth, length, self.engine, parent)

		# TODO: Keep this consistent with Block refs (ref/resolved parent setting)

		# generic pointer. Nothing to generate
		if pointer.is_generic_ptr:
			pointer.ptr_type = pointer_node.get('base')
			pointer.elem_size = pointer_node.get('elem_size')

		# complex pointer (should point to a dm)
		# Fuuuuuck. Pointers to unions. I don't handle this in generation.
		else:
			# Check if recursive
			if self.cur_parsing.name == ptr_to:
				# don't call copy(). This will be done for us after parsing the DM
				# we wait to do this because the DM is still being parsed
				resolved_dm = self.lookupRef(ptr_to)
				pointer.resolved = resolved_dm
				self.recursive_resolves.append(pointer)
			else:
				# make a copy of the resolved DM so we have a separate obj
				resolved_dm_instance = self.lookupRef(ptr_to).copy()
				resolved_dm_instance.parent = pointer
				pointer.resolved = resolved_dm_instance

		return pointer

	def ParseElement(self, node_to_parse, parent):
		name = node_to_parse.get('name')
		tag = node_to_parse.tag
		new_elem = None

		if tag == 'Number':
			new_elem = self.HandleNumber(node_to_parse, parent)
		elif tag == 'String':
			new_elem = self.HandleString(node_to_parse, parent)
		elif tag == 'Block':
			new_elem = self.HandleBlock(node_to_parse, parent)
		elif tag == 'Choice':
			new_elem = self.HandleChoice(node_to_parse, parent)
		elif tag == 'Pointer':
			new_elem = self.HandlePointer(node_to_parse, parent)
		else:
			rotten_peel("Unknown tag while parsing: %s", tag)
		
		# adds child and sets parent
		parent.addChild(new_elem)
		return new_elem

	# parses an xml dm and instantiates a DataModel dom object
	def ParseDataModel(self, dm, tree):
		name = dm.get('name')
		size = int(dm.get('byte_size'))
		# Our DataModel
		our_data_model = DataModel(name, size, tree, dm, self.engine)
		self.cur_parsing = our_data_model
		
		# go through all the contained elementes and instantiate them
		for child in dm:
			# will add the children to our_data_model
			our_data_element = self.ParseElement(child, our_data_model)

		self.parsed_dms.append(our_data_model)
		return our_data_model

	def parse_config(self, root, jpit):
		config_elem = root.find('Config')
		if config_elem is None:
			rotten_peel("Config element not found in pit!")

		# devname
		devname_ele = config_elem.find('devname')
		if devname_ele is None:
			rotten_peel("devname element not found in config!")
		devname = devname_ele.get('value')
		jpit.devname = devname

		# ioctl_id
		ioctl_id_ele = config_elem.find('ioctl_id')
		if ioctl_id_ele is None:
			rotten_peel("ioctl_id element not found in config!")
		ioctl_id = ioctl_id_ele.get('value')
		jpit.ioctl_id = ioctl_id

		# target struct
		target_struct_ele = config_elem.find('target_struct')
		if target_struct_ele is None:
			rotten_peel("target_struct element not found in config!")
		target_struct = target_struct_ele.get('value')
		jpit.target_struct = target_struct


	# parses a jpit and returns
	def Parse(self, fname):
		try:
			tree = ET.parse(fname)
		except ET.ParseError:
			rotten_peel("Failed to parse file: %s", fname)

		self.last_parsed = tree
		# reset the parsed_dms
		self.parsed_dms = []
		root = tree.getroot()

		# create our jpit
		jpit = JPit(fname)
		
		# parse the config info (devname, ioctl_id, target_struct)
		self.parse_config(root, jpit)

		data_models = []
		for child in root:
			if child.tag == 'DataModel':
				data_models.append(child)

		my_dms = []
		for dm in data_models:
			parsed_dm = self.ParseDataModel(dm, tree)
			my_dms.append(parsed_dm)

			# fixup recursive resolves now that the DM is complete
			for ptr_ele in self.recursive_resolves:
				ptr_ele.resolved = ptr_ele.resolved.copy()
				ptr_ele.resolved.parent = ptr_ele

			self.recursive_resolves = []
			self.cur_parsing = None

		for dm in my_dms:
			jpit.addChild(dm)

		# append the jpit to our engine
		self.engine.addPit(jpit)
		return jpit

	def ParseJspec(self, fname):
		supported_jtypes = juicers.getSupportedJtypes()

		self.jspec = fname
		tree = ET.parse(fname)
		self.jspec_tree = tree
		root = tree.getroot()
		juicer_elem = root.find('juicer')
		if juicer_elem is None:
			rotten_peel("Invalid jspec supplied. Couldn't find juicer element!")
		
		jtype = juicer_elem.get('type')
		if jtype not in supported_jtypes:
			rotten_peel("juicer type %s is not supported!", jtype)

		parser = juicer.getParser(jtype)
		juicer = parser(tree)

