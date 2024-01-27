#include "asar.h"
#include "crc32.h"
#include "virtualfile.h"
#include "platform/file-helpers.h"
#include "interface-shared.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "platform/thread-helpers.h"

#if defined(CPPCLI)
#define EXPORT extern "C"
#elif defined(_WIN32)
#ifdef ASAR_SHARED
#define EXPORT extern "C" __declspec(dllexport)
#elif defined(ASAR_STATIC)
#define EXPORT extern "C"
#endif
#else
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

static autoarray<const char *> prints;
static string symbolsfile;
static int numprint;
static uint32_t romCrc;

#define APIVERSION 400

// note: somewhat fragile, assumes that every patchparams struct inherits from exactly the previous one
/* $EXPORTSTRUCT_PP$
 */
struct patchparams_base {
	// The size of this struct. Set to (int)sizeof(patchparams).
	int structsize;
};


/* $EXPORTSTRUCT$
 */
struct stackentry {
	const char * fullpath;
	const char * prettypath;
	int lineno;
	const char * details;
};

/* $EXPORTSTRUCT$
 */
struct errordata {
	const char * fullerrdata;
	const char * rawerrdata;
	const char * block;
	const char * filename;
	int line;
	const struct stackentry * callstack;
	int callstacksize;
	const char * errname;
};
static  autoarray<errordata> errors;
static int numerror;

static autoarray<errordata> warnings;
static int numwarn;

/* $EXPORTSTRUCT$
 */
struct labeldata {
	const char * name;
	int location;
};

/* $EXPORTSTRUCT$
 */
struct definedata {
	const char * name;
	const char * contents;
};

/* $EXPORTSTRUCT$
 */
struct warnsetting {
	const char * warnid;
	bool enabled;
};

/* $EXPORTSTRUCT$
 */
struct memoryfile {
	const char* path;
	const void* buffer;
	size_t length;
};

void print(const char * str)
{
	prints[numprint++]= duplicate_string(str);
}

static void fillerror(errordata& myerr, const char* errname, const char * type, const char * str, bool show_block)
{
	const char* current_filename = get_current_file_name();
	if(current_filename) myerr.filename= duplicate_string(current_filename);
	else myerr.filename = duplicate_string("");
	myerr.line=get_current_line();
	const char* current_block = get_current_block();
	if (current_block) myerr.block= duplicate_string(current_block);
	else myerr.block= duplicate_string("");
	myerr.rawerrdata= duplicate_string(str);
	string location;
	string details;
	get_current_line_details(&location, &details);
	myerr.fullerrdata= duplicate_string(location+": "+type+str+details+get_callstack());
	myerr.errname = duplicate_string(errname);

	autoarray<printable_callstack_entry> printable_stack;
	get_full_printable_callstack(&printable_stack, 0, false);

	myerr.callstacksize = printable_stack.count;
	myerr.callstack = static_cast<stackentry*>(malloc(sizeof(stackentry) * myerr.callstacksize));

	for (int i = 0; i < myerr.callstacksize; ++i)
	{
		stackentry& entry = const_cast<stackentry&>(myerr.callstack[i]);

		entry.fullpath = duplicate_string(printable_stack[i].fullpath);
		entry.prettypath = duplicate_string(printable_stack[i].prettypath);
		entry.lineno = printable_stack[i].lineno;
		entry.details = duplicate_string(printable_stack[i].details);
	}
}

static bool ismath=false;
static string matherror;

void error_interface(int errid, int whichpass, const char * e_)
{
	errored = true;
	if (ismath) matherror = e_;
	else if (pass == whichpass) {
		// don't show current block if the error came from an error command
		bool show_block = (errid != error_id_error_command);
		fillerror(errors[numerror++], get_error_name((asar_error_id)errid), STR "error: (" + get_error_name((asar_error_id)errid) + "): ", e_, show_block);
	}
	else {}//ignore anything else
}

void warn(int errid, const char * str)
{
	// don't show current block if the warning came from a warn command
	bool show_block = (errid != warning_id_warn_command);
	fillerror(warnings[numwarn++], get_warning_name((asar_warning_id)errid), STR "warning: (" + get_warning_name((asar_warning_id)errid) + "): ", str, show_block);
}

