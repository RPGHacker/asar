#pragma once

enum { arch_65816, arch_spc700, arch_spc700_inline, arch_superfx };
extern int arch;

bool assemblemapper(char** word, int numwords);

struct snes_struct {
	string parent;
	int base_end;
	int struct_size;
	int object_size;
};

extern assocarr<snes_struct> structs;


// RPG Hacker: Really the only purpose of this struct is to support pushtable and pulltable
// Also don't know where else to put this, so putting it in this header
/*struct chartabledata {
	unsigned int table[256];
};

extern chartabledata table;
unsigned int get_table_val(int inp);
void set_table_val(int inp, unsigned int out);*/

struct whiletracker {
	bool iswhile;
	int startline;
	bool cond;
};

extern autoarray<whiletracker> whilestatus;

bool confirmname(const char * name);
string posneglabelname(const char ** input, bool define);

void write1_pick(unsigned int num);
void write2(unsigned int num);
void write3(unsigned int num);
void write4(unsigned int num);

int read1(int snespos);
int read2(int snespos);
int read3(int snespos);

int snestopc_pick(int addr);

int getlenfromchar(char c);

unsigned int labelval(const char ** rawname, bool define = false);
unsigned int labelval(char ** rawname, bool define = false);
unsigned int labelval(string name, bool define = false);
bool labelval(const char ** rawname, unsigned int * rval, bool define = false);
bool labelval(char ** rawname, unsigned int * rval, bool define = false);
bool labelval(string name, unsigned int * rval, bool define = false);

const char * safedequote(char * str);

void checkbankcross();

void initstuff();
void finishpass();

void assembleblock(const char * block, bool isspecialline);

extern int snespos;
extern int realsnespos;
extern int startpos;
extern int realstartpos;

extern int bytes;

extern int numopcodes;

extern bool warnxkas;

extern int numif;
extern int numtrue;

extern bool emulatexkas;

extern int freespaceextra;

extern assocarr<unsigned int> labels;

extern autoarray<int>* macroposlabels;
extern autoarray<int>* macroneglabels;
extern autoarray<string>* macrosublabels;

extern autoarray<string> sublabels;
extern string ns;
extern autoarray<string> namespace_list;

extern autoarray<string> includeonce;
