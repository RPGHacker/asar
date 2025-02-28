#!/usr/bin/env python3
"""
python interface for asar.dll
by trillian

Usage: import asar, call asar.init, call asar.patch, then use the various
functions to get info about the patch
"""

from __future__ import annotations
import ctypes
import enum
import sys
from ctypes import c_int, c_char_p, POINTER
c_int_ptr = POINTER(c_int)
_asar: _AsarDLL = None


class AsarArithmeticError(ArithmeticError):
    "Raised from math() in case an asar error occurred during evaluation."
    pass


class stackentry:
    """One entry in the call stack of an error/warning.

    This can represent e.g. a macro call or an incsrc."""
    def __init__(self, raw_stackentry):
        self.fullpath: str = raw_stackentry.fullpath.decode()
        self.prettypath: str = raw_stackentry.prettypath.decode()
        self.lineno: int = raw_stackentry.lineno
        self.details: str = raw_stackentry.details.decode()


class errordata:
    """Information about one error or warning."""
    def __init__(self, raw_errdata):
        self.fullerrdata: str = raw_errdata.fullerrdata.decode()
        self.rawerrdata: str = raw_errdata.rawerrdata.decode()
        self.block: str = raw_errdata.block.decode()
        self.filename: str = raw_errdata.filename.decode()
        self.line: int = raw_errdata.line
        self.errname: str = raw_errdata.errname.decode()
        self.callstack: list[stackentry] = []
        for i in range(raw_errdata.callstacksize):
            self.callstack.append(stackentry(raw_errdata.callstack[i]))

    def __repr__(self):
        return "<asar error: {!r}>".format(self.fullerrdata)


class writtenblockdata:
    """One region of the ROM that Asar wrote to."""
    def __init__(self, raw_writtenblock):
        self.pcoffset: int = raw_writtenblock.pcoffset
        self.snesoffset: int = raw_writtenblock.snesoffset
        self.numbytes: int = raw_writtenblock.numbytes

    def __repr__(self):
        return "<written block ${:06x} 0x{:x} size:{}>".format(
            self.snesoffset, self.pcoffset, self.numbytes)

    def __eq__(self, other):
        return (self.pcoffset == other.pcoffset
                and self.snesoffset == other.snesoffset
                and self.numbytes == other.numbytes)


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


def init(dll_path=None) -> None:
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
        _asar = None # type: ignore
        raise OSError("Asar DLL failed to initialize")


def close() -> None:
    """Free all of Asar's structures and unload the module.

    Only asar.init() may be called after calling this.
    """
    global _asar
    if _asar is None:
        return
    _asar.dll.asar_close()
    _asar = None # type: ignore


def version() -> int:
    """Return the version, in the format major*10000+minor*100+bugfix*1.

    This means that 1.2.34 would be returned as 10234.
    """
    return _asar.dll.asar_version()


def apiversion() -> int:
    """Return the API version, in the format major*100+minor.

    Minor is incremented on backwards compatible changes; major is incremented
    on incompatible changes. Does not have any correlation with the Asar
    version. It's not very useful directly, since asar.init() verifies this
    automatically.
    """
    return _asar.dll.asar_apiversion()


def reset() -> bool:
    """Clear out errors, warnings, printed statements and the file cache.

    Not really useful, since asar.patch() already does this.
    """
    return _asar.dll.asar_reset()


