#include <stddef.h>
#include <emscripten.h>
#include "../dll-bindings/asar.h"

#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE

// hardcoded assumptions in the js-side bindings
static_assert(sizeof(size_t) == 4);
static_assert(sizeof(int) == 4);
static_assert(sizeof(void*) == 4);

// doing this is easier than making proper handlers for the patchparams type...
EXPORT
bool do_patch(
	const char* patchloc,
	char* romdata,
	int buflen,
	int* romlen,
	const char** includepaths,
	int numincludepaths,
	const struct definedata* additional_defines,
	int additional_define_count,
	const char* stdincludesfile,
	const char* stddefinesfile,
	const struct warnsetting * warning_settings,
	int warning_setting_count,
	const struct memoryfile * memory_files,
	int memory_file_count,
	bool override_checksum_gen,
	bool generate_checksum,
	bool full_call_stack
) {
	struct patchparams pp;
	// wish there was some way to check i didn't forget any params...
	pp.structsize = sizeof(struct patchparams);
	pp.patchloc = patchloc;
	pp.romdata = romdata;
	pp.buflen = buflen;
	pp.romlen = romlen;
	pp.includepaths = includepaths;
	pp.numincludepaths = numincludepaths;
	pp.additional_defines = additional_defines;
	pp.additional_define_count = additional_define_count;
	pp.stdincludesfile = stdincludesfile;
	pp.stddefinesfile = stddefinesfile;
	pp.warning_settings = warning_settings;
	pp.warning_setting_count = warning_setting_count;
	pp.memory_files = memory_files;
	pp.memory_file_count = memory_file_count;
	pp.override_checksum_gen = override_checksum_gen;
	pp.generate_checksum = generate_checksum;
	pp.full_call_stack = full_call_stack;
	return asar_patch(&pp);
}

// Struct accessors:

#define STRUCTSIZE(t) EXPORT int t ## _size() { return sizeof(struct t); }
#define FLD(t, d) EXPORT int t ## _off_ ## d () { return offsetof(struct t, d); }
STRUCTSIZE(stackentry)
FLD(stackentry, fullpath)
FLD(stackentry, prettypath)
FLD(stackentry, lineno)
FLD(stackentry, details)

STRUCTSIZE(errordata)
FLD(errordata, fullerrdata)
FLD(errordata, rawerrdata)
FLD(errordata, block)
FLD(errordata, filename)
FLD(errordata, line)
FLD(errordata, callstack)
FLD(errordata, callstacksize)
FLD(errordata, errname)

STRUCTSIZE(labeldata)
FLD(labeldata, name)
FLD(labeldata, location)

STRUCTSIZE(definedata)
FLD(definedata, name)
FLD(definedata, contents)
EXPORT
void fill_definedata(definedata* obj, const char* name, const char* contents) {
	obj->name = name;
	obj->contents = contents;
}

STRUCTSIZE(warnsetting)
// don't need field accessors for this, we never read it, only write
EXPORT
void fill_warnsetting(warnsetting* obj, const char* warnid, bool enabled) {
	obj->warnid = warnid;
	obj->enabled = enabled;
}

STRUCTSIZE(memoryfile)
EXPORT
void fill_memoryfile(memoryfile* obj, const char* path, const void* buffer, size_t length) {
	obj->path = path;
	obj->buffer = buffer;
	obj->length = length;
}


STRUCTSIZE(writtenblockdata)
FLD(writtenblockdata, pcoffset)
FLD(writtenblockdata, snesoffset)
FLD(writtenblockdata, numbytes)
