import xml.etree.ElementTree as ET
from xml.dom import minidom
import sys
import re
import copy

# TODO: uint8_t's are being interpreted as strings atm
# TODO: unions

class Element:
	def __init__(self, builtin, ele_type, signed):
		self.builtin = builtin

lookup = {
		"char":"String",
		"signed char":"String",
		"unsigned char":"Number",
		"short":"Number",
		"signed short":"Number",
		"unsigned short":"Number",
		"int": "Number",
		"signed int":"Number",
		"unsigned int":"Number",
		"signed long":"Number",
		"long":"Number",
		"unsigned long":"Number",
		"long long":"Number",
		"signed long long":"Number",
		"unsigned long long":"Number",
		"long long long":"Number",
		"signed long long long":"Number",
		"unsigned long long long":"Number",
		"void":"Number",
		"bool":"Number",
		"string":"String",
		"float":"Number",
		"double":"Number",
		"long double":"Number",
		"union":"Choice",
		# non builtins
		"pointer":"Block",
		"array":"Block",
		"struct":"Block",
		"enum":"Choice",
		"padding":"String"
		}
# TODO. 32bit vs 64bit systems
size_lookup = {
		"char":1,
		"signed char":1,
		"unsigned char":1,
		"short":2,
		"signed short":2,
		"unsigned short":2,
		"int": 4,
		"signed int":4,
		"unsigned int":4,
		"signed long":8,
		"long":8,
		"unsigned long":8,
		"long long":8,
		"signed long long":8,
		"unsigned long long":8,
		"long long long":8,
		"signed long long long":8,
		"unsigned long long long":8,
		"void":1,
		"bool":1,
		"string":1,
		"float":99,
		"double":99,
		"long double":16,
		"union":1
		}

#if "fixup" in sys.argv[1]:
#	 f = open(sys.argv[1].replace("_fixup", "").replace('xml', 'h'),'r')
#else:
#	 f = open(sys.argv[1].replace('xml', 'h'),'r')
#data = f.read()
#f.close()
tree = ET.parse(sys.argv[1])
root = tree.getroot()
skip_len = len('<?xml version="1.0" ?>')
padno = 0
padno_pre = 0
anon_union = 0
anon_struct = 0
noname = 0
# TODO: Make this more sequential

def prettify(elem):
	#Return a pretty-printed XML string for the Element.
	rough_string = ET.tostring(elem, 'utf-8')
	reparsed = minidom.parseString(rough_string)
	return reparsed.toprettyxml(indent="	")[skip_len+1:-1:]

def get_val_from_line(line):
	regex = "-?0[xX][0-9a-fA-F]{1,16}|-?\d+"
	#num = re.findall("[x\-\d]+", line)
	num = re.findall(regex, line)
	if len(num) == 0:
		return None
	num = num[0]

	try:
		val = int(num)
	except ValueError:
		#print "[*] Trying to convert: ", num
		val = int(num, 16)
		#print 'converted to ', val

	return val

def find_enums(elem):
	enums = []
	look_for = elem.get('id')
	for child in root:
		if child.get('base-type') == look_for and child.get('toplevel') != None:
			enums.append(child)
	return enums

def find_num(name):
	lines = data.splitlines()
	idx = 0
	for line in lines:
		if name in line:
			break
		idx += 1

	#print idx
	#print lines[idx]
	#print line
	
	holder = idx
	while(idx):
		idx -= 1
		line = lines[idx]
		start = line.find('=')
		end = line.find('\n')
		line = line[start:end]
		num = get_val_from_line(line)
		if num is not None:
			#print "PREV NUM: ", num
			our_num = holder - idx + num
			#print "FOUND THE NUM: ", our_num
			return our_num

	print "COULDN'T FIND THE ENUM VAL"
	sys.exit(1)

# TODO: THis only works for fuggin numbers
def val_helper(name):
	#print name
	start = data.find(name) + len(name)
	end = data[start::].find('\n')+start
	line = data[start:end].strip().replace(',','')
	num = get_val_from_line(line)
	if num is None:
		#print 'val_helper: got None'
		num = find_num(name)

	#print '[+] returning num: ', num
	return num

def get_structs_and_unions(root):
	struct_nodes = []
	for child in root:
		#if child.get('type') == 'struct':
		if child.get('type') in ['struct', 'union']:
			struct_nodes.append(child)

	#print len(struct_nodes)
	return struct_nodes


def find_type(node, base_type):
	if node.get('id') == base_type:
		return node

	for child in node.getchildren():
		cool = find_type(child, base_type)
		if cool is not None:
			return cool