static autoarray<labeldata> ldata;
static int labelsinldata = 0;
static autoarray<definedata> ddata;
static int definesinddata=0;

static void resetdllstuff()
{
#define free_and_null(x) free((void*)x); x = nullptr
	for (int i=0;i<numprint;i++)
	{
		free_and_null(prints[i]);
	}
	prints.reset();
	numprint=0;

	for (int i=0;i<numerror;i++)
	{
		free_and_null(errors[i].filename);
		free_and_null(errors[i].rawerrdata);
		free_and_null(errors[i].fullerrdata);
		free_and_null(errors[i].block);
		free_and_null(errors[i].errname);

		for (int j=0;j<errors[i].callstacksize;++j)
		{
			stackentry& entry = const_cast<stackentry&>(errors[i].callstack[j]);
			free_and_null(entry.fullpath);
			free_and_null(entry.prettypath);
			free_and_null(entry.details);
		}
		free_and_null(errors[i].callstack);
	}
	errors.reset();
	numerror=0;

	for (int i=0;i<numwarn;i++)
	{
		free_and_null(warnings[i].filename);
		free_and_null(warnings[i].rawerrdata);
		free_and_null(warnings[i].fullerrdata);
		free_and_null(warnings[i].block);
		free_and_null(warnings[i].errname);

		for (int j=0;j<warnings[i].callstacksize;++j)
		{
			stackentry& entry = const_cast<stackentry&>(warnings[i].callstack[j]);
			free_and_null(entry.fullpath);
			free_and_null(entry.prettypath);
			free_and_null(entry.details);
		}
		free_and_null(warnings[i].callstack);
	}
	warnings.reset();
	numwarn=0;
	
	for (int i=0;i<definesinddata;i++)
	{
		free_and_null(ddata[i].name);
		free_and_null(ddata[i].contents);
	}
	ddata.reset();
	definesinddata=0;

	for (int i=0;i<labelsinldata;i++)
		free((void*)ldata[i].name);
	ldata.reset();
	labelsinldata=0;
#undef free_and_null

	romCrc = 0;
	clidefines.reset();
	reset_warnings_to_default();

	reseteverything();
}

#define maxromsize (16*1024*1024)

static bool expectsNewAPI = false;

static void addlabel(const string & name, const snes_label & label_data)
{
	labeldata label;
	label.name = strdup(name);
	label.location = (int)(label_data.pos & 0xFFFFFF);
	ldata[labelsinldata++] = label;
}

/* $EXPORTSTRUCT_PP$
 */
struct patchparams_v200 : public patchparams_base
{
	// Same parameters as asar_patch()
	const char * patchloc;
	char * romdata;
	int buflen;
	int * romlen;

	// Include paths to use when searching files.
	const char** includepaths;
	int numincludepaths;

	// A list of additional defines to make available to the patch.
	const struct definedata* additional_defines;
	int additional_define_count;

	// Path to a text file to parse standard include search paths from.
	// Set to NULL to not use any standard includes search paths.
	const char* stdincludesfile;

	// Path to a text file to parse standard defines from.
	// Set to NULL to not use any standard defines.
	const char* stddefinesfile;

	// A list of warnings to enable or disable.
	// Specify warnings in the format "WXXXX" where XXXX = warning ID.
	const struct warnsetting * warning_settings;
	int warning_setting_count;

	// List of memory files to create on the virtual filesystem.
	const struct memoryfile * memory_files;
	int memory_file_count;

	// Set override_checksum_gen to true and generate_checksum to true/false
	// to force generating/not generating a checksum.
	bool override_checksum_gen;
	bool generate_checksum;

	// Set this to true for generated error and warning texts to always
	// contain their full call stack.
	bool full_call_stack;
};

/* $EXPORTSTRUCT_PP$
 */
struct patchparams : public patchparams_v200
{

};

