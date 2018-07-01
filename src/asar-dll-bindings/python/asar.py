#!/usr/bin/env python3
"""
python interface for asar.dll
by randomdude999

Usage: import asar, call asar.init, call asar.patch, then use the various
functions to get info about the patch
"""

import ctypes
import enum
import sys
from ctypes import c_int, c_char_p, POINTER
c_int_ptr = POINTER(c_int)

__all__ = ["errordata", "writtenblockdata", "mappertype", "version",
           "apiversion", "init", "reset", "patch", "maxromsize", "close",
           "geterrors", "getwarnings", "getprints", "getalllabels",
           "getlabelval", "getdefine", "getalldefines", "resolvedefines",
           "math", "getwrittenblocks", "getmapper", "getsymbolsfile"]
_target_api_ver = 303
_asar = None


class AsarArithmeticError(ArithmeticError):
    pass


class errordata(ctypes.Structure):
    _fields_ = [("fullerrdata", c_char_p),
                ("rawerrdata", c_char_p),
                ("block", c_char_p),
                ("filename", c_char_p),
                ("line", c_int),
                ("callerfilename", c_char_p),
                ("callerline", c_int),
                ("errid", c_int)]

    def __repr__(self):
        return "<asar error: {!r}>".format(self.fullerrdata.decode())


# for internal use only. getalllabels() returns a dict.
class _labeldata(ctypes.Structure):
    _fields_ = [("name", c_char_p),
                ("location", c_int)]


# for internal use only. getalldefines() returns a dict.
class _definedata(ctypes.Structure):
    _fields_ = [("name", c_char_p),
                ("contents", c_char_p)]


class writtenblockdata(ctypes.Structure):
    _fields_ = [("pcoffset", c_int),
                ("snesoffset", c_int),
                ("numbytes", c_int)]

    def __repr__(self):
        return "<written block ${:06x} 0x{:x} size:{}>".format(
            self.snesoffset, self.pcoffset, self.numbytes)


# internal use only. patch() accepts a dict.
class _memoryfile(ctypes.Structure):
    _fields_ = [("path", c_char_p),
                ("buffer", c_char_p),
                ("length", ctypes.c_size_t)]


# internal use only. patch() accepts a dict.
class _warnsetting(ctypes.Structure):
    _fields_ = [("warnid", c_char_p),
                ("enabled", ctypes.c_bool)]


# For internal use only.
class _patchparams(ctypes.Structure):
    _fields_ = [("structsize", c_int),
                ("patchloc", c_char_p),
                ("romdata", c_char_p),
                ("buflen", c_int),
                ("romlen", c_int_ptr),
                ("includepaths", POINTER(c_char_p)),
                ("numincludepaths", c_int),
                ("should_reset", ctypes.c_bool),
                ("additional_defines", POINTER(_definedata)),
                ("additional_define_count", c_int),
                ("stdincludesfile", c_char_p),
                ("stddefinesfile", c_char_p),
                ("warning_settings", POINTER(_warnsetting)),
                ("warning_setting_count", c_int),
                ("memory_files", POINTER(_memoryfile)),
                ("memory_file_count", c_int),
                ("override_checksum_gen", ctypes.c_bool),
                ("generate_checksum", ctypes.c_bool)]


class mappertype(enum.Enum):
    invalid_mapper = 0
    lorom = 1
    hirom = 2
    sa1rom = 3
    bigsa1rom = 4
    sfxrom = 5
    exlorom = 6
    exhirom = 7
    norom = 8


def _getall(func):
    """Helper that does the work common to all the getall* functions."""
    count = c_int()
    raw_errs = func(ctypes.byref(count))
    errs = []
    for i in range(count.value):
        errs.append(raw_errs[i])
    return errs


