"""
	This is the central engine that contains all the global
	data that is needed by various components.
"""
import random
import time
import string
from utils import *
from blenders import BlenderFactory
import juicers
import sys

class Engine(object):
	"""
		The global engine object.
		which will be used by everyone.
	"""

	def __init__(self, initial_seed=None):
		"""
			Create a new engine object.
		:param initial_seed: The initial seed to be used by Engine.
		"""
		if initial_seed is not None:
			self.initial_seed = int(initial_seed)
			normal_peel("Provided initial seed=%d", self.initial_seed)
		else:
			self.initial_seed = int(time.time())
			raw_peel("No initial seed provided, using seed=%d", self.initial_seed)
		random.seed(self.initial_seed)
		self.pits = []
		self.iters = 0
		self.blender_factory = BlenderFactory(self)
		self.juicer = None

	def getPit(self, name):
		for pit in self.pits:
			if pit.name == name:
				return pit
		return None

	def addPit(self, jpit):
		name = jpit.name
		if self.getPit(name) != None:
			rotten_peel("Trying to add repeated pit: %s", name)
		self.pits.append(jpit)

	def run(self, juicer_type, num_tests, pit_name=None, **kwargs):

		if self.juicer is None:
			supported_jtypes = juicers.getSupportedJtypes()
			if juicer_type not in supported_jtypes:
				rotten_peel("Juicer type %s not supported!", juicer_type)

			juicer_ = juicers.get_juicer(juicer_type)
			my_juicer = juicer_(**kwargs)
			self.juicer = my_juicer

		else:
			my_juicer = self.juicer

		if num_tests is None:
			num_tests = sys.maxint

		for x in xrange(num_tests):
			if pit_name == None:
				pit = random.choice(self.pits)
			else:
				pit = self.getPit(pit_name)
				if pit == None:
					rotten_peel("Provided name does not correspond to a valid pit: %s", pit_name)
			blob_arr, map_arr = pit.run()
			my_juicer.send(blob_arr, map_arr, pit.target_struct, pit.devname, int(pit.ioctl_id))
			self.iters += 1

