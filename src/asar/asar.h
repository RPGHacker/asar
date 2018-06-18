#if (defined(__sun__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)) && !defined(linux)
#error Please use -Dlinux on non-Linux Unix-likes.
#endif

#if defined(linux) && !defined(stricmp)
#error Please use -Dstricmp=strcasecmp on Unix-like systems.
#endif

#pragma once
#define Asar

#include "autoarray.h"
#include "assocarr.h"
#include "libstr.h"
#include "libsmw.h"
#include "errors.h"
#include "virtualfile.h"

extern unsigned const char * romdata_r;
extern int romlen_r;

#define clean(string) do { string.qreplace("\t", " ", true); string.qreplace(", ", ",", true); string.qreplace("  ", " ", true); \
											itrim(string.str, " ", " ", true); } while(0)

int getlen(const char * str, bool optimizebankextraction=false);
bool is_hex_constant(const char * str);

bool validatedefinename(const char * name);

string create_symbols_file(string format);

void parse_std_includes(const char* textfile, autoarray<string>& outarray);
void parse_std_defines(const char* textfile);

void reseteverything();

void resolvedefines(string& out, const char * start);

int get_version_int();

bool setmapper();

void assemblefile(const char * filename, bool toplevel);
void assembleline(const char * fname, int linenum, const char * line);

bool file_included_once(const char* file);

string getdecor();

extern volatile int recursioncount;
extern int pass;

class recurseblock {
public:
	recurseblock()
	{
		recursioncount++;
		if (recursioncount > 2500) asar_throw_error(pass, error_type_fatal, error_id_recursion_limit);
	}
	~recurseblock()
	{
		recursioncount--;
	}
};

extern const int asarver_maj;
extern const int asarver_min;
extern const int asarver_bug;
extern const bool asarver_beta;

extern bool asarverallowed;
extern bool istoplevel;

extern bool moreonline;

extern bool checksum_fix_enabled;
extern bool force_checksum_fix;

extern const char * callerfilename;
extern int callerline;
extern string thisfilename;
extern int thisline;
extern int lastspecialline;
extern const char * thisblock;

extern int incsrcdepth;

extern bool ignoretitleerrors;

extern int repeatnext;

extern int optimizeforbank;

extern bool errored;

extern assocarr<string> clidefines;

extern virtual_filesystem* filesystem;

extern assocarr<string> defines;

extern assocarr<string> builtindefines;
