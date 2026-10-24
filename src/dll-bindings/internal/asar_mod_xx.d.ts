// typing definitions for implementing asar_mod.js.
// don't worry about these.

// emscripten library functions:
function stackAlloc(size: number): number;
function stackSave(): number;
function stackRestore(old: number): number;
function stringToUTF8OnStack(s: string): number;
function lengthBytesUTF8(s: string): number;
function stringToUTF8(str: string, outPtr: number, maxBytesToWrite: number): number;
function writeArrayToMemory(arr: Uint8Array | Uint32Array, target: number): void;
function UTF8ToString(ptr: number): string;
function _malloc(size: number): number;
function _free(ptr: number): void;
var HEAPU32: Uint32Array;
var HEAPU8: Uint8Array;

// raw exports from the wasm module:
function _asar_init(): number;
function _asar_version(): number;
function _asar_apiversion(): number;
function _asar_reset(): number;
function _asar_close(): void;
function _asar_patch(_0: number): number;
function _asar_maxromsize(): number;
function _asar_geterrors(_0: number): number;
function _asar_getwarnings(_0: number): number;
function _asar_getprints(_0: number): number;
function _asar_getalllabels(_0: number): number;
function _asar_getlabelval(_0: number): number;
function _asar_getdefine(_0: number): number;
function _asar_resolvedefines(_0: number): number;
function _asar_getalldefines(_0: number): number;
function _asar_math(_0: number, _1: number): number;
function _asar_getwrittenblocks(_0: number): number;
function _asar_getmapper(): number;
function _asar_getsymbolsfile(_0: number): number;
function _do_patch(_0: number, _1: number, _2: number, _3: number, _4: number, _5: number, _6: number, _7: number, _8: number, _9: number, _10: number, _11: number, _12: number, _13: number, _14: number, _15: number, _16: number): number;
function _stackentry_size(): number;
function _stackentry_off_fullpath(): number;
function _stackentry_off_prettypath(): number;
function _stackentry_off_lineno(): number;
function _stackentry_off_details(): number;
function _errordata_size(): number;
function _errordata_off_fullerrdata(): number;
function _errordata_off_rawerrdata(): number;
function _errordata_off_block(): number;
function _errordata_off_filename(): number;
function _errordata_off_line(): number;
function _errordata_off_callstack(): number;
function _errordata_off_callstacksize(): number;
function _errordata_off_errname(): number;
function _labeldata_size(): number;
function _labeldata_off_name(): number;
function _labeldata_off_location(): number;
function _definedata_size(): number;
function _definedata_off_name(): number;
function _definedata_off_contents(): number;
function _fill_definedata(_0: number, _1: number, _2: number): void;
function _warnsetting_size(): number;
function _fill_warnsetting(_0: number, _1: number, _2: number): void;
function _memoryfile_size(): number;
function _fill_memoryfile(_0: number, _1: number, _2: number, _3: number): void;
function _writtenblockdata_size(): number;
function _writtenblockdata_off_pcoffset(): number;
function _writtenblockdata_off_snesoffset(): number;
function _writtenblockdata_off_numbytes(): number;