static void asar_patch_begin(char * romdata_, int buflen, int * romlen_)
{
	if (buflen != maxromsize)
	{
		romdata_r = (unsigned char*)malloc(maxromsize);
		memcpy(const_cast<unsigned char*>(romdata_r)/*we just allocated this, it's safe to violate its const*/, romdata_, (size_t)*romlen_);
	}
	else romdata_r = (unsigned char*)romdata_;
	romdata = (unsigned char*)malloc(maxromsize);
	// RPG Hacker: Without this memset, freespace commands can (and probably will) fail.
	memset((void*)romdata, 0, maxromsize);
	memcpy(const_cast<unsigned char*>(romdata), romdata_, (size_t)*romlen_);
	resetdllstuff();
	romlen = *romlen_;
	romlen_r = *romlen_;
}

static void asar_patch_main(const char * patchloc)
{
	if (!path_is_absolute(patchloc)) asar_throw_warning(pass, warning_id_relative_path_used, "patch file");

	try
	{
		for (pass = 0;pass < 3;pass++)
		{
			initstuff();
			assemblefile(patchloc);
			// RPG Hacker: Necessary, because finishpass() can throws warning and errors.
			callstack_push cs_push(callstack_entry_type::FILE, filesystem->create_absolute_path(nullptr, patchloc));
			finishpass();
		}
	}
	catch (errfatal&) {}
}

static bool asar_patch_end(char * romdata_, int buflen, int * romlen_)
{
	if (checksum_fix_enabled) fixchecksum();
	if (romdata_ != (const char*)romdata_r) free(const_cast<unsigned char*>(romdata_r));
	if (buflen < romlen) asar_throw_error(pass, error_type_null, error_id_buffer_too_small);
	if (errored)
	{
		if (numerror==0)
			asar_throw_error(pass, error_type_null, error_id_phantom_error);
		free(const_cast<unsigned char*>(romdata));
		return false;
	}
	if (*romlen_ != buflen)
	{
		*romlen_ = romlen;
	}
	romCrc = crc32((const uint8_t*)romdata, (size_t)romlen);
	memcpy(romdata_, romdata, (size_t)romlen);
	free(const_cast<unsigned char*>(romdata));
	return true;
}

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

// this and asar_close are hardcoded in each api
EXPORT bool asar_init()
{
	if (!expectsNewAPI) return false;
	return true;
}

/* $EXPORT$
 * Returns the version, in the format major*10000+minor*100+bugfix*1. This
 * means that 1.2.34 would be returned as 10234.
 */
EXPORT int asar_version()
{
	return get_version_int();
}

/* $EXPORT$
 * Returns the API version, format major*100+minor. Minor is incremented on
 * backwards compatible changes; major is incremented on incompatible changes.
 * Does not have any correlation with the Asar version.
 *
 * It's not very useful directly, since asar_init() verifies this automatically.
 * Calling this one also sets a flag that makes asar_init not instantly return
 * false; this is so programs expecting an older API won't do anything unexpected.
 */
EXPORT int asar_apiversion()
{
	expectsNewAPI=true;
	return APIVERSION;
}

/* $EXPORT$
 * Clears out all errors, warnings and printed statements, and clears the file
 * cache. Not really useful, since asar_patch() already does this.
 */
EXPORT bool asar_reset()
{
	resetdllstuff();
	pass=0;
	return true;
}

EXPORT void asar_close()
{
	resetdllstuff();
}

/* $EXPORT$
 * Applies a patch. The first argument is a filename (so Asar knows where to
 * look for incsrc'd stuff); however, the ROM is in memory.
 * This function assumes there are no headers anywhere, neither in romdata nor
 * the sizes. romlen may be altered by this function; if this is undesirable,
 * set romlen equal to buflen.
 * The return value is whether any errors appeared (false=errors, call
 * asar_geterrors for details). If there is an error, romdata and romlen will
 * be left unchanged.
 * See the documentation of struct patchparams for more information.
 */
