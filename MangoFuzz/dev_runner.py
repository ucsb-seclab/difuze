#!/usr/bin/python
import random
import argparse
import os
import time
import subprocess

def main():
	parser = argparse.ArgumentParser(description="MangoFuzz options")
	parser.add_argument('-f', type=str, help="Path to the out file", required=True)
	parser.add_argument('-num', type=str, help="Number of tests to run per driver", required=True)
	parser.add_argument('-seed', type=str, help="Choose seed if desired", default=None)
	parser.add_argument('-port', type=str, help="Choose port if desired", default='2022')
	args = parser.parse_args()
	
	out_file = args.f
	num_tests = args.num
	seed = args.seed
	port = args.port

	if seed is not None:
		random.seed(seed)
	else:
		random.seed(time.time)

	devices = os.listdir(out_file)
	iters = 0
	count_dict = {}
	for dev in devices:
		count_dict[dev] = 0

	while (True):
		device = random.choice(devices)
		device_path = out_file + '/' + device
		print '[*] ', device
		print '[#] iters: %d' % iters
		print '[#] dev iters: %d' % count_dict[device]
		runner_cmd = './runner.py -f ' + device_path + ' -num ' + num_tests + ' -port ' + port
		print runner_cmd
		subprocess.call(['./runner.py', '-f', device_path, '-num', num_tests, '-port', port])
		count_dict[device] += int(num_tests)
		iters += int(num_tests)


if __name__ == '__main__':
	main()
