import argparse
import sys

def camel_case_to_snake_case(s):
	return "".join(["_" + c.lower() if c.isupper() else c for c in s]).lstrip("_")

def main():
	parser = argparse.ArgumentParser(description="Generate wintunimp.h from a def file")
	parser.add_argument("def_file", help="The def file")
	parser.add_argument("-o", "--output", help="The output file")
	args = parser.parse_args()

	f = sys.stdout
	if args.output:
		f = open(args.output, "w")

	try:
		f.write("#pragma once\n\n")
		f.write("""#include "wintun.h"\n""")
		f.write("#define DllImport __declspec(dllimport)\n\n")

		f.write("#ifdef __cplusplus\n")
		f.write("extern \"C\" {\n")
		f.write("#endif\n\n")

		with open(args.def_file, "r") as def_file:
			for line in def_file:
				line = line.strip()
				if line == "EXPORTS":
					continue
				if line == "":
					continue
				symbol = line
				upper_snake_case = camel_case_to_snake_case(symbol).upper().replace("_U_I_D", "UID")
				f.write(f"DllImport {upper_snake_case}_FUNC {symbol};\n")

		f.write("\n#ifdef __cplusplus\n")
		f.write("}\n")
		f.write("#endif\n\n")
	finally:
		if f != sys.stdout:
			f.close()

if __name__ == "__main__":
	main()