def patch(patch_name: str, rom_data: bytes, includepaths: list[str] = [],
          additional_defines: dict[str, str] = {},
          std_include_file: str | None = None, std_define_file: str | None = None,
          warning_overrides: dict[str, bool] = {}, memory_files: dict[str, bytes] = {},
          override_checksum: bool | None = None, full_call_stack: bool = False) -> tuple[bool, bytes]:
    """Applies a patch.

    Returns (success, new_rom_data). If success is False you should call
    geterrors() to see what went wrong. rom_data is assumed to be headerless.

    If includepaths is specified, it lists additional include paths for asar
    to search.

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

    full_call_stack specifies whether generated error and warning texts (i.e.
    the "fullerrdata" string in a returned error) always contain their full
    call stack.
    """
    romlen = c_int(len(rom_data))
    rom_ptr = ctypes.create_string_buffer(rom_data, maxromsize())
    pp = _patchparams()
    pp.structsize = ctypes.sizeof(_patchparams)
    pp.patchloc = patch_name.encode()
    pp.romdata = ctypes.cast(rom_ptr, c_char_p)
    pp.buflen = maxromsize()
    pp.romlen = ctypes.pointer(romlen)

    pp.includepaths = (c_char_p*len(includepaths))()
    for i, path in enumerate(includepaths):
        pp.includepaths[i] = path.encode()
    pp.numincludepaths = len(includepaths)

    defines = (_definedata * len(additional_defines))()
    for i, (k, v) in enumerate(additional_defines.items()):
        defines[i].name = k.encode()
        defines[i].contents = v.encode()
    pp.additional_defines = defines
    pp.additional_define_count = len(additional_defines)

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

    pp.full_call_stack = full_call_stack

    result = _asar.dll.asar_patch(ctypes.byref(pp))
    return result, rom_ptr.raw[:romlen.value]


def maxromsize() -> int:
    """Return the maximum possible size of the output ROM."""
    return _asar.dll.asar_maxromsize()


def geterrors() -> list[errordata]:
    """Get a list of all errors."""
    return _getall(_asar.dll.asar_geterrors, errordata)


def getwarnings() -> list[errordata]:
    """Get a list of all warnings."""
    return _getall(_asar.dll.asar_getwarnings, errordata)


def getprints() -> list[str]:
    """Get a list of all printed data."""
    return _getall(_asar.dll.asar_getprints, lambda x: x.decode())


def getalllabels() -> dict[str, int]:
    """Get a dictionary of label name -> SNES address."""
    labeldatas = _getall(_asar.dll.asar_getalllabels, lambda x: x)
    return {x.name.decode(): x.location for x in labeldatas}


def getlabelval(name: str) -> int | None:
    """Get the SNES address of one label. None means "not found"."""
    val = _asar.dll.asar_getlabelval(name.encode())
    return None if (val == -1) else val


def getdefine(name: str) -> str:
    """Get the value of a define."""
    return _asar.dll.asar_getdefine(name.encode()).decode()


def getalldefines() -> dict[str, str]:
    """Get the names and values of all defines."""
    definedatas = _getall(_asar.dll.asar_getalldefines, lambda x: x)
    return {x.name.decode(): x.contents.decode() for x in definedatas}


def resolvedefines(data: str) -> str:
    """Parse all defines in the given data.

    Returns the data with all defines evaluated.
    If there were any errors, returns an empty string.
    """
    return _asar.dll.asar_resolvedefines(data.encode()).decode()


def math(to_calculate: str) -> float:
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


def getwrittenblocks() -> list[writtenblockdata]:
    """Get a list of all the blocks written to the ROM."""
    return _getall(_asar.dll.asar_getwrittenblocks, writtenblockdata)


def getmapper() -> mappertype:
    """Get the ROM mapper currently used by Asar."""
    return mappertype(_asar.dll.asar_getmapper())


def getsymbolsfile(fmt="wla") -> str:
    """Generates the contents of a symbols file for in a specific format.

    Returns the textual contents of the symbols file.
    format specified the format of the symbols file that gets generated.
    """
    return _asar.dll.asar_getsymbolsfile(fmt.encode()).decode()



# implementation details
#-----------------------

_target_api_ver = 400

class _stackentry(ctypes.Structure):
    _fields_ = [("fullpath", c_char_p),
                ("prettypath", c_char_p),
                ("lineno", c_int),
                ("details", c_char_p)]

class _errordata(ctypes.Structure):
    _fields_ = [("fullerrdata", c_char_p),
                ("rawerrdata", c_char_p),
                ("block", c_char_p),
                ("filename", c_char_p),
                ("line", c_int),
                ("callstack", POINTER(_stackentry)),
                ("callstacksize", c_int),
                ("errname", c_char_p)]