# get to the starting point for our element
def peel_elem(elem, name, offset):
	'''
	 there are 2 cases where we don't need to peel anymore
		(a) we have a type field and it's in our lookup table
		(b) we have a base-type-builtin (unfortunately these are type node)
	'''

	elem_base = elem.get('base-type-builtin')
	elem_type = elem.get('type')
	out_elem = ''
	
	try:
		lookup_type = lookup[elem_type]

	except KeyError:
		lookup_type = None

	# complex case
	if lookup_type is not None:
		# elem_base may be None or may be something meaningful as seen here:
		# <symbol type="pointer" id="_19" bit-size="64" alignment="8" offset="0" base-type-builtin="void"/>
		#print "Complex"
		#print elem.attrib
		#ipdb.set_trace()
		out_elem = build_ele(elem, elem_type, elem_base, False, name, offset)

	# simple case
	# We should only hit this for simple cases such as numbers and chars
	elif elem_base is not None:
		#print "Simple"
		#print elem.attrib
		#ipdb.set_trace()
		out_elem = build_ele(elem, elem_type, elem_base, True, name, offset)
	
	# Should this be a while loop? I don't think so
	else:
		#print '[+] Searching...'
		btype = elem.get('base-type')
		if btype == None:
			print 'Unknown base-type'
			sys.exit(1)
		peeled_elem = find_type(root, elem.get('base-type'))
		if peeled_elem == None or peeled_elem.attrib == {}:
			print "Couldn't find the guy"
			sys.exit(1)
		return peel_elem(peeled_elem, name, offset)

	return out_elem

def parse_node(struct):
	#print struct.get('ident')
	global anon_struct
	global noname
	struct_name = struct.get('ident')
	if struct_name is None:
		struct_name = 'anon_struct' + struct.get('id')
	struct_byte_size = int(struct.get('bit-size'))/8
	struct_node = ET.Element("DataModel", name=struct_name, byte_size=str(struct_byte_size), type=struct.get('type'))
	sz = int(struct.get('bit-size'))
	for elem in struct:
		name = elem.get('ident')
		offset = elem.get('offset')
		if name is None:
			name = 'noname' + str(noname)
			noname += 1
		node = peel_elem(elem, name, offset)
		if node is not None:
			struct_node.append(node)
	return struct_node

def build_number(elem, name, val=None):
	bit_size = elem.get('bit-size')
	if val is None:
		out_elem = ET.Element('Number', name=name, size=bit_size)
	else:
		out_elem = ET.Element('Number', name=name, size=bit_size, value=val)

	return out_elem

def build_string(elem, name):
	bit_size = elem.get('bit-size')
	byte_size = str(int(bit_size)/8)
	out_elem = ET.Element('String', name=name, length=byte_size)
	#print name
	if name is None:
		import ipdb;ipdb.set_trace()
	return out_elem

# TODO: union ptrs, func ptrs
# TODO: Handle pointer array offsets
def build_pointer(elem, name, offset):
	'''
	pointers are just string elements..
	we name them specially so that our post_processor can create 
	a mapping later.
	'''

	# okay, there are a couple things to do here:
	# (1) If this is a simple ptr (char, void, int, etc). The depth is 1 and we already know the type
	# (2) get to the bottom of the pointer to assess pointer depth
	# (3) once there. Check if it's a ptr to a builtin, or a struct

	pointer_depth = 1
	size = elem.get('bit-size')
	length = str(int(size)/8)
	elem_base = elem.get('base-type-builtin')
	elem_type = elem.get('type')

	# (1) we have a simple case here where it's a pointer to something like an int or char..
	if elem_base is not None:
		ptr_to = lookup[elem_base]
		elem_size = size_lookup[elem_base]
		ptr_elem = ET.Element('Pointer', name=name, length=length, ptr_to=ptr_to, ptr_depth=str(pointer_depth), base=elem_base, offset=offset, elem_size=str(elem_size))
		return ptr_elem

	# (2) okay, ptr to something else. Let's get to the bottom here.
	resolved_ele = find_type(root, elem.get('base-type'))
	resolved_base = resolved_ele.get('base-type-builtin')
	resolved_type = resolved_ele.get('type')

	# (2) cont. This ptr is more than 1 level deep. so keep going till we hit the bottom
	while (resolved_base is None and resolved_type == 'pointer'):
		pointer_depth += 1
		resolved_ele = find_type(root, resolved_ele.get('base-type'))
		resolved_base = resolved_ele.get('base-type-builtin')
		resolved_type = resolved_ele.get('type')

	# (3) we've hit the bottom, now we either have a builtin, a struct, or some fucked up stuff that we're gonna ignore for now
	# (3a) it's a builtin, nice
	if resolved_base is not None:
		# this is to account for the fact that nonbuiltin ptrs will always have 1 extra depth just to get the the type
		pointer_depth += 1
		resolved_size = size_lookup[resolved_base]
		ptr_to = lookup[resolved_base]
		ptr_elem = ET.Element('Pointer', name=name, length=length, ptr_to=ptr_to, ptr_depth=str(pointer_depth), offset=offset, base=resolved_base, elem_size=str(resolved_size))

	else:
		# we're only handle structs right now so check if it's not a struct pointer.
		if resolved_type == 'struct':
			ptr_to = resolved_ele.get('ident')
			ptr_elem = ET.Element('Pointer', name=name, length=length, ptr_to=ptr_to, ptr_depth=str(pointer_depth), offset=offset)

		elif resolved_type == 'function':
			ptr_to = 'function'
			ptr_elem = ET.Element('Pointer', name=name, length=length, ptr_to=ptr_to, ptr_depth=str(pointer_depth), offset=offset)

		elif resolved_type == 'union':
			ptr_to = resolved_ele.get('ident')
			ptr_elem = ET.Element('Pointer', name=name, length=length, ptr_to=ptr_to, ptr_depth=str(pointer_depth), offset=offset)

	return ptr_elem


