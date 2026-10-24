// Type declarations for the Javascript/Wasm build of Asar.
// To use this, build Asar with Emscripten. It will generate lib/asar.wasm, and
// lib/asar.js which implements the interface described here.

// See asardll.h for documentation about what the functions actually do.
// Note that since wasm doesn't have access to the filesystem,
// every file you want to use must be provided in memory_files.

export type PatchParams = {
	patch_name: string,
	rom_data: Uint8Array,
	include_paths?: string[],
	additional_defines?: {[name: string]: string},
	std_include_file?: string,
	std_define_file?: string,
	warning_overrides?: {[warnid: string]: boolean},
	memory_files?: {[path: string]: Uint8Array | string},
	override_checksum?: boolean,
	full_call_stack?: boolean,
};

export type ErrorData = {
	fullerrdata: string;
	rawerrdata: string;
	block: string;
	filename: string;
	line: number;
	callstack: {
		fullpath: string;
		prettypath: string;
		lineno: number;
		details: string;
	}[];
	errname: string;
};

export interface AsarInterface {
	version(): number;
	apiversion(): number;
	reset(): boolean;
	patch(options: PatchParams): {
		ok: boolean,
		// null if ok==false
		rom_data: Uint8Array | null,
	};
	maxromsize(): number;
	geterrors(): ErrorData[];
	getwarnings(): ErrorData[];
	getprints(): string[];
	getalllabels(): {[name: string]: number};
	getlabelval(name: string): number | null;
	getdefine(name: string): string;
	resolvedefines(data: string): string;
	getalldefines(): {[name: string]: string};
        // returns pair of [result, error_msg],
        // exactly one of which will be null.
	math(math_str: string): ([number, null] | [null, string]);
	getwrittenblocks(): {
		pcoffset: number;
		snesoffset: number;
		numbytes: number;
	}[];
	getmapper(): string | null;
	getsymbolsfile(type: string): string;
}

// The actual export is a Promise, since it loads the wasm blob asynchronously.
export function Asar(): Promise<AsarInterface>;
export default Asar;
