def normal_peel(fmt, *args):
	"""
		logs info
	:param fmt: format string
	:param args: arguments.
	:return: None
	"""
	print("[*] " + fmt % args)


def thick_peel(fmt, *args):
	"""
		logs debug
	:param fmt: format string
	:param args: arguments.
	:return: None
	"""
	print("[$] " + fmt % args)


def juicy_peel(fmt, *args):
	"""
		logs success
	:param fmt: format string
	:param args: arguments.
	:return: None
	"""
	print("[+] " + fmt % args)


def raw_peel(fmt, *args):
	"""
		logs warning
	:param fmt: format string
	:param args: arguments.
	:return: None
	"""
	print("[!] " + fmt % args)


def rotten_peel(fmt, *args):
	"""
		logs error
	:param fmt: format string
	:param args: arguments.
	:return: None
	"""
	print("[-] " + fmt % args)
	import ipdb;ipdb.set_trace()
	import sys;sys.exit(1)

