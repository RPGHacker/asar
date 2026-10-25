// this file defines the JS-side interface of the Wasm build.
// it gets injected into lib/asar.js by the Emscripten build process.

// closure compiler will rename all dot-accessed properties,
// so we must use a['b'] instead of a.b for all exported symbols

// basically a knockoff libffi:
// s = char* (null terminated)
// i = int32
// p = void*
// b = bool
// {'l':x} = array of x (ptr+len)

// these are used in return values so keep the names
let struct_stackentry = {
	'fullpath': 's',
	'prettypath': 's',
	'lineno': 'i',
	'details': 's',
};
let struct_errordata = {
	'fullerrdata': 's',
	'rawerrdata': 's',
	'block': 's',
	'filename': 's',
	'line': 'i',
	'callstack': {'l': struct_stackentry},
	'errname': 's',
};
let struct_labeldata = {
	name: 's',
	location: 'i',
};
let struct_definedata = {
	name: 's',
	contents: 's',
};
let struct_warnsetting = {
	warnid: 's',
	enabled: 'b',
};
let struct_memoryfile = {
	path: 's',
	buffer: 'p',
	length: 'i',
};
let struct_writtenblockdata = {
	'pcoffset': 'i',
	'snesoffset': 'i',
	'numbytes': 'i',
};
let struct_patchparams = {
	structsize: 'i',
	patchloc: 's',
	romdata: 'p',
	buflen: 'i',
	romlen: 'p',
	includepaths: {'l': 's'},
	additional_defines: {'l': struct_definedata},
	stdincludesfile: 's',
	stddefinesfile: 's',
	warning_settings: {'l': struct_warnsetting},
	memory_files: {'l': struct_memoryfile},
	override_checksum_gen: 'b',
	generate_checksum: 'b',
	full_call_stack: 'b',
};

let sizeof = (desc) => {
	if(desc == 's' || desc == 'i' || desc == 'p') return [4,4];
	if(desc == 'b') return [1,1];
	if('l' in desc) return [8,4];
	let sz = 0;
	let align = 0;
	for(let v of Object.values(desc)) {
		let [sz1, align1] = sizeof(v);
		sz = ((sz-1) | (align1-1)) + 1;
		sz += sz1;
		align = Math.max(align, align1);
	}
	sz = ((sz-1) | (align-1)) + 1;
	return [sz, align];
};
let foreach_struct = (desc, cb) => {
	let offset = 0;
	for(let [k,desc2] of Object.entries(desc)) {
		let [sz, align] = sizeof(desc2);
		offset = ((offset-1) | (align-1)) + 1;
		cb(k, desc2, offset);
		offset += sz;
	}
};
let decode_arr = (ptr, count, desc) => {
	let size = sizeof(desc)[0];
	let res = [];
	for(let i = 0; i < count; i++) {
		res.push(decode(ptr+i*size, desc));
	}
	return res;
};

let read32 = (p) => HEAP32[p/4];
let write32 = (p, v) => HEAP32[p/4] = v;

let decode = (ptr, desc) => {
	if(desc == 's') return (read32(ptr) == 0 ? null : UTF8ToString(read32(ptr)));
	else if(desc == 'i' || desc == 'p') return read32(ptr);
	else if(desc == 'b') return !!HEAPU8[ptr];
	else if('l' in desc) {
		let ptr1 = read32(ptr);
		let count = read32(ptr+4);
		return decode_arr(ptr1, count, desc['l']);
	} else {
		let res = {};
		foreach_struct(desc, (k, desc2, offset) => {
			res[k] = decode(ptr+offset, desc2);
		});
		return res;
	}
};

let encode = (val, ptr, desc) => {
	if(desc == 's') write32(ptr, (typeof val == 'string' ? stringToUTF8OnStack(val) : 0));
	else if(desc == 'i' || desc == 'p') write32(ptr, val);
	else if(desc == 'b') HEAPU8[ptr] = val;
	else if('l' in desc) {
		let sz = sizeof(desc['l'])[0];
		let ptr1 = stackAlloc(sz * val.length);
		val.forEach((v, i) => encode(v, ptr1+sz*i, desc['l']));
		write32(ptr, ptr1);
		write32(ptr+4, val.length);
	} else {
		foreach_struct(desc, (k, desc2, offset) => {
			encode(val[k], ptr+offset, desc2);
		});
	}
};

Module['version'] = () => {
	return _asar_version();
};
Module['apiversion'] = () => {
	return _asar_apiversion();
};
Module['reset'] = () => {
	return !!_asar_reset();
};

