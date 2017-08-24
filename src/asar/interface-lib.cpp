#include "asar.h"
#include "scapegoat.hpp"
#include "libstr.h"
//#include "libcon.h"
#include "libsmw.h"
//#include <stdio.h>

#if defined(CPPCLI)
#define EXPORT extern "C"
#elif defined(_WIN32)
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

extern bool errored;

//extern bool checksum;

string dir(char const *name);

string getdecor();

void assemblefile(const char * filename, bool toplevel);

extern const char * thisfilename;
extern int thisline;
extern const char * thisblock;
extern const char * callerfilename;
extern int callerline;

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
extern lightweight_map<string, unsigned int> labels;

struct definedata {
	const char * name;
	const char * contents;
};
extern lightweight_map<string, string> defines;

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
	else myerr.block="";
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
#define free(x) free((void*)x)
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
		free(errors[i].block);
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
	return 300;
}

EXPORT bool asar_reset()
{
	resetdllstuff();
	pass=0;
	return true;
}

void deinitmathcore();

EXPORT void asar_close()
{
	resetdllstuff();
	deinitmathcore();
}

#define maxromsize (16*1024*1024)
EXPORT bool asar_patch(const char * patchloc, char * romdata_, int buflen, int * romlen_)
{
	if (buflen!=maxromsize)
	{
		romdata_r=(unsigned char*)malloc(maxromsize);
		memcpy((char*)romdata_r/*we just allocated this, it's safe to violate its const*/, romdata_, *romlen_);
	}
	else romdata_r=(unsigned char*)romdata_;
	romdata=(unsigned char*)malloc(maxromsize);
	memcpy((unsigned char*)romdata, romdata_, *romlen_);
	resetdllstuff();
	romlen=*romlen_;
	romlen_r=*romlen_;
	try
	{
		for (pass=0;pass<3;pass++)
		{
			initstuff();
			assemblefile(patchloc, true);
			finishpass();
		}
	}
	catch(errfatal&){}
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
	memcpy(romdata_, romdata, romlen);
	free((unsigned char*)romdata);
	return true;
}

EXPORT int asar_maxromsize()
{
	return maxromsize;
}

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
	labels.traverse(addlabel);
	*count=labelsinldata;
	return ldata;
}

extern int numopcodes;
EXPORT int asar_getlabelval(const char * name)
{
	if (!stricmp(name, ":$:opcodes:$:")) return numopcodes;//aaah, you found me
	int i=labelval(&name);
	if (*name || i<0) return -1;
	else return i&0xFFFFFF;
}

EXPORT const char * asar_getdefine(const char * name)
{
	static string out;
	defines.find(name, out);
	return out;
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
	defines.traverse(adddef);
	*count=definesinddata;
	return ddata;
}

long double math(const char * mystr, const char ** e);
extern autoarray<string> sublabels;
extern string ns;

EXPORT double asar_math(const char * str, const char ** e)//degrading to normal double because long double seems volatile
{
	ns="";
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


