import zipfile
import sys
import urllib.request
import os

# massive hack but whatever

if len(sys.argv) != 3:
	print("Usage: {} appveyor_job_id version_number".format(sys.argv[0]))
	sys.exit(1)

zipf = zipfile.ZipFile("asar"+sys.argv[2]+".zip", 'x', compression=zipfile.ZIP_DEFLATED)

appveyor_prefix = "https://ci.appveyor.com/api/buildjobs/{}/artifacts/build/asar/MinSizeRel/".format(sys.argv[1])
with urllib.request.urlopen(appveyor_prefix + "asar-standalone.exe") as resp:
	exe_data = resp.read()
with urllib.request.urlopen(appveyor_prefix + "asar.dll") as resp:
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
		zipf.write(dirpath+"/"+x, dirpath.replace("src/asar-dll-bindings", "dll/bindings")+"/"+x)