class _labeldata(ctypes.Structure):
    _fields_ = [("name", c_char_p),
                ("location", c_int)]

class _definedata(ctypes.Structure):
    _fields_ = [("name", c_char_p),
                ("contents", c_char_p)]

class _writtenblockdata(ctypes.Structure):
    _fields_ = [("pcoffset", c_int),
                ("snesoffset", c_int),
                ("numbytes", c_int)]

class _memoryfile(ctypes.Structure):
    _fields_ = [("path", c_char_p),
                ("buffer", c_char_p),
                ("length", ctypes.c_size_t)]

class _warnsetting(ctypes.Structure):
    _fields_ = [("warnid", c_char_p),
                ("enabled", ctypes.c_bool)]

class _patchparams(ctypes.Structure):
    _fields_ = [("structsize", c_int),
                ("patchloc", c_char_p),
                ("romdata", c_char_p),
                ("buflen", c_int),
                ("romlen", c_int_ptr),
                ("includepaths", POINTER(c_char_p)),
                ("numincludepaths", c_int),
                ("additional_defines", POINTER(_definedata)),
                ("additional_define_count", c_int),
                ("stdincludesfile", c_char_p),
                ("stddefinesfile", c_char_p),
                ("warning_settings", POINTER(_warnsetting)),
                ("warning_setting_count", c_int),
                ("memory_files", POINTER(_memoryfile)),
                ("memory_file_count", c_int),
                ("override_checksum_gen", ctypes.c_bool),
                ("generate_checksum", ctypes.c_bool),
                ("full_call_stack", ctypes.c_bool)]


def _getall(func, wrapper):
    """Helper that does the work common to all the getall* functions."""
    count = c_int()
    raw_errs = func(ctypes.byref(count))
    errs = []
    for i in range(count.value):
        errs.append(wrapper(raw_errs[i]))
    return errs


class _AsarDLL:
    def __init__(self, dllname):
        dll = ctypes.CDLL(dllname)
        self.dll = dll
        try:
            # argument/return type setup
            # (also verifies that those functions are exported from the DLL)
            # this is directly from asardll.h
            # setup_func(name, argtypes, returntype)
            self.setup_func("version", (), c_int)
            self.setup_func("apiversion", (), c_int)
            self.setup_func("init", (), ctypes.c_bool)
            self.setup_func("reset", (), ctypes.c_bool)
            self.setup_func("patch", (POINTER(_patchparams),), ctypes.c_bool)
            self.setup_func("maxromsize", (), c_int)
            self.setup_func("close", (), None)
            self.setup_func("geterrors", (c_int_ptr,), POINTER(_errordata))
            self.setup_func("getwarnings", (c_int_ptr,), POINTER(_errordata))
            self.setup_func("getprints", (c_int_ptr,), POINTER(c_char_p))
            self.setup_func("getalllabels", (c_int_ptr,), POINTER(_labeldata))
            self.setup_func("getlabelval", (c_char_p,), c_int)
            self.setup_func("getdefine", (c_char_p,), c_char_p)
            self.setup_func("getalldefines", (c_int_ptr,), POINTER(_definedata))
            self.setup_func("resolvedefines", (c_char_p,), c_char_p)
            self.setup_func("math", (c_char_p, POINTER(c_char_p)),
                            ctypes.c_double)
            self.setup_func("getwrittenblocks", (c_int_ptr,),
                            POINTER(_writtenblockdata))
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

__all__ = ["stackentry", "errordata", "writtenblockdata", "mappertype",
           "version", "apiversion", "init", "reset", "patch", "maxromsize",
           "close", "geterrors", "getwarnings", "getprints", "getalllabels",
           "getlabelval", "getdefine", "getalldefines", "resolvedefines",
           "math", "getwrittenblocks", "getmapper", "getsymbolsfile",
           "AsarArithmeticError"]
