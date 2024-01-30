using AsarCLR;
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;

class AsarTest
{
	static void check(bool ck, string error)
	{
		if(ck == false) {
			Console.WriteLine("Assertion \"{0}\" failed!", error);
			Environment.Exit(1);
		}
	}

	static int Main(string[] args)
	{
		check(Asar.init() == true, "init()");
		check(Asar.version() >= 20000, "version() correct");
		check(Asar.apiversion() == 400, "apiversion() correct");
		var rom = new byte[0];
		var files = new Dictionary<string, byte[]>() {
			{"/a.asm", Encoding.ASCII.GetBytes(@"
org $8000
db 1,2
org $ffff
db 0
macro x(a)
warn ""hello <a>!""
endmacro
macro y(h)
%x(<h>)
endmacro
%y(a)
%y(b)
") }
		};
		check(Asar.patch("/a.asm", ref rom, memoryFiles: files, fullCallStack: true, generateChecksum: true), "patch()");
		var errs = Asar.geterrors();
		foreach(var x in errs) {
			Console.WriteLine("{0}", x.Fullerrdata);
		}
		check(errs.Length == 0, "no errors");
		errs = Asar.getwarnings();
		check(errs.Length == 2, "2 warnings");
		check(errs[0].Rawerrdata == "warn command: hello a!", "warning 1 contents");
		check(errs[1].Rawerrdata == "warn command: hello b!", "warning 2 contents");
		check(errs[1].ErrorId == "Wwarn_command", "warning name");
		check(errs[1].Block == "warn \"hello b!\"", "warning block");
		foreach(var x in errs) {
			//Console.WriteLine("{0}", x.Fullerrdata);
			check(x.CallStack.Length == 2, "callstack size");
			foreach(var y in x.CallStack) {
				check(y.PrettyPath == "a.asm", "callstack pretty name");
				check(y.FullPath == "/a.asm", "callstack full name");
				check(y.Details.Contains("in macro call"), "callstack details");
			}
		}
		//Console.WriteLine("{0}", rom.Length);
		check(rom.Length == 0x8000, "rom size");
		//Console.WriteLine("{0} {1}", rom[0], rom[1]);
		check(rom[0] == 1 && rom[1] == 2, "rom contents");
		//Console.WriteLine("{0}", BitConverter.ToString(rom.Skip(0x7fdc).Take(4).ToArray()));
		check(rom[0x7fdc] == 0xfe && rom[0x7fdd] == 0xfd, "rom checksum");

		// TODO: mapper type, written block, defines, labels

		Console.WriteLine("All checks passed!");
		return 0;
	}
}
