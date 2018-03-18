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
extern unsigned const char * romdata_r;
extern int romlen_r;

struct errfatal {};
struct errline : public errfatal {};
struct errblock : public errline {};
struct errnull : public errblock {};
template<typename t> void error(int neededpass, const char * str);
#define berror(e) error<errblock>(e)

#define clean(string) do { string.qreplace("\t", " ", true); string.qreplace(", ", ",", true); string.qreplace("  ", " ", true); \
											itrim(string.str, " ", " ", true); } while(0)

//void write1(unsigned int num);
//void write2(unsigned int num);
//void write3(unsigned int num);
//void write4(unsigned int num);

struct snes_struct {
	string parent;
	int base_end;
	int struct_size;
	int object_size;
};

// RPG Hacker: Really the only purpose of this struct is to support pushtable and pulltable
// Also don't know where else to put this, so putting it in this header
struct chartabledata {
	unsigned int table[256];
};

extern int pass;
extern bool foundlabel;
extern bool ignoretitleerrors;

//extern bool emulate;

enum { arch_65816, arch_spc700, arch_spc700_inline, arch_superfx };
extern int arch;

template<typename t> void error(int neededpass, const char * e_);
inline void fatalerror(const char * e_)
{
	error<errfatal>(pass, e_);
}
inline void nullerror(const char * e_)
{
	error<errnull>(pass, e_);
}
void warn(const char * e);
#define warn0(e) do { if (pass==0) warn(e); } while(0)
#define warn1(e) do { if (pass==1) warn(e); } while(0)
#define warn2(e) do { if (pass==2) warn(e); } while(0)

void write1_pick(unsigned int num);

void write2(unsigned int num);
void write3(unsigned int num);
void write4(unsigned int num);

int read1(int snespos);
int read2(int snespos);
int read3(int snespos);

bool assemblemapper(char** word, int numwords);

extern int snespos;
extern int realsnespos;
extern int startpos;
extern int realstartpos;

extern int bytes;

int getlen(const char * str, bool optimizebankextraction=false);
int getnum(const char * str);
double getnumdouble(const char * str);

int getlenfromchar(char c);

unsigned int labelval(const char ** rawname, bool define=false);
unsigned int labelval(char ** rawname, bool define=false);
unsigned int labelval(string name, bool define=false);
bool labelval(const char ** rawname, unsigned int * rval, bool define=false);
bool labelval(char ** rawname, unsigned int * rval, bool define=false);
bool labelval(string name, unsigned int * rval, bool define=false);

extern volatile int recursioncount;
class recurseblock {
public:
	recurseblock()
	{
		recursioncount++;
		if (recursioncount>2500) fatalerror("Recursion limit reached.");
	}
	~recurseblock()
	{
		recursioncount--;
	}
};

struct whiletracker {
	bool iswhile;
	int startline;
	bool cond;
};

extern const int asarver_maj;
extern const int asarver_min;
extern const int asarver_bug;
extern const bool asarver_beta;