def build_array(elem, name, offset):
	'''
	an array element is just a block element with some enclosed elements
	One thing to keep in mind here is the possibility of a 0 element array.
	Also watch for multi-dimensional arrays.
	If we hit one of these, we'll need to backpedal a bit and remove the parent element
	'''
	arr_size = elem.get('array-size')
	# TODO: CHECKME. Seems sometimes 0 size arrays have no array-size element:
	# 19532   struct kmem_cache *memcg_caches[0];
	#  8513   <symbol alignment="8" base-type="_7240" bit-size="-1" end-col="33" end-line="9567" file="/tmp/cool.h" id="_7255" offset="0" start-col="31" start-line="9567" type="array" />
	if arr_size == '0' or arr_size == None:
		#print "array size of 0"
		return None
	try:
		elem_size = (int(elem.get('bit-size'))/int(arr_size))/8
	except TypeError:
		import ipdb;ipdb.set_trace()

	# we'll do a prelim check to make sure we have an array size > 0.

	# if we have a postiive array size, it should be a given that we'll have a Block element
	block_element = ET.Element('Block', name=name, maxOccurs=arr_size, minOccurs=arr_size, offset=offset, elem_size=str(elem_size))

	# let's first deal with the simple case where we have an array of builtin type
	array_type = elem.get('base-type-builtin')
	if array_type is not None:
		array_peach_type = lookup[array_type]
		# we'll want to modify the element's size so our Number and String builder don't mess up
		single_elem_bitsize = int(elem.get('bit-size'))/int(arr_size)
		# CHECK: is it fine to just use the original element?
		elem_dup = copy.copy(elem)
		elem_dup.set('bit-size', str(single_elem_bitsize))

		if array_peach_type == 'Number':
			arr_element = build_number(elem_dup, name+'_ele')

		elif array_peach_type == 'String':
			arr_element = build_string(elem_dup, name+'_ele')

		else:
			print 'unaccounted for array type'
			import ipd;ipdb.set_trace()

		if arr_element is None:
			import ipdb;ipdb.set_trace()
		block_element.append(arr_element)


	# okay, so if we're here then we have a more complex array type
	else:
		arr_elem_btype = elem.get('base-type')
		if arr_elem_btype is None:
			print 'array element has no base-type and is not simple'
			import ipdb;ipdb.set_trace()

		resolved_arr_elem = find_type(root, arr_elem_btype)
		resolved_arr_elem_type = resolved_arr_elem.get('type')

		arr_element = peel_elem(resolved_arr_elem, name+'_ele', offset)
		if arr_element is None:
			import ipdb;ipdb.set_trace()
		block_element.append(arr_element)

	# this should be catching arrays who have zero size somewhere
	if arr_element is None:
		if resolved_arr_elem_type != 'array':
			print 'somethine weird is going on in array building'
			import ipdb;ipdb.set_trace()
		return None

	return block_element


def build_struct(elem, name, offset):
	'''
	a struct element is just a block elem with a ref to the struct type
	pretty easy :)
	'''
	
	struct_type = elem.get('ident')
	if struct_type is None:
		struct_type = "anon_struct" + elem.get('id')
	block_element = ET.Element('Block', name=name, ref=struct_type, offset=offset)
	return block_element

