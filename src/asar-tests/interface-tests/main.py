import os
import subprocess
import sys
import unittest
import tempfile

sys.path += [os.path.join(os.path.dirname(__file__), "../../asar-dll-bindings/python")]
import asar

asar_dll_path = None
if "ASARDLL" in os.environ:
    asar_dll_path = os.environ["ASARDLL"]
asar_exe_path = None
if "ASAREXE" in os.environ:
    asar_exe_path = os.environ["ASAREXE"]

tests_dir = os.path.dirname(os.path.abspath(__file__))

@unittest.skipIf(asar_dll_path is None, "ASARDLL environment variable not set")
class TestAsarDLL(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        asar.init(asar_dll_path)

    @classmethod
    def tearDownClass(cls):
        asar.close()

    def assertOk(self, out, expected_out):
        self.assertEqual(asar.geterrors(), [])
        self.assertTrue(out[0])
        self.assertEqual(out[1], expected_out)
        self.assertEqual(asar.getwarnings(), [])

    def patchSingle(self, patch, **kwargs):
        return asar.patch("/main", b'', memory_files={"/main": patch}, **kwargs)

    def testMemoryFiles(self):
        out = asar.patch("/memf", b'', memory_files={
            "/memf": b"org $8000 : db $42"
            })
        self.assertOk(out, b'\x42')

        out = asar.patch("/memf", b'', memory_files={
            "/memf": b"incsrc mem2",
            "/mem2": b"org $8000 : db $43"
            })
        self.assertOk(out, b'\x43')

    def testRealFiles(self):
        out = asar.patch(tests_dir + "/memory.asm", b'', memory_files={
                tests_dir + "/memory.asm": b"incsrc sample1.asm"
            })
        self.assertOk(out, b'\x11')


    def testStdIncludes(self):
        # std_include_file cannot be a memory file for some reason?
        out = asar.patch(
            "/main", b'',
            std_include_file=tests_dir+"/stdincludes.txt",
            memory_files={
                "/main": b"org $8000 : incsrc stdi1.asm\n\
                        incsrc stdi2.asm : incsrc stdi3.asm",
                "/stdinc/stdi1.asm": b"db 1",
                "/stdinc2/stdi2.asm": b"db 2",
                tests_dir + "/stdinc3/stdi3.asm": b"db 3",
            })
        self.assertOk(out, b'\1\2\3')

    def testStdDefines(self):
        out = asar.patch("/main", b'',
                std_define_file=tests_dir+"/stddefines.txt",
                memory_files={
                    "/main": b"org $8000 : db \"!def1!def2!def3!def4\"",
                })
        self.assertOk(out, b'a bcd ')
        self.assertEqual(asar.getalldefines()["def1"], "a")

    def testDefaultDefines(self):
        out = asar.patch("/main", b'',
            memory_files={"/main": b"""
                    print "!assembler"
                    print dec(!assembler_ver)," or ",hex(!assembler_ver)
                    """})
        self.assertOk(out, b'')
        prints = asar.getprints()
        ver = asar.version()
        self.assertEqual(prints, ["asar", f"{ver} or {ver:X}"])

    def testLibDefines(self):
        out = asar.patch("/main", b'',
            additional_defines={"a": "$42", "b": ""},
            memory_files={"/main": b"org $8000 : db !a\n!b"})
        self.assertOk(out, b'\x42')
        alldefs = asar.getalldefines()
        self.assertEqual(alldefs["a"], "$42")
        self.assertEqual(alldefs["b"], "")

    def testDefining(self):
        out = asar.patch("/main", b'',
            memory_files={"/main": b"!a = 123\n!b = \"\""})
        self.assertOk(out, b'')
        alldefs = asar.getalldefines()
        self.assertEqual(alldefs["b"], "")
        self.assertEqual(asar.getdefine("a"), "123")
        # this one probably shouldn't default to empty string...
        self.assertEqual(asar.getdefine("afdsa"), "")

    def testGetLabels(self):
        out = asar.patch("/main", b'', memory_files={
            "/main": b"""
                norom : org 0
                lbl1:
                -
                jmp +
                +
                jmp -
                .lbl2
                namespace n
                aaa:
                """})
        self.assertTrue(out[0])
        self.assertEqual(asar.getalllabels(), {':neg_1_1': 0, ':pos_1_0': 3,
                                               'lbl1': 0, 'lbl1_lbl2': 6, 'n_aaa': 6})
        self.assertEqual(asar.getlabelval("gfdsgfds"), None)
        self.assertEqual(asar.getlabelval("n_aaa"), 6)

    def testWarnSettings(self):
        out = asar.patch("main", b'',
                warning_overrides={"Wwarn_command": False, "Wimplicitly_sized_immediate": True},
                memory_files={"main": b"org $8000 : lda #0 : warn \"lol\""})
        self.assertTrue(out[0])
        warns = [x.errname.decode() for x in asar.getwarnings()]
        self.assertEqual(warns, ["Wrelative_path_used", "Wimplicitly_sized_immediate"])

    def testMappers(self):
        for patch, type in [
                (b"lorom", asar.mappertype.lorom),
                (b"hirom", asar.mappertype.hirom),
                (b"exhirom", asar.mappertype.exhirom),
                (b"exlorom", asar.mappertype.exlorom),
                (b"sa1rom 4,5,6,7", asar.mappertype.sa1rom),
                (b"fullsa1rom", asar.mappertype.bigsa1rom),
                (b"sfxrom", asar.mappertype.sfxrom),
                (b"norom", asar.mappertype.norom),
                ]:
            self.assertOk(asar.patch("/a", b'', memory_files={"/a": patch}), b'')
            self.assertEqual(asar.getmapper(), type)

    def testWrittenBlocks(self):
        out = self.patchSingle(b"""
                    org $8000 : db 1,2,0,0,2,1
                    org $8020 : db 3,2,1
                    org $8006 : db 3,4
                    org $8021 : db 5
                    org $ffff : db $ff
                    org $018000 : db 1,2,3
                    """)
        self.assertTrue(out[0])
        wb = asar.writtenblockdata
        self.assertEqual(asar.getwrittenblocks(), [wb(0, 0x808000, 8), wb(0x20, 0x808020, 3),
                    wb(0x7fdc, 0x80ffdc, 4), wb(0x7fff, 0x80ffff, 1), wb(0x8000, 0x818000, 3)])

    def testWrittenBlocks2(self):
        out = self.patchSingle(b"""
            hirom
            org $40ffff : db $01
            org $410000 : db $02
            """, override_checksum=False)
        self.assertTrue(out[0])
        wb = asar.writtenblockdata
        self.assertEqual(asar.getwrittenblocks(), [wb(0xffff, 0xc0ffff, 1), wb(0x10000, 0xc10000, 1)])

    def testWrittenBlocks3(self):
        out = self.patchSingle(b"org $fffe : db $00, $00")
        self.assertTrue(out[0])
        wb = asar.writtenblockdata
        self.assertEqual(asar.getwrittenblocks(), [wb(0x7fdc, 0x80ffdc, 4), wb(0x7ffe, 0x80fffe, 2)])

    def testErrFormat(self):
        out = self.patchSingle(b"error \"err or! \", hex(1+1)\nblah")
        self.assertFalse(out[0])
        (err, err2) = asar.geterrors()
        self.assertEqual(err.errname, b"Eunknown_command")
        self.assertEqual(err2.errname, b"Eerror_command")
        self.assertEqual(err2.rawerrdata, b"error command: err or! 2")
        self.assertEqual(err.filename, b"/main")
        # 0-indexed line numbers...
        self.assertEqual(err.line, 1)
        self.assertEqual(err.fullerrdata, b"main:2: error: (Eunknown_command): Unknown command.\n    in block: [blah]")

    def testResolvedefines(self):
        out = self.patchSingle(b"!x = aaa")
        self.assertOk(out, b"")
        self.assertEqual(asar.resolvedefines(b"asdf !x"), b"asdf aaa")
        self.assertEqual(asar.resolvedefines(b"g"), b"g")
        self.assertEqual(asar.resolvedefines(b"aaa !aaaaa"), b"")

    def testMath(self):
        self.patchSingle(b"mylbl = 1234")
        self.assertEqual(asar.math("1+1"), 2)
        self.assertEqual(asar.math("mylbl"), 1234)
        with self.assertRaises(asar.AsarArithmeticError) as err:
            asar.math("1+aaa")
        self.assertEqual(err.exception.args[0], "Label 'aaa' wasn't found.")

    def testSymbolsFile(self):
        self.patchSingle(b"""
org $8000
lbl1: lda $1234
.sub: nop #2
db $69
""")
        expected = """; wla symbolic information file
; generated by asar

[labels]
00:8000 lbl1
00:8003 lbl1_sub

[source files]
0000 395c8a96 /main

[rom checksum]
8e7bb52d

[addr-to-line mapping]
00:8000 0000:00000003
00:8003 0000:00000004
00:8005 0000:00000005
"""
        self.assertEqual(asar.getsymbolsfile(), expected)

    def testChecksum(self):
        out = self.patchSingle(b"org $ffff : db $69")
        expected_rom = bytearray(0x8000)
        expected_rom[0x7fff] = 0x69
        expected_rom[0x7fdc:0x7fe0] = [0x98, 0xfd, 0x67, 0x02]
        self.assertEqual(out[1], bytes(expected_rom))

        out = self.patchSingle(b"org $ffff : db $69", override_checksum=False)
        self.assertEqual(out[1], b"\x00"*0x7fff + b"\x69")
        out = self.patchSingle(b"norom : org $ffff : db $69", override_checksum=True)
        expected_rom = bytearray(0x10000)
        expected_rom[0xffff] = 0x69
        expected_rom[0xffdc:0xffe0] = [0x98, 0xfd, 0x67, 0x02]
        self.assertEqual(out[1], bytes(expected_rom))

# TODO:
# error call stack

@unittest.skipIf(asar_exe_path is None, "ASAREXE environment variable not set")
class TestAsarEXE(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory(prefix="asar-test-")

    def tearDown(self):
        self.temp_dir.cleanup()
        del self.temp_dir

    def runAsar(self, *args):
        assert asar_exe_path is not None # just to shut up pyright
        res = subprocess.run([asar_exe_path, *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
        return (res.returncode, res.stdout, res.stderr)

    def testBasicCli(self):
        patch_name = os.path.join(self.temp_dir.name, "patch.asm")
        rom_name = os.path.join(self.temp_dir.name, "rom.sfc")
        with open(patch_name, 'w') as f:
            f.write("org $8000\ndb $42")
        out = self.runAsar(patch_name, rom_name)
        self.assertEqual(out, (0, "", ""))
        with open(rom_name, 'rb') as f:
            self.assertEqual(f.read(), b"\x42")

    @unittest.skipIf(sys.platform.startswith("win"), "our windows console input doesn't work with pipes")
    def testInteractive(self):
        assert asar_exe_path is not None
        patch_name = os.path.join(self.temp_dir.name, "patch.asm")
        rom_name = os.path.join(self.temp_dir.name, "rom.sfc")
        with open(patch_name, 'w') as f:
            f.write("org $8000\ndb $42")
        p = subprocess.Popen([asar_exe_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, encoding="utf-8")
        p.stdin.write(patch_name + "\n") # type: ignore
        p.stdin.write(rom_name + "\n") # type: ignore
        stdout, stderr = p.communicate()
        self.assertEqual(stderr, '')
        lines = stdout.splitlines()
        self.assertIn("Assembling completed without problems.", lines[-1])
        # These 2 are also on the last line because they don't print a terminating newline themselves
        self.assertIn("Enter patch name: ", lines[-1])
        self.assertIn("Enter ROM name: ", lines[-1])

if __name__ == '__main__':
    unittest.main()
