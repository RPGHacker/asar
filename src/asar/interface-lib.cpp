#include "asar.h"
#include "assocarr.h"
#include "libstr.h"
#include "libsmw.h"
#include "virtualfile.hpp"

#if defined(CPPCLI)
#define EXPORT extern "C"
#elif defined(_WIN32)
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

extern bool errored;

extern bool checksum;

string getdecor();

void assemblefile(const char * filename, bool toplevel);

extern string thisfilename;
extern int thisline;
extern const char * thisblock;
extern const char * callerfilename;
extern int callerline;

extern virtual_filesystem* filesystem;

extern assocarr<string> clidefines;

autoarray<const char *> prints;
int numprint;

struct errordata {
	const char * fullerrdata;
	const char * rawerrdata;
	const char * block;
	const char * filename;
	int line;
	const char * callerfilename;
	int callerline;
};
autoarray<errordata> errors;
int numerror;

autoarray<errordata> warnings;
int numwarn;

struct labeldata {
	const char * name;
	int location;
};
extern assocarr<unsigned int> labels;

struct definedata {
	const char * name;
	const char * contents;
};
extern assocarr<string> defines;

extern autoarray<writtenblockdata> writtenblocks;

void print(const char * str)
{
	prints[numprint++]=strdup(str);
}

void fillerror(errordata& myerr, const char * type, const char * str)
{
	myerr.filename=strdup(thisfilename);
	myerr.line=thisline;
	if (thisblock) myerr.block=strdup(thisblock);
	else myerr.block=strdup("");
	myerr.rawerrdata=strdup(str);
	myerr.fullerrdata=strdup(S getdecor()+type+str+(thisblock?(S" ["+thisblock+"]"):""));
	myerr.callerline=callerline;
	myerr.callerfilename=callerfilename;
}

static bool ismath=false;
static string matherror;
template<typename t> void error(int neededpass, const char * str)
{
	try
	{
		errored=true;
		if (ismath) matherror=str;
		else if (pass==neededpass) fillerror(errors[numerror++], "error: ", str);
		else {}//ignore anything else
		t err;
		throw err;
	}
	catch(errnull&){}
}

void (*shutupgcc1)(int, const char*)=error<errnull>;
void (*shutupgcc2)(int, const char*)=error<errblock>;
void (*shutupgcc3)(int, const char*)=error<errline>;
void (*shutupgcc4)(int, const char*)=error<errfatal>;

void initmathcore();

void initstuff();
void finishpass();

void warn(const char * str)
{
	fillerror(warnings[numwarn++], "warning: ", str);
}

void reseteverything();

void resetdllstuff()
{
#define free(x) free((void*)x); x = NULL
	for (int i=0;i<numprint;i++)
	{
		free(prints[i]);
	}
	prints.reset();
	numprint=0;

	for (int i=0;i<numerror;i++)
	{
		free(errors[i].filename);
		free(errors[i].rawerrdata);
		free(errors[i].fullerrdata);
		free(errors[i].block);
	}
	errors.reset();
	numerror=0;

	for (int i=0;i<numwarn;i++)
	{
		free(warnings[i].filename);
		free(warnings[i].rawerrdata);
		free(warnings[i].fullerrdata);
		free(warnings[i].block);
	}
	warnings.reset();
	numwarn=0;
#undef free
	reseteverything();
}

static bool expectsNewAPI=false;
EXPORT bool asar_init()
{
	if (!expectsNewAPI) return false;
	initmathcore();
	return true;
}

EXPORT int asar_version()
{
	return asarver_maj*10000+asarver_min*100+asarver_bug;
}

EXPORT int asar_apiversion()
{
	expectsNewAPI=true;
	return 301;
}

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

#define maxromsize (16*1024*1024)
void asar_patch_begin(char * romdata_, int buflen, int * romlen_, bool should_reset)
{
	if (buflen != maxromsize)
	{
		romdata_r = (unsigned char*)malloc(maxromsize);
		memcpy((char*)romdata_r/*we just allocated this, it's safe to violate its const*/, romdata_, (size_t)*romlen_);
	}
	else romdata_r = (unsigned char*)romdata_;
	romdata = (unsigned char*)calloc(1, maxromsize);
	memcpy((unsigned char*)romdata, romdata_, (size_t)*romlen_);
	if (should_reset)
		resetdllstuff();
	romlen = *romlen_;
	romlen_r = *romlen_;
}

void asar_patch_main(const char * patchloc)
{
	try
	{
		for (pass = 0;pass < 3;pass++)
		{
			initstuff();
			assemblefile(patchloc, true);
			finishpass();
		}
	}
	catch (errfatal&) {}
}

bool asar_patch_end(char * romdata_, int buflen, int * romlen_)
{
	if (checksum) fixchecksum();
	if (romdata_!=(char*)romdata_r) free((char*)romdata_r);
	if (buflen<romlen) error<errnull>(pass, "The given buffer is too small to contain the resulting ROM.");
	if (errored)
	{
		if (buflen != maxromsize) free((unsigned char*)romdata);
		return false;
	}
	if (*romlen_ != buflen)
	{
		*romlen_ = romlen;
	}
	memcpy(romdata_, romdata, (size_t)romlen);
	free((unsigned char*)romdata);
	return true;
}

EXPORT bool asar_patch(const char * patchloc, char * romdata_, int buflen, int * romlen_)
{
	asar_patch_begin(romdata_, buflen, romlen_, true);

	virtual_filesystem new_filesystem;
	new_filesystem.initialize(nullptr, 0);
	filesystem = &new_filesystem;
	
	asar_patch_main(patchloc);

	new_filesystem.destroy();
	filesystem = nullptr;

	return asar_patch_end(romdata_, buflen, romlen_);
}