def build_union(elem, name, offset):
	'''
	This will be a Choice block with a set of block elements inside
	each block element is a valid choice of the union.
	This gets a tad tricky because despite the choice we pick, it has to be the same size
	so depending on the choice we'll add padding elements
	'''
	global padno_pre
	global noname
	# I think I can just iterate through the choices and call peel_elem
	# and then fixup the size with padding
	choice_no = 0
	choice_block_element = ET.Element('Choice', name=name, offset=offset, choice_type='union')
	bit_size = elem.get('bit-size')
	byte_size = str(int(bit_size)/8)

	for choice in elem:
		if name is None:
			import ipdb;ipdb.set_trace()
		block_element = ET.Element('Block', name=name+'_choice'+str(choice_no))
		choice_no += 1
		choice_size_bits = choice.get('bit-size')
		choice_name = choice.get('ident')
		if choice_name is None:
			choice_name = 'anon' + str(noname)
			noname += 1
		choice_element = peel_elem(choice, choice_name, offset)
		if choice_element is None:
			#print 'weird choice while building union'
			#import ipdb;ipdb.set_trace()
			# TODO: Checkme. This can occur when a choice is basically 0 size..
			continue
		block_element.append(choice_element)
		# add padding if needed
		if int(bit_size) > int(choice_size_bits):
			bit_dif = int(bit_size) - int(choice_size_bits)
			pad_bytes = bit_dif/8
			#print pad_bytes
			pad_ele = ET.Element('String', name=str(padno_pre)+'_'+'padding', length=str(pad_bytes))
			padno_pre += 1
			block_element.append(pad_ele)

		choice_block_element.append(block_element)

	return choice_block_element

def build_enum(elem, name, offset):
	'''
	this is pretty similar to union. We'll have a choice block with
	inner blocks.
	'''
	# so the main task here is to go retrieve the
	# actual enum values because c2xml does not include them
	# this means well need to parse the values from the actual file
	choice_no = 0
	bit_size = elem.get('bit-size')
	fname = elem.get('file')
	start_line = int(elem.get('start-line'))
	end_line = int(elem.get('end-line'))
	f = open(fname, 'r')
	data = f.read().splitlines()[start_line:end_line-1]
	f.close()
	fine_grained = []
	choice_names = []
	regex = '[=, ,\,]'
	# NOTE: this is hacky as fuck
	# let's try to weed the comments out (if any) and pinpoint the values
	for line in data:
		line = line.strip()
		if line[0:2] == "//":
			continue
		if line[0:2] == "/*":
			continue
		if ',' in line or line == data[-1].strip():
			if "//" in line:
				line = line[:line.find('//')]
			if "/*" in line:
				line = line[:line.find('/*')]
			pinpoint = line[line.find('=')::]
			fine_grained.append(pinpoint)
			line = line.strip()
			found = re.search(regex, line)
			if found is None:
				if line == data[-1].strip():
					choice_name = line
				else:
					print "Something wrong with your enum regex"
					print line
					ipdb.set_trace()
			else:
				choice_name = line[:found.start()]

			choice_names.append(choice_name)

	last_val = -1
	last_idx = 0
	val_list = []
	# okay now we HOPEFULLY have either the values or empty strings
	# let's use this to get the vals
	for thing in fine_grained:
		val = get_val_from_line(thing)
		if val is None:
			val = last_val + 1
		
		last_val = val
		last_idx = fine_grained.index(thing)
		val_list.append(val)

	assert len(val_list) == len(choice_names)
	goods = zip(val_list, choice_names)
	choice_block = ET.Element('Choice', name=name, offset=offset, choice_type='enum')

	# now generate choice blocks for all these values
	for pair in goods:
		block_element = ET.Element('Block', name=name+'_choice'+str(choice_no))
		choice_no += 1
		num_element = ET.Element('Number', size=bit_size, name=pair[1], value=str(pair[0]))
		block_element.append(num_element)
		choice_block.append(block_element)

	return choice_block

def build_ele(elem, elem_type, elem_base, simple, name, offset):

	# if it's simple, just build the number/string :)
	if simple:
		out_type = lookup[elem_base]
		out_elem = ''

		if out_type == 'Number':
			out_elem = build_number(elem, name)

		elif out_type == 'String':
			out_elem = build_string(elem, name)

		if out_elem == '':
			ipdb.set_trace()
		return out_elem
		
	# handles: array, pointer, struct, union, enum, ...
	else:
		out_type = lookup[elem_type]
		out_elem = ''
		#print '-'*20
		#print elem_type
		#print '-'*20
		#print

		if elem_type == 'pointer':
			out_elem = build_pointer(elem, name, offset)

		elif elem_type == 'array':
			out_elem = build_array(elem, name, offset)

		elif elem_type == 'struct':
			out_elem = build_struct(elem, name, offset)

		elif elem_type == 'union':
			out_elem = build_union(elem, name, offset)

		elif elem_type == 'enum':
			out_elem = build_enum(elem, name, offset)
		
		if out_elem == '':
			ipdb.set_trace()
		return out_elem


nodes_of_interest = get_structs_and_unions(root)
outer_node = ET.Element("jay_partial_pit", name=sys.argv[1])
for node in nodes_of_interest:
	parsed_node = parse_node(node)
	try:
		prettify(parsed_node)
		outer_node.append(parsed_node)
	except TypeError:
		pass
		#import ipdb;ipdb.set_trace()
	

print prettify(outer_node)