EXPORT bool asar_patch(const struct patchparams_base *params)
{
	auto execute_patch = [&]() {
		if (params == nullptr)
		{
			asar_throw_error(pass, error_type_null, error_id_params_null);
		}

		if (params->structsize != sizeof(patchparams_v200))
		{
			asar_throw_error(pass, error_type_null, error_id_params_invalid_size);
		}

		patchparams paramscurrent;
		memset(&paramscurrent, 0, sizeof(paramscurrent));
		memcpy(&paramscurrent, params, (size_t)params->structsize);


		asar_patch_begin(paramscurrent.romdata, paramscurrent.buflen, paramscurrent.romlen);

		simple_callstacks = !paramscurrent.full_call_stack;

		autoarray<string> includepaths;
		autoarray<const char*> includepath_cstrs;

		for (int i = 0; i < paramscurrent.numincludepaths; ++i)
		{
			if (!path_is_absolute(paramscurrent.includepaths[i])) asar_throw_warning(pass, warning_id_relative_path_used, "include search");
			string& newpath = includepaths.append(paramscurrent.includepaths[i]);
			includepath_cstrs.append((const char*)newpath);
		}

		if (paramscurrent.stdincludesfile != nullptr) {
			if (!path_is_absolute(paramscurrent.stdincludesfile)) asar_throw_warning(pass, warning_id_relative_path_used, "std includes file");
			string stdincludespath = paramscurrent.stdincludesfile;
			parse_std_includes(stdincludespath, includepaths);
		}

		for (int i = 0; i < includepaths.count; ++i)
		{
			includepath_cstrs.append((const char*)includepaths[i]);
		}

		size_t includepath_count = (size_t)includepath_cstrs.count;
		virtual_filesystem new_filesystem;
		new_filesystem.initialize(&includepath_cstrs[0], includepath_count);
		filesystem = &new_filesystem;

		for(int i = 0; i < paramscurrent.memory_file_count; ++i) {
			memoryfile f = paramscurrent.memory_files[i];
			filesystem->add_memory_file(f.path, f.buffer, f.length);
		}

		clidefines.reset();
		for (int i = 0; i < paramscurrent.additional_define_count; ++i)
		{
			string name = (paramscurrent.additional_defines[i].name != nullptr ? paramscurrent.additional_defines[i].name : "");
			strip_whitespace(name);
			name.strip_prefix('!'); // remove leading ! if present
			if (!validatedefinename(name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_invalid, "asar_patch_ex() additional defines", name.data());
			if (clidefines.exists(name)) {
				asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "asar_patch_ex() additional define", name.data());
				return false;
			}
			string contents = (paramscurrent.additional_defines[i].contents != nullptr ? paramscurrent.additional_defines[i].contents : "");
			clidefines.create(name) = contents;
		}

		if (paramscurrent.stddefinesfile != nullptr) {
			if (!path_is_absolute(paramscurrent.stddefinesfile)) asar_throw_warning(pass, warning_id_relative_path_used, "std defines file");
			string stddefinespath = paramscurrent.stddefinesfile;
			parse_std_defines(stddefinespath);
		} else {
			parse_std_defines(nullptr); // needed to populate builtin defines
		}

		for (int i = 0; i < paramscurrent.warning_setting_count; ++i)
		{
			asar_warning_id warnid = parse_warning_id_from_string(paramscurrent.warning_settings[i].warnid);

			if (warnid != warning_id_end)
			{
				set_warning_enabled(warnid, paramscurrent.warning_settings[i].enabled);
			}
			else
			{
				asar_throw_warning(pass, warning_id_invalid_warning_id, paramscurrent.warning_settings[i].warnid, "asar_patch_ex() warning_settings");
			}
		}

		if(paramscurrent.override_checksum_gen) {
			checksum_fix_enabled = paramscurrent.generate_checksum;
			force_checksum_fix = true;
		}

		asar_patch_main(paramscurrent.patchloc);

		// RPG Hacker: Required before the destroy() below,
		// otherwise it will leak memory.
		closecachedfiles();

		new_filesystem.destroy();
		filesystem = nullptr;

		return asar_patch_end(paramscurrent.romdata, paramscurrent.buflen, paramscurrent.romlen);
};
#if defined(RUN_VIA_FIBER)
	return run_as_fiber(execute_patch);
#elif defined(RUN_VIA_THREAD)
	return run_as_thread(execute_patch);
#else
	return execute_patch();
#endif
}

