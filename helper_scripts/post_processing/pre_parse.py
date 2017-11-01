import xml.etree.ElementTree as ET
from lxml import objectify
from xml.dom import minidom
import sys
import re

# Take h files and create xml files
# fixup the XML such that it accounts for padding
# fixup the XML such that enums and vals are included from other headers
debug=0

tree = ET.parse(sys.argv[1])
root = tree.getroot()
# have to make each name unique
i=0
skip_len = len('<?xml version="1.0" ?>')
def prettify(elem):
    #Return a pretty-printed XML string for the Element.
    rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="    ")[skip_len+1:-1:]

def get_index(findme, elem_list):
    for index, elem in enumerate(elem_list):
        if (findme == elem):
            return index

    return -1

def get_structs(root):
    struct_nodes = []
    for child in root:
        if child.get('type') == 'struct':
            struct_nodes.append(child)

    if debug:
        print len(struct_nodes)
    return struct_nodes


def insert_padding(struct, elem, pad_size, member_offset):
    global i
    kwargs = {'ident':"padding" + str(i), 'length':str(pad_size), 'base-type-builtin':'padding', 'bit-size':str(pad_size*8)}
    pad_node = ET.Element("jay_symbol" ,**kwargs)
    i+=1
    pad_node.tail = "\n    "
    struct.insert(member_offset, pad_node)


def fixup_struct(struct):
    if debug:
        print struct.get('ident')
        print "+"*20
    old_elem_end = 0
    cur_idx = 0
    total = len(struct)
    tot_size = struct.get('bit-size')
    tot_bytes = int(tot_size)/8
    cur_size = 0
    while (cur_idx < total):
        elem = struct[cur_idx]
        pad_size = 0
        offset = int(elem.get('offset'))
        if debug:
            print "old_elem_end:", old_elem_end
            print "offset:", offset
        if ( old_elem_end != offset ):
            pad_size = offset-old_elem_end
            if debug:
                print "pad with %d bytes" % pad_size
            insert_padding(struct, elem, pad_size, cur_idx)
            cur_idx += 1
            total += 1

        bit_size = int(elem.get('bit-size'))
        old_elem_end = ( (bit_size/8) + offset)
        cur_idx += 1
        if debug:
            print "=="

    global i
    if old_elem_end < tot_bytes:
        end_pad_amount = tot_bytes - old_elem_end
        kwargs = {'ident':"padding" + str(i), 'length':str(end_pad_amount), 'base-type-builtin':'padding', 'bit-size':str(end_pad_amount*8)}
        pad_node = ET.Element("jay_symbol" ,**kwargs)
        i+=1
        elem.tail += '  '
        pad_node.tail = "\n  "
        #import ipdb;ipdb.set_trace()
        struct.append(pad_node)
    return struct

struct_nodes = get_structs(root)
for struct in struct_nodes:
    if debug:
        print '-'*20
    fixed = fixup_struct(struct)
    idx = get_index(struct, root)
    root.remove(struct)
    root.insert(idx, fixed)
    if debug:
        print '-'*20

fname = sys.argv[1]
tree.write(fname[0:-4] + "_fixup" + ".xml")
