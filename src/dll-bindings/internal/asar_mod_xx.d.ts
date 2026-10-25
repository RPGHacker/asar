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