/* $EXPORT$
 * Returns the maximum possible size of the output ROM from asar_patch().
 * Giving this size to buflen guarantees you will not get any buffer too small
 * errors; however, it is safe to give smaller buffers if you don't expect any
 * ROMs larger than 4MB or something.
 */
EXPORT int asar_maxromsize()
{
	return maxromsize;
}

/* $EXPORT$
 * Get a list of all errors.
 * All pointers from these functions are valid only until the same function is
 * called again, or until asar_patch, asar_reset or asar_close is called,
 * whichever comes first. Copy the contents if you need it for a longer time.
 */
EXPORT const struct errordata * asar_geterrors(int * count)
{
	*count=numerror;
	return errors;
}

/* $EXPORT$
 * Get a list of all warnings.
 */
EXPORT const struct errordata * asar_getwarnings(int * count)
{
	*count=numwarn;
	return warnings;
}

/* $EXPORT$
 * Get a list of all printed data.
 */
EXPORT const char * const * asar_getprints(int * count)
{
	*count=numprint;
	return prints;
}

/* $EXPORT$
 * Get a list of all labels.
 */
EXPORT const struct labeldata * asar_getalllabels(int * count)
{
	for (int i=0;i<labelsinldata;i++) free((void*)ldata[i].name);
	labelsinldata=0;
	labels.each(addlabel);
	*count=labelsinldata;
	return ldata;
}

/* $EXPORT$
 * Get the ROM location of one label. -1 means "not found".
 */
EXPORT int asar_getlabelval(const char * name)
{
	int i;
	try {
		i=(int)labelval(&name).pos;
	}
	catch(errfatal&) { return -1; }
	if (*name || i<0) return -1;
	else return i&0xFFFFFF;
}

/* $EXPORT$
 * Get the value of a define.
 */
EXPORT const char * asar_getdefine(const char * name)
{
	if (!defines.exists(name)) return "";
	return defines.find(name);
}

/* $EXPORT$
 * Parses all defines in the parameter. Note that it may emit errors.
 */
EXPORT const char * asar_resolvedefines(const char * data)
{
	static string out;
	try
	{
		resolvedefines(out, data);
	}
	catch(errfatal&){}
	return out;
}

static void adddef(const string& name, string& value)
{
	definedata define;
	define.name= duplicate_string(name);
	define.contents= duplicate_string(value);
	ddata[definesinddata++]=define;
}

/* $EXPORT$
 * Gets the values and names of all defines.
 */
EXPORT const struct definedata * asar_getalldefines(int * count)
{
	for (int i=0;i<definesinddata;i++)
	{
		free((void*)ddata[i].name);
		free((void*)ddata[i].contents);
	}
	definesinddata=0;
	defines.each(adddef);
	*count=definesinddata;
	return ddata;
}

/* $EXPORT$
 * Parses a string containing math. It automatically assumes global scope (no
 * namespaces), and has access to all functions and labels from the last call
 * to asar_patch. Remember to check error to see if it's successful (NULL) or
 * if it failed (non-NULL, contains a descriptive string). It does not affect
 * asar_geterrors.
 */
EXPORT double asar_math(const char * math_, const char ** error)
{
	ns="";
	namespace_list.reset();
	sublabels.reset();
	errored=false;
	ismath=true;
	initmathcore();
	double rval=0;
	try
	{
		rval=(double)math(math_);
	}
	catch(errfatal&)
	{
		*error=matherror;
	}
	ismath=false;
	deinitmathcore();
	return rval;
}

/* $EXPORT$
 * Get a list of all the blocks written to the ROM by calls such as
 * asar_patch().
 */
EXPORT const struct writtenblockdata * asar_getwrittenblocks(int * count)
{
	*count = writtenblocks.count;
	return writtenblocks;
}

/* $EXPORT$
 * Get the mapper currently used by Asar.
 */
EXPORT enum mapper_t asar_getmapper()
{
	return mapper;
}

/* $EXPORT$
 * Generates the contents of a symbols file for in a specific format.
 */
EXPORT const char * asar_getsymbolsfile(const char* type){
	symbolsfile = create_symbols_file(type, romCrc);
	return symbolsfile;
}

#if defined(__clang__)
#	pragma clang diagnostic pop
#endif
