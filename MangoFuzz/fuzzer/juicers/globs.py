supported_jtypes = {}

def getSupportedJtypes():
	return supported_jtypes.keys()

def get_juicer(jtype):
	try:
		return supported_jtypes[jtype]
	except KeyError:
		return None

def registerJtype(jtype, jclass):
	supported_jtypes[jtype] = jclass
