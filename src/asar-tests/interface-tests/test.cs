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
		var defs = new Dictionary<string, string>() { {"somedefine", "a"} };
		var files = new Dictionary<string, byte[]>() {
			{"/a.asm", Encoding.ASCII.GetBytes(@"
org $8000
db 1,2
org $ffff
db 0
org $018000
lbl:
db 0
lbl2:
macro x(a)
warn ""hello <a>!""
endmacro
macro y(h)
%x(<h>)
endmacro
%y(a)
%y(b)
!x = asdf
!y := !somedefine
") }
		};
		// TODO: should probably exercise more of the parameters here
		check(Asar.patch("/a.asm", ref rom,
					memoryFiles: files, additionalDefines: defs,
					fullCallStack: true, generateChecksum: true),
				"patch() ok");
		var errs = Asar.geterrors();
		foreach(var x in errs) {
			Console.WriteLine(x.Fullerrdata);
		}
		check(errs.Length == 0, "no errors");
		errs = Asar.getwarnings();
		check(errs.Length == 2, "2 warnings");
		check(errs[0].Rawerrdata == "warn command: hello a!", "warning 1 contents");
		check(errs[1].Rawerrdata == "warn command: hello b!", "warning 2 contents");
		check(errs[1].ErrorId == "Wwarn_command", "warning name");
		check(errs[1].Block == "warn \"hello b!\"", "warning block");
		foreach(var x in errs) {
			//Console.WriteLine(x.Fullerrdata);
			check(x.CallStack.Length == 2, "callstack size");
			foreach(var y in x.CallStack) {
				check(y.PrettyPath == "a.asm", "callstack pretty name");
				check(y.FullPath == "/a.asm", "callstack full name");
				check(y.Details.Contains("in macro call"), "callstack details");
			}
		}
		//Console.WriteLine(rom.Length);
		check(rom.Length == 0x8001, "rom size");
		//Console.WriteLine("{0} {1}", rom[0], rom[1]);
		check(rom[0] == 1 && rom[1] == 2, "rom contents");
		//Console.WriteLine(BitConverter.ToString(rom.Skip(0x7fdc).Take(4).ToArray()));
		check(rom[0x7fdc] == 0xfe && rom[0x7fdd] == 0xfd, "rom checksum");

		var blocks = Asar.getwrittenblocks();
		foreach(var x in blocks) {
			//Console.WriteLine("snes={0:x} pc={1:x} len={2:x}", x.Snesoffset, x.Pcoffset, x.Numbytes);
		}
		check(blocks.Length == 4, "written block count");
		check(blocks[0].Snesoffset == 0x808000 && blocks[0].Pcoffset == 0 && blocks[0].Numbytes == 2, "writtenblock 0 ok");
		check(blocks[3].Snesoffset == 0x818000 && blocks[3].Pcoffset == 0x8000 && blocks[3].Numbytes == 1, "writtenblock 3 ok");

		check(Asar.getmapper() == MapperType.LoRom, "mapper type");

		check(Asar.getlabelval("lbl") == 0x018000, "label value");
		var labels = Asar.getlabels();
		check(labels.Length == 2, "getlabels() length");
		check(labels[0].Name == "lbl" && labels[0].Location == 0x018000, "label 0 ok");
		check(labels[1].Name == "lbl2" && labels[1].Location == 0x018001, "label 1 ok");
		check(Asar.getlabelval("hgfsd") == -1, "nonexistent label");

		var found_defs = new bool[] {false, false, false};
		foreach(var x in Asar.getalldefines()) {
			//Console.WriteLine("!{0} = {1}", x.Name, x.Contents);
			if(x.Name == "somedefine") { check(x.Contents == "a", "!somedefine value"); found_defs[0] = true; }
			if(x.Name == "x") { check(x.Contents == "asdf", "!x value"); found_defs[1] = true; }
			if(x.Name == "y") { check(x.Contents == "a", "!y value"); found_defs[2] = true; }
		}
		check(found_defs.SequenceEqual(new bool[] {true, true, true}), "all defines found");

		check(Asar.getdefine("fdafdsa") == "", "nonexistent define");
		check(Asar.getdefine("x") == "asdf", "define !x value");
		check(Asar.resolvedefines("aaaaa !y") == "aaaaa a", "resolvedefines()");

		string matherr;
		check(Asar.math("1+lbl2", out matherr) == 0x018002, "math");
		check(matherr == null, "math error string");
		Asar.math("asdf", out matherr);
		//Console.WriteLine(matherr);
		check(matherr == "Label 'asdf' wasn't found.", "math error");

		check(Asar.getsymbolsfile("wla")[0] == ';', "symbol file format");

		Console.WriteLine("All checks passed!");
		return 0;
	}
}
