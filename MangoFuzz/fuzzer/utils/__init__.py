"""
Utils guy
"""
from peelers import *
import binascii


def int2bytes(i):
	"""

	:param i:
	:return:
	"""
	hex_string = '%x' % i
	n = len(hex_string)
	return binascii.unhexlify(hex_string.zfill(n + (n & 1)))
