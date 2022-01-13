#if (defined(__sun__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)) && !defined(linux)
#error Please use -Dlinux on non-Linux Unix-likes.
#endif

#if defined(linux) && !defined(stricmp)
#error Please use -Dstricmp=strcasecmp on Unix-like systems.
#endif

#pragma once
#define Asar

#include "assocarr.h"
#include "libstr.h"
#include "libsmw.h"
#include "errors.h"
#include "warnings.h"
#include "virtualfile.h"
#include <cstdint>

extern unsigned const char * romdata_r;
extern int romlen_r;

inline void verify_paren(autoptr<char **> &ptr)
{
	 if(!ptr) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
}

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

void assemblefile(const char * filename, bool toplevel);
void assembleline(const char * fname, int linenum, const char * line);

bool file_included_once(const char* file);

string getdecor();

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

extern bool asarverallowed;
extern bool parsing_macro;
extern bool istoplevel;

extern bool moreonline;

extern bool checksum_fix_enabled;
extern bool force_checksum_fix;

enum ir_command
{
	COMMAND_ASSERT,
	COMMAND_IF,
	COMMAND_WHILE,
	COMMAND_ELSEIF,
	COMMAND_ENDWHILE,
	COMMAND_ENDIF,
	COMMAND_ELSE,
	COMMAND_OPCODE,
	COMMAND_DB,
	COMMAND_DW,
	COMMAND_DL,
	COMMAND_DD,
	COMMAND_MACRO,
	COMMAND_CALLMACRO,
	INTERNAL_COMMAND_NUMVARARGS,
	INTERNAL_COMMAND_ENDMACRO,
	COMMAND_ENDMACRO,
	COMMAND_UNDEF,
	COMMAND_ERROR,
	COMMAND_WARN,
	COMMAND_WARNINGS,
	COMMAND_GLOBAL,
	COMMAND_CHECK,
	COMMAND_ASAR,
	COMMAND_INCLUDE,
	COMMAND_INCLUDEFROM,
	COMMAND_INCLUDEONCE,
	COMMAND_SETLABEL,
	COMMAND_ORG,
	COMMAND_STRUCT,
	COMMAND_ENDSTRUCT,
	COMMAND_SPCBLOCK,
	COMMAND_ENDSPCBLOCK,
	COMMAND_STARTPOS,
	COMMAND_BASE,
	COMMAND_DPBASE,
	COMMAND_OPTIMIZE,
	COMMAND_BANK,
	COMMAND_FREESPACE,
	COMMAND_FREECODE,
	COMMAND_FREEDATA,
	COMMAND_PROT,
	COMMAND_AUTOCLEAN,
	COMMAND_PUSHPC,
	COMMAND_PULLPC,
	COMMAND_PUSHBASE,
	COMMAND_PULLBASE,
	COMMAND_PUSHNS,
	COMMAND_PULLNS,
	COMMAND_NAMESPACE,
	COMMAND_WARNPC,
	COMMAND_TABLE,
	INTERNAL_COMMAND_FILENAME,
	COMMAND_INCSRC,
	COMMAND_INCBIN,
	COMMAND_FILL,
	COMMAND_SKIP,
	COMMAND_CLEARTABLE,
	COMMAND_PUSHTABLE,
	COMMAND_PULLTABLE,
	COMMAND_FUNCTION,
	COMMAND_PRINT,
	COMMAND_RESET,
	COMMAND_PADBYTE,
	COMMAND_PADWORD,
	COMMAND_PADLONG,
	COMMAND_PADDWORD,
	COMMAND_PAD,
	COMMAND_FILLBYTE,
	COMMAND_FILLWORD,
	COMMAND_FILLLONG,
	COMMAND_FILLDWORD,
	COMMAND_ARCH,
	COMMAND_OPEN_BRACE,
	COMMAND_CLOSE_BRACE,
	COMMAND_LOROM,
	COMMAND_HIROM,
	COMMAND_EXLOROM,
	COMMAND_EXHIROM,
	COMMAND_SFXROM,
	COMMAND_NOROM,
	COMMAND_FULLSA1ROM,
	COMMAND_SA1ROM,
	COMMAND_EOL
};

struct ir_block
{
	autoptr <char *>block;
	autoptr <char **>word;
	int numwords;
	ir_command command;
};

extern string callerfilename;
extern int callerline;
extern string thisfilename;
extern int thisline;
extern int blockid;
extern autoarray<ir_block> block_ir;
extern const char * thisblock;

extern int incsrcdepth;

extern bool ignoretitleerrors;

extern int optimizeforbank;

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
