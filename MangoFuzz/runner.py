#!/usr/bin/python
import argparse
import os
from fuzzer.utils import *
import time
from fuzzer.engine import Engine
from fuzzer.parse import Parser


def main():
	parser = argparse.ArgumentParser(description="MangoFuzz options")
	parser.add_argument('-f', type=str, help="Filename of the jpit, or driver directory containing jpits", required=True)
	parser.add_argument('-j', type=str, help="Juicer type. Default is TCP", default='tcp')
	parser.add_argument('-seed', type=int, help="Seed. Default will be time", default=None)
	parser.add_argument('-num', type=int, help="Number of tests to run (if limited). Default is to simply keep running.", default=None)
	parser.add_argument('-a', type=str, help="Address to send the data to. Default is localhost", default='localhost')
	parser.add_argument('-port', type=str, help="Port to send the data to. Default is 2022", default='2022')

	args = parser.parse_args()
	
	pits = args.f
	jtype = args.j
	num_tests = args.num
	addr = args.a
	port = args.port
	seed = args.seed
	
	mango = Engine(seed)
	parser = Parser(mango)

	if os.path.isdir(pits):
		pit_list = []
		files = os.listdir(pits)

		for f in files:
			if list(f).count('_') >= 2 and ".swp" not in f:
				pit_list.append(pits+'/'+f)

		for pit in pit_list:
			parser.Parse(os.path.abspath(pit))

	else:
		parser.Parse(pits)

	mango.run(jtype, num_tests, pit_name=None, name='dsa', address=addr, port=port)

	

if __name__ == '__main__':
	main()
