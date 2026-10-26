// Type declarations for the Javascript/Wasm build of Asar.
// To use this, build Asar with Emscripten. It will generate lib/asar.wasm, and
// lib/asar.js which implements the interface described here.


export type PatchParams = {
	// Filename of the patch to apply. Since wasm doesn't have access to the
	// filesystem, this file (and every other file you want to use) must be
	// provided in memory_files.
	patch_name: string,
	// ROM data to apply the patch to. The ROM is assumed to be unheadered.
	rom_data: Uint8Array,
	// Additional paths to use when searching for included files. Each element
	// is an include search path; see the "Includes" chapter of the manual for
	// details.
	include_paths?: string[],
	// Additional defines to make available to the patch.
	additional_defines?: {[name: string]: string},
	// These two don't do anything on wasm. (You can just use include_paths and
	// additional_defines to achieve what they were meant for.)
	std_include_file?: string,
	std_define_file?: string,
	// Map of warnings to enable or disable.
	warning_overrides?: {[warnid: string]: boolean},
	// List of files to create on the virtual filesystem. File content can be
	// provided either as text or binary (Uint8Array); text will be encoded as
	// UTF-8.
	memory_files?: {[path: string]: Uint8Array | string},
	// Set to true or false to force generating / not generating a checksum.
	generate_checksum?: boolean,
	// Set this to true for generated error and warning texts to always
	// contain their full call stack.
	full_call_stack?: boolean,
};

export type ErrorData = {
	// full error message as asar would print it.
	fullerrdata: string;
	// just the error text itself without any of the location info or other
	// details
	rawerrdata: string;
	// the command which caused the error
	block: string;
	filename: string;
	// zero-indexed line number in the file
	line: number;
	callstack: {
		// absolute path to the file
		fullpath: string;
		// path relative to the top-level asm file
		prettypath: string;
		// zero-indexed line number
		lineno: number;
		details: string;
	}[];
	// name (ID) of the error/warning
	errname: string;
};

export interface AsarInterface {
	// Returns the version, in the format major*10000+minor*100+bugfix*1. This
	// means that 1.2.34 would be returned as 10234.
	version(): number;

	// Returns the API version, format major*100+minor. Minor is incremented on
	// backwards compatible changes; major is incremented on incompatible
	// changes. Does not have any correlation with the Asar version.
	apiversion(): number;

	// Clears out all errors, warnings and printed statements, and clears the
	// file cache. Not really useful, since patch() already does this.
	reset(): boolean;

	// Apply a patch. See documentation of PatchParams for options.
	patch(options: PatchParams):
		{ok: true, rom_data: Uint8Array} |
		{ok: false, rom_data: null};
	// Returns the maximum possible size of the output ROM from patch().
	maxromsize(): number;

	// Get a list of all errors that were thrown by the last call to patch().
	geterrors(): ErrorData[];
	// Get a list of all warnings from the last patch.
	getwarnings(): ErrorData[];
	// Get a list of all prints from the last patch.
	getprints(): string[];

	// Get a list of all labels defined in the last patch.
	getalllabels(): {[name: string]: number};
	// Get the value (location) of a label. Returns null if the label wasn't
	// found.
	getlabelval(name: string): number | null;

	// Get the value of a define.
	// If the define does not exist, returns empty string.
	getdefine(name: string): string;
	// Parses all defines in the parameter.
	// If there were any errors, returns an empty string.
	resolvedefines(data: string): string;
	// Get a list of all defines.
	getalldefines(): {[name: string]: string};

	// Parses a string containing math. It automatically assumes global scope
	// (no namespaces), and has access to all functions and labels from the last
	// call to patch(). Returns pair of [result, error_msg], exactly one of
	// which will be null.
	math(math_str: string): [number, null] | [null, string];

	// Get a list of all the blocks written to the ROM by the last patch().
	getwrittenblocks(): {
		pcoffset: number;
		snesoffset: number;
		numbytes: number;
	}[];

	// Get the mapper currently used by Asar.
	getmapper(): string | null;

	// Generates the contents of a symbols file for in a specific format. See
	// the documentation of --symbols in the "Usage" chapter in the manual for
	// valid values of `type`.
	// Note that some formats include a crc checksum of the ROM. When using this
	// function, this will be the checksum of the unheadered ROM; you'll need to
	// fix that manually if the original ROM file was actually headered.
	getsymbolsfile(type: string): string;
}

// The actual export is a Promise, since it loads the wasm blob asynchronously.
export function Asar(): Promise<AsarInterface>;
export default Asar;