class _AsarDLL:
    def __init__(self, dllname):
        dll = ctypes.CDLL(dllname)
        self.dll = dll
        self.funcs = {}
        try:
            # argument/return type setup
            # (also verifies that those functions are exported from the DLL)
            # this is directly from asardll.h
            # setup_func(name, argtypes, returntype)
            self.setup_func("version", (), c_int)
            self.setup_func("apiversion", (), c_int)
            self.setup_func("init", (), ctypes.c_bool)
            self.setup_func("reset", (), ctypes.c_bool)
            self.setup_func("patch", (c_char_p, c_char_p, c_int, c_int_ptr),
                            ctypes.c_bool)
            self.setup_func("patch_ex", (POINTER(_patchparams),), ctypes.c_bool)
            self.setup_func("maxromsize", (), c_int)
            self.setup_func("close", (), None)
            self.setup_func("geterrors", (c_int_ptr,), POINTER(errordata))
            self.setup_func("getwarnings", (c_int_ptr,), POINTER(errordata))
            self.setup_func("getprints", (c_int_ptr,), POINTER(c_char_p))
            self.setup_func("getalllabels", (c_int_ptr,), POINTER(_labeldata))
            self.setup_func("getlabelval", (c_char_p,), c_int)
            self.setup_func("getdefine", (c_char_p,), c_char_p)
            self.setup_func("getalldefines", (c_int_ptr,), POINTER(_definedata))
            self.setup_func("resolvedefines", (c_char_p, ctypes.c_bool),
                            c_char_p)
            self.setup_func("math", (c_char_p, POINTER(c_char_p)),
                            ctypes.c_double)
            self.setup_func("getwrittenblocks", (c_int_ptr,),
                            POINTER(writtenblockdata))
            self.setup_func("getmapper", (), c_int)
            self.setup_func("getsymbolsfile", (c_char_p,), c_char_p)

        except AttributeError:
            raise OSError("Asar DLL is missing some functions")
        api_ver = dll.asar_apiversion()
        if api_ver < _target_api_ver or \
                (api_ver // 100) > (_target_api_ver // 100):
            raise OSError("Asar DLL version "+str(api_ver)+" unsupported")

    def setup_func(self, name, argtypes, restype):
        """Setup argument and return types for a function.

        name: name of the function in the DLL. "asar_" is added automatically
        argtypes and restype: see ctypes documentation
        """
        func = getattr(self.dll, "asar_" + name)
        func.argtypes = argtypes
        func.restype = restype


def init(dll_path=None):
    """Load the Asar DLL.

    You must call this before calling any other Asar functions. Raises OSError
    if there was something wrong with the DLL (not found, wrong version,
    doesn't have all necessary functions).
    You can pass a custom DLL path if you want. If you don't, some common names
    for the asar dll are tried.
    """
    global _asar
    if _asar is not None:
        return

    if dll_path is not None:
        _asar = _AsarDLL(dll_path)
    else:
        if sys.platform == "win32":
            _asar = _AsarDLL("asar")
        else:
            if sys.platform == "darwin":
                libnames = ["./libasar.dylib", "libasar"]
            else:
                libnames = ["./libasar.so", "libasar"]
            for x in libnames:
                try:
                    _asar = _AsarDLL(x)
                except OSError:
                    continue
        if _asar is None:
            # Nothing in the search path is valid
            raise OSError("Could not find asar DLL")

    if not _asar.dll.asar_init():
        _asar = None
        return False
    else:
        return True


def close():
    """Free all of Asar's structures and unload the module.

    Only asar.init() may be called after calling this.
    """
    global _asar
    if _asar is None:
        return
    _asar.dll.asar_close()
    _asar = None


def version():
    """Return the version, in the format major*10000+minor*100+bugfix*1.

    This means that 1.2.34 would be returned as 10234.
    """
    return _asar.dll.asar_version()


def apiversion():
    """Return the API version, in the format major*100+minor.

    Minor is incremented on backwards compatible changes; major is incremented
    on incompatible changes. Does not have any correlation with the Asar
    version. It's not very useful directly, since asar.init() verifies this
    automatically.
    """
    return _asar.dll.asar_apiversion()


def reset():
    """Clear out errors, warnings, printed statements and the file cache.

    Not really useful, since asar.patch() already does this.
    """
    return _asar.dll.asar_reset()


def patch(patch_name, rom_data, includepaths=[], should_reset=True,
          additional_defines={}, std_include_file=None, std_define_file=None,
          warning_overrides={}, memory_files={}, override_checksum=None):
    """Applies a patch.

    Returns (success, new_rom_data). If success is False you should call
    geterrors() to see what went wrong. rom_data is assumed to be headerless.

    If includepaths is specified, it lists additional include paths for asar
    to search.

    should_reset specifies whether asar should clear out all defines, labels,
    etc from the last inserted file. Setting it to False will make Asar act
    like the currently patched file was directly appended to the previous one.

    additional_defines specifies extra defines to give to the patch
    (similar to the -D option).

    std_include_file and std_define_file specify files where to look for extra
    include paths and defines, respectively.

    warning_overrides is a dict of str (warning ID) -> bool. It overrides
    enabling/disabling specific warnings.

    memory_files is a dict of str (file name) -> bytes (file contents). It
    specifies memory files to use.

    override_checksum specifies whether to override checksum generation. True
    forces Asar to update the ROM's checksum, False forces Asar to not update
    it.
    """
    romlen = c_int(len(rom_data))
    rom_ptr = ctypes.create_string_buffer(rom_data, maxromsize())
    pp = _patchparams()
    pp.structsize = ctypes.sizeof(_patchparams)
    pp.patchloc = patch_name.encode()
    pp.romdata = ctypes.cast(rom_ptr, c_char_p)
    pp.buflen = maxromsize()
    pp.romlen = ctypes.pointer(romlen)

    # construct an array type of len(includepaths) elements and initialize
    # it with elements from includepaths
    pp.includepaths = (c_char_p*len(includepaths))(*includepaths)
    pp.numincludepaths = len(includepaths)

    defines = (_definedata * len(additional_defines))()
    for i, (k, v) in enumerate(additional_defines.items()):
        defines[i].name = k.encode()
        defines[i].contents = v.encode()
    pp.additional_defines = defines
    pp.additional_define_count = len(additional_defines)

    pp.should_reset = should_reset

    pp.stdincludesfile = std_include_file.encode() if std_include_file else None
    pp.stddefinesfile = std_define_file.encode() if std_define_file else None

    warnsettings = (_warnsetting * len(warning_overrides))()
    for i, (k, v) in enumerate(warning_overrides.items()):
        warnsettings[i].warnid = k.encode()
        warnsettings[i].enabled = v
    pp.warning_settings = warnsettings
    pp.warning_setting_count = len(warnsettings)

    memoryfiles = (_memoryfile * len(memory_files))()
    for i, (k, v) in enumerate(memory_files.items()):
        memoryfiles[i].path = k.encode()
        memoryfiles[i].buffer = v
        memoryfiles[i].length = len(v)
    pp.memory_files = memoryfiles
    pp.memory_file_count = len(memory_files)

    if override_checksum is not None:
        pp.override_checksum_gen = True
        pp.generate_checksum = override_checksum
    else:
        pp.override_checksum_gen = False
        pp.generate_checksum = False

    result = _asar.dll.asar_patch_ex(ctypes.byref(pp))
    return result, rom_ptr.raw[:romlen.value]


def maxromsize():
    """Return the maximum possible size of the output ROM."""
    return _asar.dll.asar_maxromsize()


def geterrors():
    """Get a list of all errors."""
    return _getall(_asar.dll.asar_geterrors)


def getwarnings():
    """Get a list of all warnings."""
    return _getall(_asar.dll.asar_getwarnings)


def getprints():
    """Get a list of all printed data."""
    return [x.decode() for x in _getall(_asar.dll.asar_getprints)]


def getalllabels():
    """Get a dictionary of label name -> SNES address."""
    labeldatas = _getall(_asar.dll.asar_getalllabels)
    return {x.name.decode(): x.location for x in labeldatas}


def getlabelval(name):
    """Get the ROM location of one label. None means "not found"."""
    val = _asar.dll.asar_getlabelval(name.encode())
    return None if (val == -1) else val


def getdefine(name):
    """Get the value of a define."""
    return _asar.dll.asar_getdefine(name.encode()).decode()


def getalldefines():
    """Get the names and values of all defines."""
    definedatas = _getall(_asar.dll.asar_getalldefines)
    return {x.name.decode(): x.contents.decode() for x in definedatas}


def resolvedefines(data, learnnew):
    """Parse all defines in the given data.

    Returns the data with all defines evaluated.
    learnnew controls whether it'll learn new defines in this string if it
    finds any. Note that it may emit errors.
    """
    return _asar.dll.asar_resolvedefines(data, learnnew)


def math(to_calculate):
    """Parse a string containing math.

    It automatically assumes global scope (no namespaces), and has access to
    all functions and labels from the last call to asar.patch(). If there was
    an error, ArithmeticError is raised with the message returned by Asar.
    """
    error = ctypes.c_char_p()
    result = _asar.dll.asar_math(to_calculate.encode(), ctypes.byref(error))
    if not bool(error):
        # Null pointer, means no error
        return result
    else:
        raise AsarArithmeticError(error.value.decode())


def getwrittenblocks():
    """Get a list of all the blocks written to the ROM."""
    return _getall(_asar.dll.asar_getwrittenblocks)


def getmapper():
    """Get the ROM mapper currently used by Asar."""
    return mappertype(_asar.dll.asar_getmapper())

def getsymbolsfile(fmt="wla"):
    """Generates the contents of a symbols file for in a specific format.

    Returns the textual contents of the symbols file.
    format specified the format of the symbols file that gets generated.
    """
    return _asar.dll.asar_getsymbolsfile(fmt.encode()).decode()
