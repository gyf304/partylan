import argparse
import sys
import subprocess

def main():
	parser = argparse.ArgumentParser(description="Generate a .def file from a .dll file")
	parser.add_argument("dll", help="The .dll file to generate a .def file from")
	parser.add_argument("-o", "--output", help="The .def file to output to")
	args = parser.parse_args()

	f = sys.stdout
	if args.output:
		f = open(args.output, "w")

	try:
		dumpbin = subprocess.Popen(["dumpbin", "/exports", args.dll], stdout=subprocess.PIPE)
		out, err = dumpbin.communicate()
		if dumpbin.wait() != 0:
			raise Exception("dumpbin failed")
		
		f.write("EXPORTS\n")

		out = out.decode("utf-8")
		started = False
		ended = False
		for line in out.splitlines():
			line = line.strip()
			if not started and line.startswith("ordinal"):
				started = True
				continue
			if started and line.startswith("Summary"):
				ended = True
				break
			if started and not ended:
				if line != "":
					symbol = line.split()[-1]
					f.write("\t" + symbol + "\n")

	finally:
		if f != sys.stdout:
			f.close()

if __name__ == "__main__":
	main()
