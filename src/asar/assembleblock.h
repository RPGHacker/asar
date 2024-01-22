#pragma once

enum { arch_65816, arch_spc700, arch_superfx };
extern int arch;

bool assemblemapper(char** word, int numwords);

struct snes_struct {
	string parent;
	int base_end;
	int struct_size;
	int object_size;
	bool is_static;
};

extern assocarr<snes_struct> structs;
extern int label_counter;


struct snes_label {
	unsigned int pos;
	int id;
	int freespace_id;
	bool is_static;

	snes_label()
	{
		pos = 0;
		is_static = false;
		freespace_id = 0;
		id = label_counter++;
	}
};

// data necessary for one freespace block
struct freespace_data {
	// snespos of the start of the freespace block. set to the found freespace
	// block during the `freespace` command in pass 1.
	int pos;
	// length of the freespace block
	int len;
	// whether this freespace is leaked (no autocleans pointing at it)
	bool leaked;
	// position of the previous version of this freespace
	int orgpos;
	// length of previous version
	int orglen;
	// whether this freespace is static, i.e. can't be relocated when reinserting
	bool is_static;
	// what byte to use when searching for freespace, and clearing out previous rats tags
	unsigned char cleanbyte;

	// options only used for finding freespace:

	// if this freespace is pinned to another one, this holds the name of the label of the target.
	// we can't resolve this into a freespace number earlier since it could be a forward reference.
	// we also need to keep the current namespace around since this is necessary for resolving label references.
	string pin_target;
	string pin_target_ns;
	// computed at the end of pass 0. this is the freespace id of the final pin
	// target, in case of multiple "nested" pins or whatnot.
	int pin_target_id;
	// what address to start searching for freespace at
	int search_start;
	// what bank to search for freespace in: -1 for any bank, -2 for banks with
	// code mirrors, positive for specific bank
	int bank;
	bool write_rats;
	// should rework this...
	bool flag_align;
	// pinned freespaces: how much space is actually needed for this freespace
	int total_len;
	// pinned freespaces: how much of this block we've already used
	int used_len;
};
extern autoarray<freespace_data> freespaces;

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
	bool is_for;
	string for_variable;
	string for_var_backup;
	bool for_has_var_backup;
	int for_start;
	int for_end;
	int for_cur;
};

extern autoarray<whiletracker> whilestatus;

// 0 - not first block, not in for
// 1 - first block
// 2 - inside single-line for
// 3 - after endfor
extern int single_line_for_tracker;

bool confirmname(const char * name);
string posneglabelname(const char ** input, bool define);

void write1_pick(unsigned int num);
void write2(unsigned int num);
void write3(unsigned int num);
void write4(unsigned int num);

int snestopc_pick(int addr);

int getlenfromchar(char c);

snes_label labelval(const char ** rawname, bool define = false);
snes_label labelval(string name, bool define = false);
bool labelval(const char ** rawname, snes_label * rval, bool define = false);
bool labelval(string name, snes_label * rval, bool define = false);

const char * safedequote(char * str);

void checkbankcross();

void initstuff();
void finishpass();

void assembleblock(const char * block, int& single_line_for_tracker);

extern int snespos;
extern int realsnespos;
extern int startpos;
extern int realstartpos;

extern int bytes;

extern int numif;
extern int numtrue;

extern int freespaceid;

extern assocarr<snes_label> labels;

extern autoarray<int>* macroposlabels;
extern autoarray<int>* macroneglabels;
extern autoarray<string>* macrosublabels;

extern autoarray<string> sublabels;
extern string ns;
extern autoarray<string> namespace_list;

extern autoarray<string> includeonce;
