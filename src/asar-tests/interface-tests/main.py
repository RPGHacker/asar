import unittest
import os
import asar

asar_dll_path = None
if "ASARDLL" in os.environ:
    asar_dll_path = os.environ["ASARDLL"]
asar_exe_path = None
if "ASAREXE" in os.environ:
    asar_exe_path = os.environ["ASAREXE"]

tests_dir = os.path.dirname(os.path.abspath(__file__))

class TestAsarDLL(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if asar_dll_path is None:
            raise unittest.SkipTest("ASARDLL environment variable not set")
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
        blocks = []
        for blk in asar.getwrittenblocks():
            blocks.append((blk.pcoffset, blk.snesoffset, blk.numbytes))
        self.assertEqual(blocks, [(0, 0x808000, 8), (0x20, 0x808020, 3),
                    (0x7fdc, 0x80ffdc, 4), (0x7fff, 0x80ffff, 1), (0x8000, 0x818000, 3)])

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

# TODO:
# error call stack
# warning format?
# getlabelval
# resolvedefines
# math
# checksums
# symbols file

if __name__ == '__main__':
    unittest.main()
