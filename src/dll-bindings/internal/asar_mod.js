// @ts-check

// this file defines the JS-side interface of the Wasm build.
// it gets injected into lib/asar.js by the Emscripten build process.

/** @import {AsarInterface} from "../asar.d.ts"; */

// closure compiler will rename all dot-accessed properties,
// so we must use a['b'] instead of a.b for all exported symbols

/** @type {AsarInterface['version']} */
Module['version'] = () => {
	return _asar_version();
};
/** @type {AsarInterface['apiversion']} */
Module['apiversion'] = () => {
	return _asar_apiversion();
};
/** @type {AsarInterface['reset']} */
Module['reset'] = () => {
	return !!_asar_reset();
};

/** @param {number} p */
let read32 = (p) => HEAPU32[p/4];

/** @type {AsarInterface['patch']} */
Module['patch'] = (options) => {
	// we put almost everything on the stack, except for rom data and memory file contents.
	// then we can free all the misc stuff at once by just doing stackRestore().
	let stackorig = stackSave();
	let patchloc = stringToUTF8OnStack(options['patch_name']);
	let buflen = _asar_maxromsize();
	let romdata = _malloc(buflen);
	writeArrayToMemory(options['rom_data'], romdata);
	let romlen = stackAlloc(4);
	HEAPU32[romlen/4] = options['rom_data'].length;

	/**
	 * @template T
	 * @param {{[x: string]: T} | T[] | undefined} arr
	 * @param {number} sz
	 * @param {(ptr: number, k: string, v: T) => void} handler
	 * @returns {[number, number]}
	 */
	let handle_arr = (arr, sz, handler) => {
		if(!arr) return [0, 0];
		let arr2 = Object.entries(arr);
		let buf = stackAlloc(arr2.length * sz);
		// call handler on each slot
		arr2.forEach(([k, v], i) => handler(buf + sz*i, k, v));
		return [buf, arr2.length];
	};

	let [includepaths, numincludepaths] = handle_arr(
		options['include_paths'],
		4,
		(p, _, s) => HEAPU32[p/4] = stringToUTF8OnStack(s));

	let [additional_defines, additional_define_count] = handle_arr(
		options['additional_defines'],
		_definedata_size(),
		(ptr, k, v) => {
			let kptr = stringToUTF8OnStack(k);
			let vptr = stringToUTF8OnStack(v);
			_fill_definedata(ptr, kptr, vptr);
		});

	let stdincludesfile = 0;
	if(typeof options['std_include_file'] == 'string') {
		stdincludesfile = stringToUTF8OnStack(options['std_include_file']);
	}
	let stddefinesfile = 0;
	if(typeof options['std_define_file'] == 'string') {
		stddefinesfile = stringToUTF8OnStack(options['std_define_file']);
	}

	let [warning_settings, warning_setting_count] = handle_arr(
		options['warning_overrides'],
		_warnsetting_size(),
		(ptr, k, v) => {
			let kptr = stringToUTF8OnStack(k);
			_fill_warnsetting(ptr, kptr, +v);
		}
	);

	/** @type {number[]} */
	let ptrs_to_free = [];
	let [memory_files, memory_file_count] = handle_arr(
		options['memory_files'],
		_memoryfile_size(),
		(ptr, k, v) => {
			let kptr = stringToUTF8OnStack(k);
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
			_fill_memoryfile(ptr, kptr, vptr, size);
		});

	let override_checksum_gen = +(typeof options['override_checksum'] != 'undefined');
	let generate_checksum = +(!!options['override_checksum']);

	let full_call_stack = +(!!options['full_call_stack']);

	let isok = !!_do_patch(patchloc, romdata, buflen, romlen, includepaths, numincludepaths, additional_defines, additional_define_count, stdincludesfile, stddefinesfile, warning_settings, warning_setting_count, memory_files, memory_file_count, override_checksum_gen, generate_checksum, full_call_stack);

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

/** @type {AsarInterface['maxromsize']} */
Module['maxromsize'] = () => {
	return _asar_maxromsize();
};

/**
 * @param {number} errs_ptr
 * @param {number} count
 */
let decode_errordatas = (errs_ptr, count) => {
	let sz = _errordata_size();
	let sz_cs = _stackentry_size();
	let off_fullerrdata = _errordata_off_fullerrdata();
	let off_rawerrdata = _errordata_off_rawerrdata();
	let off_block = _errordata_off_block();
	let off_filename = _errordata_off_filename();
	let off_line = _errordata_off_line();
	let off_callstack = _errordata_off_callstack();
	let off_callstacksize = _errordata_off_callstacksize();
	let off_errname = _errordata_off_errname();
	let off_cs_fullpath = _stackentry_off_fullpath();
	let off_cs_prettypath = _stackentry_off_prettypath();
	let off_cs_lineno = _stackentry_off_lineno();
	let off_cs_details = _stackentry_off_details();
	let res = [];
	for(let i = 0; i < count; i++) {
		let p = errs_ptr + sz*i;
		let callstack = [];
		let cs_num = read32(p+off_callstacksize);
		let cs_start = read32(p+off_callstack);
		for(let j = 0; j < cs_num; j++) {
			let pp = cs_start + sz_cs * j;
			callstack.push({
				'fullpath': UTF8ToString(read32(pp+off_cs_fullpath)),
				'prettypath': UTF8ToString(read32(pp+off_cs_prettypath)),
				'lineno': read32(pp+off_cs_lineno),
				'details': UTF8ToString(read32(pp+off_cs_details)),
			});
		}
		res.push({
			'fullerrdata': UTF8ToString(read32(p+off_fullerrdata)),
			'rawerrdata': UTF8ToString(read32(p+off_rawerrdata)),
			'block': UTF8ToString(read32(p+off_block)),
			'filename': UTF8ToString(read32(p+off_filename)),
			'line': read32(p+off_line),
			'callstack': callstack,
			'errname': UTF8ToString(read32(p+off_errname)),
		});
	}
	return res;
};

/** @type {AsarInterface['geterrors']} */
Module['geterrors'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let errs_ptr = _asar_geterrors(count_ptr);
	let count = read32(count_ptr);
	let res = decode_errordatas(errs_ptr, count);
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getwarnings']} */
Module['getwarnings'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let errs_ptr = _asar_getwarnings(count_ptr);
	let count = read32(count_ptr);
	let res = decode_errordatas(errs_ptr, count);
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getprints']} */
Module['getprints'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let prints_ptr = _asar_getprints(count_ptr);
	let count = read32(count_ptr);
	let res = [];
	for(let i = 0; i < count; i++) {
		res.push(UTF8ToString(read32(prints_ptr + 4*i)));
	}
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getalllabels']} */
Module['getalllabels'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let labels_ptr = _asar_getalllabels(count_ptr);
	let count = read32(count_ptr);
	let sz = _labeldata_size();
	let off_name = _labeldata_off_name();
	let off_location = _labeldata_off_location();
	/** @type {{[x: string]: number}} */
	let res = {};
	for(let i = 0; i < count; i++) {
		let p = labels_ptr + i*sz;
		let name = UTF8ToString(read32(p+off_name));
		let location = read32(p+off_location);
		res[name] = location;
	}
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getlabelval']} */
Module['getlabelval'] = (name) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(name);
	let res = _asar_getlabelval(sptr);
	stackRestore(stackorig);
	if(res == -1) return null;
	return res;
};

/** @type {AsarInterface['getdefine']} */
Module['getdefine'] = (name) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(name);
	let res = _asar_getdefine(sptr);
	let res_str = UTF8ToString(res);
	stackRestore(stackorig);
	return res_str;
}

/** @type {AsarInterface['resolvedefines']} */
Module['resolvedefines'] = (data) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(data);
	let res = _asar_resolvedefines(sptr);
	let res_str = UTF8ToString(res);
	stackRestore(stackorig);
	return res_str;
};

/** @type {AsarInterface['getalldefines']} */
Module['getalldefines'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let defs_ptr = _asar_getalldefines(count_ptr);
	let count = read32(count_ptr);
	let sz = _definedata_size();
	let off_name = _definedata_off_name();
	let off_contents = _definedata_off_contents();
	/** @type {{[x: string]: string}} */
	let res = {};
	for(let i = 0; i < count; i++) {
		let p = defs_ptr + i*sz;
		let name = UTF8ToString(read32(p+off_name));
		let contents = UTF8ToString(read32(p+off_contents));
		res[name] = contents;
	}
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['math']} */
Module['math'] = (math_str) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(math_str);
	let err_ptr = stackAlloc(4);
	let res_num = _asar_math(sptr, err_ptr);
	/** @type {[number, null] | [null, string]} */
	let res;
	if(read32(err_ptr) == 0) res = [res_num, null];
	else res = [null, UTF8ToString(read32(err_ptr))];
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getwrittenblocks']} */
Module['getwrittenblocks'] = () => {
	let stackorig = stackSave();
	let count_ptr = stackAlloc(4);
	let blocks_ptr = _asar_getwrittenblocks(count_ptr);
	let count = read32(count_ptr);
	let sz = _writtenblockdata_size();
	let off_pcoffset = _writtenblockdata_off_pcoffset();
	let off_snesoffset = _writtenblockdata_off_snesoffset();
	let off_numbytes = _writtenblockdata_off_numbytes();
	let res = [];
	for(let i = 0; i < count; i++) {
		let p = blocks_ptr + sz*i;
		res.push({
			'pcoffset': read32(p+off_pcoffset),
			'snesoffset': read32(p+off_snesoffset),
			'numbytes': read32(p+off_numbytes),
		});
	}
	stackRestore(stackorig);
	return res;
};

/** @type {AsarInterface['getmapper']} */
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

/** @type {AsarInterface['getsymbolsfile']} */
Module['getsymbolsfile'] = (type) => {
	let stackorig = stackSave();
	let sptr = stringToUTF8OnStack(type);
	let res = _asar_getsymbolsfile(sptr);
	let res_str = UTF8ToString(res);
	stackRestore(stackorig);
	return res_str;
};