struct patchparams_base
{
	int structsize;
};

struct patchparams_v160 : public patchparams_base
{
	const char * patchloc;
	char * romdata;
	int buflen;
	int * romlen;

	const char** includepaths;
	int numincludepaths;

	bool should_reset;

	definedata* additional_defines;
	int definecount;

	const char* stdincludesfile;
	const char* stddefinesfile;
};

struct patchparams : public patchparams_v160
{

};

EXPORT bool asar_patch_ex(const patchparams_base* params)
{
	if (params == nullptr)
	{
		error<errnull>(pass, "params passed to asar_patch_ex() is null.");
	}

	if (params->structsize != sizeof(patchparams_v160))
	{
		error<errnull>(pass, "Size of params passed to asar_patch_ex() is invalid.");
	}

	patchparams paramscurrent;
	memset(&paramscurrent, 0, sizeof(paramscurrent));
	memcpy(&paramscurrent, params, (size_t)params->structsize);


	asar_patch_begin(paramscurrent.romdata, paramscurrent.buflen, paramscurrent.romlen, paramscurrent.should_reset);

	autoarray<string> includepaths;
	autoarray<const char*> includepath_cstrs;

	for (int i = 0; i < paramscurrent.numincludepaths; ++i)
	{
		string& newpath = includepaths.append(paramscurrent.includepaths[i]);
		includepath_cstrs.append((const char*)newpath);
	}

	if (paramscurrent.stdincludesfile != nullptr) {
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

	clidefines.reset();
	for (int i = 0; i < paramscurrent.definecount; i++)
	{
		string name = (paramscurrent.additional_defines[i].name != nullptr ? paramscurrent.additional_defines[i].name : "");
		name = name.replace("\t", " ", true);
		name = itrim(name.str, " ", " ", true);
		name = itrim(name.str, "!", "", false); // remove leading ! if present
		if (!validatedefinename(name)) error<errnull>(0, S "Invalid define name in asar_patch_ex() additional defines: '" + name + "'.");
		if (clidefines.exists(name)) {
			error<errnull>(pass, S "asar_patch_ex() additional define '" + name + "' overrides a previous define. Did you specify the same define twice?");
			return false;
		}
		string contents = (paramscurrent.additional_defines[i].contents != nullptr ? paramscurrent.additional_defines[i].contents : "");
		clidefines.create(name) = contents;
	}

	if (paramscurrent.stddefinesfile != nullptr) {
		string stddefinespath = paramscurrent.stddefinesfile;
		parse_std_defines(stddefinespath);
	}

	asar_patch_main(paramscurrent.patchloc);

	new_filesystem.destroy();
	filesystem = nullptr;

	return asar_patch_end(paramscurrent.romdata, paramscurrent.buflen, paramscurrent.romlen);
}

EXPORT int asar_maxromsize()
{
	return maxromsize;
}

// randomdude999: this is not exposed in any of the wrappers, why does it even exist?
extern chartabledata table;
EXPORT const unsigned int * asar_gettable()
{
	return table.table;
}

EXPORT const errordata * asar_geterrors(int * count)
{
	*count=numerror;
	return errors;
}

EXPORT const errordata * asar_getwarnings(int * count)
{
	*count=numwarn;
	return warnings;
}

EXPORT const char * const * asar_getprints(int * count)
{
	*count=numprint;
	return prints;
}

autoarray<labeldata> ldata;
int labelsinldata=0;
static void addlabel(const string & name, unsigned int & value)
{
	labeldata label;
	label.name=strdup(name);
	label.location=(int)(value&0xFFFFFF);
	ldata[labelsinldata++]=label;
}

EXPORT const labeldata * asar_getalllabels(int * count)
{
	for (int i=0;i<labelsinldata;i++) free((void*)ldata[i].name);
	labelsinldata=0;
	labels.each(addlabel);
	*count=labelsinldata;
	return ldata;
}

extern int numopcodes;
EXPORT int asar_getlabelval(const char * name)
{
	if (!stricmp(name, ":$:opcodes:$:")) return numopcodes;//aaah, you found me
	int i=(int)labelval(&name);
	if (*name || i<0) return -1;
	else return i&0xFFFFFF;
}

EXPORT const char * asar_getdefine(const char * name)
{
	if (!defines.exists(name)) return "";
	return defines.find(name);
}

void resolvedefines(string& out, const char * start);
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

autoarray<definedata> ddata;
int definesinddata=0;
static void adddef(const string& name, string& value)
{
	definedata define;
	define.name=strdup(name);
	define.contents=strdup(value);
	ddata[definesinddata++]=define;
}

EXPORT const definedata * asar_getalldefines(int * count)
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

double math(const char * mystr, const char ** e);
extern autoarray<string> sublabels;
extern string ns;
extern autoarray<string> namespace_list;

EXPORT double asar_math(const char * str, const char ** e)
{
	ns="";
	namespace_list.reset();
	sublabels.reset();
	errored=false;
	ismath=true;
	double rval=0;
	try
	{
		rval=(double)math(str, e);
	}
	catch(errfatal&)
	{
		*e=matherror;
	}
	ismath=false;
	return rval;
}

EXPORT const writtenblockdata * asar_getwrittenblocks(int * count)
{
	*count = writtenblocks.count;
	return writtenblocks;
}

EXPORT mapper_t asar_getmapper()
{
	return mapper;
}

EXPORT void asar_add_virtual_memory_file(const char* path, void* buffer, size_t length)
{
	if(filesystem) {
		filesystem->add_memory_file(path, buffer, length);
	}
}