Module['patch'] = (options) => {
	// we put almost everything on the stack, except for rom data and memory file contents.
	// then we can free all the misc stuff at once by just doing stackRestore().
	let stackorig = stackSave();
	let patchloc = options['patch_name'];
	let buflen = _asar_maxromsize();
	let romdata = _malloc(buflen);
	writeArrayToMemory(options['rom_data'], romdata);
	let romlen = stackAlloc(4);
	write32(romlen, options['rom_data'].length);

	let includepaths = options['include_paths'] || [];
	let additional_defines = Object.entries(options['additional_defines'] || {}).map(([k,v]) => ({
		name: k,
		contents: v
	}));

	let stdincludesfile = options['std_include_file'];
	let stddefinesfile = options['std_define_file'];
	let warning_settings = Object.entries(options['warning_overrides'] || {}).map(([k,v]) => ({
		warnid: k,
		enabled: v,
	}));

	let ptrs_to_free = [];
	let memory_files = Object.entries(options['memory_files'] || {}).map(([k,v]) => {
		// technically we don't even need the null terminator here, but stringToUTF8 really wants to write it.....
		let size = (typeof v == 'string') ? lengthBytesUTF8(v)+1 : v.length;
		let vptr = _malloc(size);
		if(typeof v == 'string') {
			stringToUTF8(v, vptr, size);
			size -= 1; // the trailing null isn't part of the file
		} else {
			writeArrayToMemory(v, vptr);
		}
		ptrs_to_free.push(vptr);
		return {path: k, buffer: vptr, length: size};
	});

	let override_checksum_gen = typeof options['generate_checksum'] != 'undefined';
	let generate_checksum = !!options['generate_checksum'];

	let full_call_stack = !!options['full_call_stack'];

	let sizeof_pp = sizeof(struct_patchparams)[0];
	let pp = stackAlloc(sizeof_pp);
	let pp_value = {
		structsize: sizeof_pp,
		patchloc,
		romdata,
		buflen,
		romlen,
		includepaths,
		additional_defines,
		stdincludesfile,
		stddefinesfile,
		warning_settings,
		memory_files,
		override_checksum_gen,
		generate_checksum,
		full_call_stack,
	};
	encode(pp_value, pp, struct_patchparams);
	let isok = !!_asar_patch(pp);

	let new_romdata = null;
	if(isok) {
		let new_romlen = read32(romlen);
		new_romdata = HEAPU8.slice(romdata, romdata+new_romlen);
	}

	for(let ptr of ptrs_to_free) _free(ptr);
	_free(romdata);
	stackRestore(stackorig);

	return {
		'ok': isok,
		'rom_data': new_romdata,
	};
};

Module['maxromsize'] = () => {
	return _asar_maxromsize();
};

let wrap_getall = (func, desc) => () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let errs_ptr = func(count_ptr);
	let count = read32(count_ptr);
	let res = decode_arr(errs_ptr, count, desc);
	stackRestore(stackorig);
	return res;
};

let wrap_str_to_str = (func) => (param) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(param);
	let res = func(sptr);
	let res_str = UTF8ToString(res);
	stackRestore(stackorig);
	return res_str;
};

Module['geterrors'] = wrap_getall(_asar_geterrors, struct_errordata);

Module['getwarnings'] = wrap_getall(_asar_getwarnings, struct_errordata);

Module['getprints'] = wrap_getall(_asar_getprints, 's');

Module['getalllabels'] = () => {
	let res_raw = wrap_getall(_asar_getalllabels, struct_labeldata)();
	let res = {};
	for(let k of res_raw) res[k.name] = k.location;
	return res;
};

Module['getlabelval'] = (name) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(name);
	let res = _asar_getlabelval(sptr);
	stackRestore(stackorig);
	if(res == -1) return null;
	return res;
};

Module['getdefine'] = wrap_str_to_str(_asar_getdefine);

Module['resolvedefines'] = wrap_str_to_str(_asar_resolvedefines);

Module['getalldefines'] = () => {
	let res_raw = wrap_getall(_asar_getalldefines, struct_definedata)();
	let res = {};
	for(let k of res_raw) res[k.name] = k.contents;
	return res;
};

Module['math'] = (math_str) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(math_str);
	let err_ptr = stackAlloc(4);
	let res_num = _asar_math(sptr, err_ptr);
	let res;
	if(read32(err_ptr) == 0) res = [res_num, null];
	else res = [null, UTF8ToString(read32(err_ptr))];
	stackRestore(stackorig);
	return res;
};

Module['getwrittenblocks'] = wrap_getall(_asar_getwrittenblocks, struct_writtenblockdata);

Module['getmapper'] = () => {
	let mapper = _asar_getmapper();
	return [null,
		"lorom",
		"hirom",
		"sa1rom",
		"bigsa1rom",
		"sfxrom",
		"exlorom",
		"exhirom",
		"norom",
	][mapper];
};

Module['getsymbolsfile'] = wrap_str_to_str(_asar_getsymbolsfile);
