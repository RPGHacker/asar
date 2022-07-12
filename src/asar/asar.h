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
#include <cstdint>

extern unsigned const char * romdata_r;
extern int romlen_r;

#define clean(string) do { string.qreplace(", ", ",", true); string.qreplace("  ", " ", true); \
						strip_whitespace(string); string.qreplace("\t", " ", true);} while(0)
#define clean_and_trim(string) do { clean(string); string.qreplace(" ", "", true);} while(0)

int getlen(const char * str, bool optimizebankextraction=false);
bool is_hex_constant(const char * str);

bool validatedefinename(const char * name);

string create_symbols_file(string format, uint32_t romCrc);

void parse_std_includes(const char* textfile, autoarray<string>& outarray);
void parse_std_defines(const char* textfile);

void reseteverything();

void resolvedefines(string& out, const char * start);

int get_version_int();

bool setmapper();

void assemblefile(const char * filename);
void assembleline(const char * fname, int linenum, const char * line);

void do_line_logic(const char* line, const char* filename, int lineno);

bool file_included_once(const char* file);

void get_current_line_details(string* location, string* details, bool exclude_block=false);
string getcallstack();

asar_error_id vfile_error_to_error_id(virtual_file_error vfile_error);

virtual_file_error asar_get_last_io_error();

extern volatile int recursioncount;
extern int pass;

class recurseblock {
public:
	recurseblock()
	{
		recursioncount++;
		if (recursioncount > 250) asar_throw_error(pass, error_type_fatal, error_id_recursion_limit);
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
extern bool default_math_pri;
extern bool default_math_round_off;

extern bool asarverallowed;

extern bool moreonline;
extern bool moreonlinecond;
extern int fakeendif;

extern bool checksum_fix_enabled;
extern bool force_checksum_fix;

extern int incsrcdepth;

extern bool ignoretitleerrors;

extern int repeatnext;

extern int optimizeforbank;

extern int in_macro_def;

//this is a trick to namespace an enum to avoid name collision without too much verbosity
//could technically name the enum too but this is fine for now.
namespace optimize_dp_flag {
	enum : int {
		NONE,	//don't optimize
		RAM,	//bank 7E only (always uses dp base)
		ALWAYS	//bank 00-3F[|80] and 7E (always uses dp base)
	};
}

extern int optimize_dp;
extern int dp_base;

namespace optimize_address_flag {
	enum : int {
		DEFAULT,//simply use optimizeforbank
		RAM,	//default+bank 7E only RAM address < $2000
		MIRRORS	//ram+if optimizeforbank is 00-3F[|80] and address < $8000
	};
}

extern int optimize_address;

extern bool errored;

extern assocarr<string> clidefines;

extern virtual_filesystem* filesystem;

extern assocarr<string> defines;

extern assocarr<string> builtindefines;


namespace callstack_entry_type {
	enum e : int {
		FILE,
		MACRO_CALL,
		LINE,
		BLOCK,
	};
}

struct callstack_entry {
	callstack_entry_type::e type;
	string content;
	int lineno;
	
	callstack_entry(callstack_entry_type::e type, const char* content, int lineno)
	{
		this->type = type;
		this->content = content;
		this->lineno = lineno;
	}
	
	callstack_entry()
	{
	}
};


extern autoarray<callstack_entry> callstack;

class callstack_push {
public:
	callstack_push(callstack_entry_type::e type, const char* content, int lineno=-1)
	{
		callstack.append(callstack_entry(type, content, lineno));
	}
	
	~callstack_push()
	{
		callstack.remove(callstack.count-1);
	}
};

bool in_top_level_file();
const char* get_current_file_name();
int get_current_line();
const char* get_current_block();

// RPG Hacker: We only need to keep these two functions around
// until we bump the DLL API version number and update the
// interface to make use of the full callstack.
const char* get_previous_file_name();
int get_previous_file_line_no();
