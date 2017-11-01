class Juicer(object):
	"""
		Main class for all juicers.
	"""
	def __init__(self, name):
		self.name = name

	def juice(self, target_juice):
		"""
			This is the main function that processes the provided
			target object.
		:param target_juice: The target object which needs to be processed by the juicer.
		:return: Depends on the specific juicers.
		"""
		raise NotImplementedError("Function not implemented.")

	def getName(self):
		"""
			This function returns the name of this juicer.
		:return: String representing name of this juicer.
		"""
		raise NotImplementedError("Function not implemented.")
