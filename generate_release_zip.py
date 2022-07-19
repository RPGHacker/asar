import zipfile
import sys
import urllib.request
import os
import subprocess

if len(sys.argv) != 2:
	print("Usage: {} version_number".format(sys.argv[0]))
	sys.exit(1)

zipf = zipfile.ZipFile("asar"+sys.argv[1]+".zip", 'x', compression=zipfile.ZIP_DEFLATED)

branch_name = subprocess.run("git rev-parse --abbrev-ref HEAD", shell=True, stdout=subprocess.PIPE).stdout.decode().strip()
build_server_prefix = lambda f, n: f"https://random.muncher.se/ftp/asar/windows/"+branch_name+"/win32/build/asar/{f}/MinSizeRel/{n}"
with urllib.request.urlopen(build_server_prefix("bin", "asar.exe")) as resp:
	exe_data = resp.read()
with urllib.request.urlopen(build_server_prefix("lib", "asar.dll")) as resp:
	dll_data = resp.read()

zipf.writestr("asar.exe", exe_data)
zipf.writestr("dll/asar.dll", dll_data)

for (dirpath, dirnames, filenames) in os.walk("docs"):
	for x in filenames:
		zipf.write(dirpath + "/" + x)

for (dirpath, dirnames, filenames) in os.walk("ext"):
	for x in filenames:
		zipf.write(dirpath + "/" + x)

zipf.write("README.txt")
zipf.write("LICENSE")
zipf.write("license-gpl.txt")
zipf.write("license-lgpl.txt")
zipf.write("license-wtfpl.txt")

for (dirpath, dirnames, filenames) in os.walk("src/asar-dll-bindings"):
	for x in filenames:
		if not x.endswith(".in"):
			zipf.write(dirpath+"/"+x, dirpath.replace("src/asar-dll-bindings", "dll/bindings")+"/"+x)
