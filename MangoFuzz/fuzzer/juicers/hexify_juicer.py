from juicer import Juicer
from . import registerJtype


class HexifyJuicer(Juicer):
	"""
		This juicer converts the provided juice into hex string.
	"""
	def __init__(self, console_print=True):
		self.console_print = console_print
		self.jtype = 'hex'

	def juice(self, target_juice):
		# TODO:
		to_ret = None
		if self.console_print:
			# TODO: print the output in hex on console
			pass
		# TODO: convert into hex string
		return to_ret

	def getName(self):
		return "hexify"


