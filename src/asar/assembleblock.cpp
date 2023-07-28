#include "addr2line.h"
#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include "platform/file-helpers.h"
#include "table.h"
#include "unicode.h"
#include <cinttypes>

#include "interface-shared.h"
#include "arch-shared.h"

int arch=arch_65816;

bool snespos_valid = false;
int snespos;
int realsnespos;
int startpos;
int realstartpos;

bool mapper_set = false;
bool warn_endwhile = true;

static int old_snespos;
static int old_startpos;
static int old_optimizeforbank;
static bool old_snespos_valid;
static int struct_base;
static string struct_name;
static string struct_parent;
static bool in_struct = false;
static bool in_sub_struct = false;
static bool static_struct = false;
static bool in_spcblock = false;

assocarr<snes_struct> structs;

static bool movinglabelspossible = false;

static bool disable_bank_cross_errors = false;
static bool check_half_banks_crossed = false;

int bytes;
static int freespaceuse=0;

static enum {
	ratsmeta_ban,
	ratsmeta_allow,
	ratsmeta_used,
} ratsmetastate=ratsmeta_ban;

enum spcblock_type{
	spcblock_nspc,
	spcblock_custom
};

struct spcblock_data{
	unsigned int destination;
	spcblock_type type;
	string macro_name;

	unsigned int execute_address;
	unsigned int size_address;
	mapper_t old_mapper;
}spcblock;

int snestopc_pick(int addr)
{
	return snestopc(addr);
}

inline void verifysnespos()
{
	if (!snespos_valid)
	{
		asar_throw_error(0, error_type_block, error_id_missing_org);
		snespos=0x008000;
		realsnespos=0x008000;
		startpos=0x008000;
		realstartpos=0x008000;
		snespos_valid = true;
	}
}

static int fixsnespos(int inaddr, int step)
{
	// randomdude999: turns out it wasn't very reliable at all.
	/* // RPG Hacker: No idea how reliable this is.
	 // Might not work with some of the more exotic mappers.
	 return pctosnes(snestopc(inaddr) + step); */
	if (mapper == lorom) {
		if ((inaddr&0xFFFF)+step > 0xFFFF) {
			// bank crossed
			return inaddr+step+0x8000;
		}
		return inaddr+step;
	} else if (mapper == hirom) {
		if ((inaddr&0x400000) == 0) {
			// system pages, need to account for low pages and stuff
			if ((inaddr&0xFFFF)+step > 0xFFFF) {
				return inaddr+step+0x8000;
			}
		}
		return inaddr+step;
	} else if (mapper == exlorom) {
		// exlorom has no mirroring so this should work fine
		return pctosnes(snestopc(inaddr)+step);
	} else if (mapper == exhirom) {
		// apparently exhirom is pretty similar to hirom after all
		if ((inaddr&0x400000) == 0) {
			// system pages, need to account for low pages and stuff
			if ((inaddr&0xFFFF)+step > 0xFFFF) {
				return inaddr+step+0x8000;
			}
		}
		return inaddr+step;
	} else if (mapper == sa1rom) {
		if((inaddr&0x400000) == 0) {
			// lorom area
			if ((inaddr&0xFFFF)+step > 0xFFFF) {
				return inaddr+step+0x8000;
			}
			return inaddr+step;
		} else {
			// hirom area
			return inaddr+step;
		}
	} else if (mapper == sfxrom) {
		if ((inaddr&0x400000) == 0) {
			// lorom area
			if ((inaddr&0xFFFF)+step > 0xFFFF) {
				return inaddr+step+0x8000;
			}
		} else {
			// hirom area
			return inaddr+step;
		}
	} else if (mapper == bigsa1rom) {
		// no mirrors here, so this should work
		return pctosnes(snestopc(inaddr)+step);
	} else if (mapper == norom) {
		return inaddr+step;
	}
	return -1;
}

inline void step(int num)
{
	if (disable_bank_cross_errors)
	{
		snespos = fixsnespos(snespos, num);
		realsnespos = fixsnespos(realsnespos, num);

		// RPG Hacker: Not adjusting startpos here will eventually throw
		// an error in checkbankcross() if we set warn bankcross on again.
		// As far as I can tell, those are pretty much just used for
		// checking bank crossing, anyways, so it's hopefully save to just
		// adjust them here.
		startpos = snespos;
		realstartpos = realsnespos;
	}
	else
	{
		snespos += num;
		realsnespos += num;
	}
	bytes+=num;
}

inline void write1_65816(unsigned int num)
{
	verifysnespos();
	if (pass==2)
	{
		int pcpos=snestopc(realsnespos&0xFFFFFF);
		if (pcpos<0)
		{
			movinglabelspossible=true;
			asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex((unsigned int)realsnespos, 6).data());
		}
		writeromdata_byte(pcpos, (unsigned char)num);
		if (pcpos>=romlen) romlen=pcpos+1;
	}
	step(1);
	ratsmetastate=ratsmeta_ban;
}

int recent_opcode_num = 0;

void write1_pick(unsigned int num)
{
	write1_65816(num);
}

static bool asblock_pick(char** word, int numwords)
{
	recent_opcode_num = 1;

	if (arch==arch_65816) return asblock_65816(word, numwords);
	if (arch==arch_spc700) return asblock_spc700(word, numwords);
	if (arch==arch_superfx) return asblock_superfx(word, numwords);
	return true;
}

#define write1 write1_pick
#define snestopc snestopc_pick

const char * safedequote(char * str)
{
	const char * tmp=dequote(str);
	if (!tmp) asar_throw_error(0, error_type_block, error_id_garbage_near_quoted_string);
	return tmp;
}

extern char romtitle[30];
extern bool stdlib;

void write2(unsigned int num)
{
	write1(num);
	write1(num/256);
}

void write3(unsigned int num)
{
	write1(num);
	write1(num/256);
	write1(num/65536);
}

void write4(unsigned int num)
{
	write1(num);
	write1(num/256);
	write1(num/65536);
	write1(num/16777216);
}

//these are NOT used by the math parser - see math.cpp for that
int read2(int insnespos)
{
	int addr=snestopc(insnespos);
	if (addr<0 || addr+2>romlen_r) return -1;
	return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8);
}

int read3(int insnespos)
{
	int addr=snestopc(insnespos);
	if (addr<0 || addr+3>romlen_r) return -1;
	return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
}

int getlenfromchar(char c)
{
	c=(char)to_lower(c);
	if (c=='b') return 1;
	if (c=='w') return 2;
	if (c=='l') return 3;
	asar_throw_error(0, error_type_block, error_id_invalid_opcode_length);
	return -1;
}

assocarr<snes_label> labels;
static autoarray<int> poslabels;
static autoarray<int> neglabels;

autoarray<int>* macroposlabels;
autoarray<int>* macroneglabels;

autoarray<string> sublabels;
autoarray<string>* macrosublabels;

// randomdude999: ns is still the string to prefix to all labels, it's calculated whenever namespace_list is changed
string ns;
string ns_backup;
autoarray<string> namespace_list;

//bool fastrom=false;

autoarray<string> includeonce;

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
	// hack for incbin -> label
	bool dont_find;
};
static autoarray<freespace_data> freespaces;

// id of the next unused freespace.
static int freespaceidnext;
// id of the current freespace, or 0 if not in freespace.
int freespaceid;
// start address of the current freespace, used for computing the length of the
// current freespace.
static int freespacestart;


bool confirmname(const char * name)
{
	if (!name[0]) return false;
	if (is_digit(name[0])) return false;
	for (int i=0;name[i];i++)
	{
		if (!is_ualnum(name[i])) return false;
	}
	return true;
}

string posneglabelname(const char ** input, bool define)
{
	const char* label = *input;

	string output;

	int depth = 0;
	bool ismacro = false;

	if (label[0] == '?')
	{
		ismacro = true;
		label++;
	}
	if (label[0] == '-' || label[0] == '+')
	{
		char first = label[0];
		for (depth = 0; label[0] && label[0] == first; depth++) label++;

		if (!ismacro)
		{
			if (first == '+')
			{
				*input = label;
				output = STR":pos_" + dec(depth) + "_" + dec(poslabels[depth]);
				if (define) poslabels[depth]++;
			}
			else
			{
				*input = label;
				if (define) neglabels[depth]++;
				output = STR":neg_" + dec(depth) + "_" + dec(neglabels[depth]);
			}
		}
		else
		{
			if (macrorecursion == 0 || macroposlabels == nullptr || macroneglabels == nullptr)
			{
				if (!macrorecursion) asar_throw_error(0, error_type_block, error_id_macro_label_outside_of_macro);
			}
			else
			{
				if (first == '+')
				{
					*input = label;
					output = STR":macro_" + dec(calledmacros) + "_pos_" + dec(depth) + "_" + dec((*macroposlabels)[depth]);
					if (define) (*macroposlabels)[depth]++;
				}
				else
				{
					*input = label;
					if (define) (*macroneglabels)[depth]++;
					output = STR":macro_" + dec(calledmacros) + "_neg_" + dec(depth) + "_" + dec((*macroneglabels)[depth]);
				}
			}
		}
	}

	return output;
}

static string labelname(const char ** rawname, bool define=false)
{
#define deref_rawname (*rawname)
	autoarray<string>* sublabellist = &sublabels;

	bool ismacro = (deref_rawname[0] == '?');
	bool issublabel = false;

	if (ismacro)
	{
		deref_rawname++;
		sublabellist = macrosublabels;
	}

	string name;
	int i=-1;

	if (is_digit(*deref_rawname)) asar_throw_error(1, error_type_block, error_id_invalid_label_name);
	if (*deref_rawname ==':')
	{
		deref_rawname++;
		name=":";
	}
	else if (!in_struct && !in_sub_struct)
	{
		for (i=0;(*deref_rawname =='.');i++) deref_rawname++;
		if (!is_ualnum(*deref_rawname)) asar_throw_error(1, error_type_block, error_id_invalid_label_name);
		if (i)
		{
			if (!sublabellist || !(*sublabellist)[i - 1]) asar_throw_error(1, error_type_block, error_id_label_missing_parent);
			name+=STR(*sublabellist)[i-1]+"_";
			issublabel = true;
		}
	}

	if (ismacro && !issublabel)
	{
		// RPG Hacker: Don't add the prefix for sublabels, because they already inherit it from
		// their parents' names.
		if (!macrorecursion || macrosublabels == nullptr) asar_throw_error(1, error_type_block, error_id_macro_label_outside_of_macro);
		name = STR":macro_" + dec(calledmacros) + "_" + name;
	}


	if (in_struct || in_sub_struct)
	{
		if(in_sub_struct)
		{
			name += struct_parent + ".";
		}
		name += struct_name;
		name += '.';
		if(*deref_rawname != '.') asar_throw_error(1, error_type_block, error_id_invalid_label_name);  //probably should be a better error. TODO!!!
		deref_rawname++;
	}

	if (!is_ualnum(*deref_rawname)) asar_throw_error(1, error_type_block, error_id_invalid_label_name);

	while (is_ualnum(*deref_rawname) || *deref_rawname == '.')
	{
		name+=*(deref_rawname++);
	}

	if(!define && *deref_rawname == '[')
	{
		while (*deref_rawname && *deref_rawname != ']') deref_rawname++;
		if(*deref_rawname != ']') asar_throw_error(1, error_type_block, error_id_invalid_label_missing_closer);
		deref_rawname++;
		if(*deref_rawname != '.') asar_throw_error(1, error_type_block, error_id_invalid_label_name);
	}

	while (is_ualnum(*deref_rawname) || *deref_rawname == '.')
	{
		name+=*(deref_rawname++);
	}

	if(*deref_rawname == '[') asar_throw_error(2, error_type_block, error_id_invalid_subscript);

	if (define && i>=0)
	{
		(*sublabellist).reset(i);
		(*sublabellist)[i]=name;
	}
	return name;
#undef deref_rawname
}

inline bool labelvalcore(const char ** rawname, snes_label * rval, bool define, bool shouldthrow)
{
	string name=labelname(rawname, define);
	if (ns && labels.exists(ns+name)) {*rval = labels.find(ns+name);}
	else if (labels.exists(name)) {*rval = labels.find(name);}
	else
	{
		if (shouldthrow && pass)
		{
			asar_throw_error(2, error_type_block, error_id_label_not_found, name.data());
		}
		rval->pos = (unsigned int)-1;
		rval->freespace_id = 0;
		rval->is_static = false;
		return false;
	}
	return true;
}

snes_label labelval(const char ** rawname, bool define)
{
	snes_label rval;
	labelvalcore(rawname, &rval, define, true);
	return rval;
}

snes_label labelval(string name, bool define)
{
	const char * rawname=name;
	snes_label rval;
	labelvalcore(&rawname, &rval, define, true);
	return rval;
}

bool labelval(const char ** rawname, snes_label * rval, bool define)
{
	return labelvalcore(rawname, rval, define, false);
}

bool labelval(string name, snes_label * rval, bool define)
{
	const char * str=name;
	return labelvalcore(&str, rval, define, false);
}

static void setlabel(string name, int loc=-1, bool is_static=false, int freespace_id = -1)
{
	if (loc==-1)
	{
		verifysnespos();
		loc=snespos;
	}

	snes_label label_data;
	label_data.pos = (unsigned int)loc;
	label_data.is_static = is_static;
	label_data.freespace_id = freespace_id == -1 ? freespaceid : freespace_id;

	unsigned int labelpos;
	if (pass==0)
	{
		if (labels.exists(name))
		{
			movinglabelspossible=true;
			asar_throw_error(0, error_type_block, error_id_label_redefined, name.data());
		}
		labels.create(name) = label_data;
	}
	else if (pass==1)
	{
		labels.create(name) = label_data;
	}
	else if (pass==2)
	{
		//all label locations are known at this point, add a sanity check
		if (!labels.exists(name)) asar_throw_error(2, error_type_block, error_id_label_on_third_pass);
		labelpos = labels.find(name).pos;
		if ((int)labelpos != loc && !movinglabelspossible)
		{
			if((unsigned int)loc>>16 != labelpos>>16)  asar_throw_error(2, error_type_block, error_id_label_ambiguous, name.raw());
			else if(labelpos == (dp_base + 0xFFu))   asar_throw_error(2, error_type_block, error_id_label_ambiguous, name.raw());
			else if(errored) return;
			else asar_throw_error(2, error_type_block, error_id_label_moving);
		}
	}
}

table thetable;
static autoarray<table> tablestack;

static void cleartable()
{
	thetable = table();
}

struct pushable {
	int arch;
	int snespos;
	int snesstart;
	int snesposreal;
	int snesstartreal;
	int freeid;
	int freest;
	int arch1;
	int arch2;
	int arch3;
	int arch4;
};
static autoarray<pushable> pushpc;
static int pushpcnum;

static autoarray<int> basestack;
static int basestacknum;

struct ns_pushable {
	string ns;
	autoarray<string> namespace_list;
	bool nested_namespaces;
};

static autoarray<ns_pushable> pushns;
static int pushnsnum;


static unsigned char fillbyte[12];
static unsigned char padbyte[12];

static bool nested_namespaces = false;

static int getfreespaceid()
{
	/*static const int max_num_freespaces = 125;
	if (freespaceidnext > max_num_freespaces) asar_throw_error(pass, error_type_fatal, error_id_freespace_limit_reached, max_num_freespaces);*/
	int newid = freespaceidnext++;
	if(newid >= freespaces.count) {
		freespaces[newid].leaked = true;
		freespaces[newid].orgpos = -2;
		freespaces[newid].orglen = -1;
		freespaces[newid].cleanbyte = 0x00;
	}
	return newid;
}

void checkbankcross()
{
	if (!snespos_valid) return;
	if (disable_bank_cross_errors) return;
	unsigned int mask = 0x7FFF0000 | (check_half_banks_crossed ? 0x8000 : 0);
	if (((snespos^    startpos) & mask) && (((snespos - 1) ^ startpos) & mask))
	{
		asar_throw_error(pass, error_type_fatal, error_id_bank_border_crossed, snespos);
	}
	else if (((realsnespos^realstartpos) & mask) && (((realsnespos - 1) ^ realstartpos) & mask))
	{
		asar_throw_error(pass, error_type_fatal, error_id_bank_border_crossed, realsnespos);
	}
}

static void freespaceend()
{
	if (freespaceid > 0)
	{
		freespaces[freespaceid].len = snespos-freespacestart;
		snespos=(int)0xFFFFFFFF;
		snespos_valid = false;
	}
	freespaceid = 0;
}

int numopcodes;

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

void initstuff()
{
	if (pass==0)
	{
		freespaces.reset();
		movinglabelspossible = false;
	}
	arch=arch_65816;
	mapper=lorom;
	mapper_set = false;
	calledmacros = 0;
	reallycalledmacros = 0;
	macrorecursion = 0;
	defines.reset();
	builtindefines.each(adddefine);
	clidefines.each(adddefine);
	ns="";
	namespace_list.reset();
	sublabels.reset();
	poslabels.reset();
	neglabels.reset();
	macroposlabels = nullptr;
	macroneglabels = nullptr;
	macrosublabels = nullptr;
	cleartable();
	pushpc.reset();
	pushpcnum=0;
	pushns.reset();
	pushnsnum = 0;
	bytes=0;
	freespaceuse=0;
	memset(fillbyte, 0, sizeof(fillbyte));
	memset(padbyte, 0, sizeof(padbyte));
	snespos_valid = false;
	snespos=(int)0xFFFFFFFF;
	realsnespos= (int)0xFFFFFFFF;
	startpos= (int)0xFFFFFFFF;
	realstartpos= (int)0xFFFFFFFF;
	//fastrom=false;
	freespaceidnext=1;
	freespaceid=0;
	numopcodes=0;
	incsrcdepth = 0;

	optimizeforbank = -1;
	optimize_dp = optimize_dp_flag::NONE;
	dp_base = 0;
	optimize_address = optimize_address_flag::DEFAULT;

	in_struct = false;
	in_sub_struct = false;
	in_spcblock = false;

	if (arch==arch_65816) asinit_65816();
	if (arch==arch_spc700) asinit_spc700();
	if (arch==arch_superfx) asinit_superfx();

	disable_bank_cross_errors = false;
	check_half_banks_crossed = false;
	nested_namespaces = false;

	includeonce.reset();

	extern AddressToLineMapping addressToLineMapping;
	addressToLineMapping.reset();

	push_warnings(false);

	initmathcore();
	
	callstack.reset();
}


void resolve_pinned_freespaces() {
	for(int i = 0; i < freespaces.count; i++)
		// default to everyone being in a separate component
		freespaces[i].pin_target_id = i;
	for(int i = 0; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		if(fs.pin_target == "") continue;
		snes_label value;
		if(fs.pin_target_ns && labels.exists(fs.pin_target_ns + fs.pin_target))
			value = labels.find(fs.pin_target_ns + fs.pin_target);
		else if(labels.exists(fs.pin_target))
			value = labels.find(fs.pin_target);
		else continue; // the error for this is thrown in the freespace command during pass 2
		int target_id = value.freespace_id;
		// do a DSU find
		while(freespaces[target_id].pin_target_id != target_id) {
			// i love programming
			freespaces[target_id].pin_target_id = freespaces[freespaces[target_id].pin_target_id].pin_target_id;
			target_id = freespaces[target_id].pin_target_id;
			// TODO actually this might be broken lol
			// should do a linear post-processing step...
		}
		// found the root of this component
		fs.pin_target_id = target_id;
		fs.len = 0;
	}
}

void allocate_freespaces() {
	for(int i = 0; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		if(fs.dont_find) continue;
		// todo lol
		printf("freespace %d: len %x isforcode %d", i, fs.len, (fs.bank == -2));
		fs.pos = getsnesfreespace(fs.len, (fs.bank == -2), true, true, fs.flag_align, fs.cleanbyte);
		printf(" pos %x\n", fs.pos);
	}
	labels.each([](const char * key, snes_label & val) {
		if(val.freespace_id != 0 && !freespaces[val.freespace_id].dont_find)
			val.pos += freespaces[val.freespace_id].pos;
	});
}

//void nerf(const string& left, string& right){puts(S left+" = "+right);}

void finishpass()
{
	verify_warnings();
	pull_warnings(false);

//defines.traverse(nerf);
	if(in_spcblock) asar_throw_error(0, error_type_block, error_id_missing_endspcblock);
	if (in_struct || in_sub_struct) asar_throw_error(pass, error_type_null, error_id_struct_without_endstruct);
	else if (pushpcnum && pass == 0) asar_throw_error(pass, error_type_null, error_id_pushpc_without_pullpc);
	else if (pushnsnum && pass == 0) asar_throw_error(pass, error_type_null, error_id_pushns_without_pullns);
	freespaceend();
	if (arch==arch_65816) asend_65816();
	if (arch==arch_spc700) asend_spc700();
	if (arch==arch_superfx) asend_superfx();

	deinitmathcore();
	if(pass == 0) {
		resolve_pinned_freespaces();
	} else if(pass == 1) {
		allocate_freespaces();
	}
}

static bool addlabel(const char * label, int pos=-1, bool global_label = false)
{
	if (!label[0] || label[0]==':') return false;//colons are reserved for special labels

	const char* posneglabel = label;
	string posnegname = posneglabelname(&posneglabel, true);

	if (posnegname.length() > 0)
	{
		if (global_label) return false;
		if (*posneglabel != '\0' && *posneglabel != ':') asar_throw_error(0, error_type_block, error_id_broken_label_definition);
		setlabel(posnegname, pos);
		return true;
	}
	if (label[strlen(label)-1]==':' || label[0]=='.' || label[0]=='?' || label[0] == '#')
	{
		if (!label[1]) return false;
		if(global_label && (in_struct || in_sub_struct || label[0]=='?')) return false;

		bool define = true;

		if (label[0] == '#')
		{
			define = false;
			label++;
		}

		// RPG Hacker: Also checking label[1] now, since it might be a macro sublabel.
		// Also, apparently this here doesn't account for main labels. I guess because
		// we don't even get here in the first place if they don't include a colon?
		bool requirecolon = (label[0] != '.' && label[1] != '.') && (in_struct || in_sub_struct);
		string name=labelname(&label, define);
		if (label[0]==':') label++;
		else if (requirecolon) asar_throw_error(0, error_type_block, error_id_broken_label_definition);
		else if (global_label) return false;
		if (label[0]) asar_throw_error(0, error_type_block, error_id_broken_label_definition);
		if (ns && !global_label) name=ns+name;
		setlabel(name, pos, ((in_struct || in_sub_struct) && static_struct));
		return true;
	}
	return false;
}

static autoarray<bool> elsestatus;
int numtrue=0;//if 1 -> increase both
int numif = 0;  //if 0 or inside if 0 -> increase only numif

autoarray<whiletracker> whilestatus;
int single_line_for_tracker;


static void push_pc()
{
	pushpc[pushpcnum].arch=arch;
	pushpc[pushpcnum].snespos=snespos;
	pushpc[pushpcnum].snesstart=startpos;
	pushpc[pushpcnum].snesposreal=realsnespos;
	pushpc[pushpcnum].snesstartreal=realstartpos;
	pushpc[pushpcnum].freeid=freespaceid;
	pushpc[pushpcnum].freest=freespacestart;
	pushpcnum++;
}

static void pop_pc()
{
	pushpcnum--;
	snespos=pushpc[pushpcnum].snespos;
	startpos=pushpc[pushpcnum].snesstart;
	realsnespos=pushpc[pushpcnum].snesposreal;
	realstartpos=pushpc[pushpcnum].snesstartreal;
	freespaceid=pushpc[pushpcnum].freeid;
	freespacestart=pushpc[pushpcnum].freest;
}


string handle_print(char* input)
{
	string out;
	autoptr<char**> pars = qpsplit(input, ',');
	verify_paren(pars);
	for (int i = 0; pars[i]; i++)
	{
		if (0);
		else if (pars[i][0] == '"') out += safedequote(pars[i]);
		else if (!stricmp(pars[i], "bytes")) out += dec(bytes);
		else if (!stricmp(pars[i], "freespaceuse")) out += dec(freespaceuse);
		else if (!stricmp(pars[i], "pc")) out += hex((unsigned int)(snespos & 0xFFFFFF), 6);
		else if (!strncasecmp(pars[i], "bin(", strlen("bin(")) ||
			!strncasecmp(pars[i], "dec(", strlen("dec(")) ||
			!strncasecmp(pars[i], "hex(", strlen("hex(")) ||
			!strncasecmp(pars[i], "double(", strlen("double(")))
		{
			char * arg1pos = strchr(pars[i], '(') + 1;
			char * endpos = strchr(arg1pos, '\0');
			while (*endpos == ' ' || *endpos == '\0') endpos--;
			if (*endpos != ')') asar_throw_error(0, error_type_block, error_id_invalid_print_function_syntax);
			string paramstr = string(arg1pos, (int)(endpos - arg1pos));

			int numargs;
			autoptr<char**> params = qpsplit(paramstr.temp_raw(), ',', &numargs);
			verify_paren(params);
			if (numargs > 2) asar_throw_error(0, error_type_block, error_id_wrong_num_parameters);
			int precision = 0;
			bool hasprec = numargs == 2;
			if (hasprec)
			{
				precision = getnum(params[1]);
				if (precision < 0) precision = 0;
				if (precision > 64) precision = 64;
			}
			*(arg1pos - 1) = '\0'; // allows more convenient comparsion functions
			if (!stricmp(pars[i], "bin"))
			{
				// sadly printf doesn't have binary, so let's roll our own
				int64_t value = getnum(params[0]);
				char buffer[65];
				if (value < 0) {
					out += '-';
					value = -value;
					// decrement precision because we've output one char already
					precision -= 1;
					if (precision<0) precision = 0;
				}
				for (int j = 0; j < 64; j++) {
					buffer[63 - j] = '0' + ((value & (1ull << j)) >> j);
				}
				buffer[64] = 0;
				int startidx = 0;
				while (startidx < 64 - precision && buffer[startidx] == '0') startidx++;
				if (startidx == 64) startidx--; // always want to print at least one digit
				out += buffer + startidx;
			}
			else if (!stricmp(pars[i], "dec"))
			{
				int64_t value = getnum(params[0]);
				char buffer[65];
				snprintf(buffer, 65, "%0*" PRId64, precision, value);
				out += buffer;
			}
			else if (!stricmp(pars[i], "hex"))
			{
				int64_t value = getnum(params[0]);
				char buffer[65];
				snprintf(buffer, 65, "%0*" PRIX64, precision, value);
				out += buffer;
			}
			else if (!stricmp(pars[i], "double"))
			{
				if (!hasprec) precision = 5;
				out += ftostrvar(getnumdouble(params[0]), precision);
			}
		}
		else asar_throw_error(2, error_type_block, error_id_unknown_variable);
	}
	return out;
}

// single_line_for_tracker is:
// 0 if not in first block of line, not in (single-line) for loop
// 1 if first block of line
// 2 if in single-line for loop
// 3 if after endfor (of a single-line loop)
void assembleblock(const char * block, int& single_line_for_tracker)
{
#define is(test) (!stricmpwithlower(word[0], test))
#define is0(test) (numwords==1 && !stricmpwithlower(word[0], test))
#define is1(test) (numwords==2 && !stricmpwithlower(word[0], test))
#define is2(test) (numwords==3 && !stricmpwithlower(word[0], test))
#define is3(test) (numwords==4 && !stricmpwithlower(word[0], test))
#define par word[1]

	callstack_push cs_push(callstack_entry_type::BLOCK, block);
	
	int numwords;
	string splitblock = block;
	char ** word = qsplit(splitblock.temp_raw(), ' ', &numwords);
	autoptr<char **> scope_word = word;
	// when writing out the data for the addrToLine mapping,
	// we want to write out the snespos we had before writing opcodes
	int addrToLinePos = realsnespos & 0xFFFFFF;

	while (numif==numtrue && word[0] && (!word[1] || strcmp(word[1], "=")) && addlabel(word[0]))
	{
		word++;
		numwords--;
	}
	if (!word[0] || !word[0][0]) return;
	if (is("if") || is("elseif") || is("assert") || is("while") || is("for"))
	{
		string errmsg;
		whiletracker wstatus;
		wstatus.startline = get_current_line();
		wstatus.iswhile = is("while");
		wstatus.cond = false;
		wstatus.is_for = false;
		wstatus.for_start = wstatus.for_end = wstatus.for_cur = 0;
		wstatus.for_has_var_backup = false;
		if(is("for")) wstatus.is_for = true;

		bool is_for_cont = false;
		// if this is a for loop and a whilestatus entry already exists at this level,
		// and the for loop isn't finished, this is a continuation of the for loop
		if (is("for") && whilestatus.count > numif && whilestatus[numif].is_for
				&& whilestatus[numif].for_cur < whilestatus[numif].for_end)
			is_for_cont = true;
		whiletracker& addedwstatus = is_for_cont ? whilestatus[numif] : (whilestatus[numif] = wstatus);
		if (is("assert"))
		{
			autoptr<char**> tokens = qpsplit(word[numwords - 1], ',');
			verify_paren(tokens);
			if (tokens[0] != NULL && tokens[1] != NULL)
			{
				string rawerrmsg;
				size_t pos = 1;
				while (tokens[pos])
				{
					rawerrmsg += tokens[pos];
					if (tokens[pos + 1] != NULL)
					{
						rawerrmsg += ",";
					}
					pos++;
				}

				errmsg = handle_print(rawerrmsg.raw());
			}
		}

		//handle nested if statements
		if (numtrue!=numif && !(is("elseif") && numtrue+1==numif))
		{
			if ((is("if") || is("while") || is("for"))) numif++;
			return;
		}
		if ((is("if") || is("while") || is("for"))) numif++;

		for(int i = 1; i < numwords - 1; i++)
		{
			word[i][strlen(word[i])] = ' ';
		}
		numwords = 2;

		bool cond;
		if(!is("for"))
		{
			cond = getnum(word[1]);
			if (foundlabel && !foundlabel_static && !is("assert")) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
		}

		if (is("for"))
		{
			if(single_line_for_tracker != 1)
			{
				numif--;
				asar_throw_error(0, error_type_line, error_id_bad_single_line_for);
			}

			if(!is_for_cont)
			{
				char* past_eq = strchr(word[1], '=');
				if(!past_eq)
					asar_throw_error(0, error_type_block, error_id_broken_for_loop, "missing loop range");

				string varname(word[1], past_eq - word[1]);
				past_eq += 1;
				strip_whitespace(varname);
				if(!validatedefinename(varname))
					asar_throw_error(0, error_type_block, error_id_broken_for_loop, "invalid define name");

				char* range_sep = strqpstr(past_eq, "..");
				if(!range_sep)
					asar_throw_error(0, error_type_block, error_id_broken_for_loop, "invalid loop range");

				string for_start(past_eq, range_sep - past_eq);
				strip_whitespace(for_start);
				string for_end(range_sep+2);
				strip_whitespace(for_end);

				addedwstatus.for_start = getnum(for_start);
				if(foundlabel && !foundlabel_static)
					asar_throw_error(0, error_type_block, error_id_label_in_conditional, "for");
				addedwstatus.for_end = getnum(for_end);
				if(foundlabel && !foundlabel_static)
					asar_throw_error(0, error_type_block, error_id_label_in_conditional, "for");

				addedwstatus.for_variable = varname;
				addedwstatus.for_cur = addedwstatus.for_start;
			}
			else addedwstatus.for_cur++;

			addedwstatus.cond = addedwstatus.for_cur < addedwstatus.for_end;
			single_line_for_tracker = 2;
			if(addedwstatus.cond)
			{
				numtrue++;
				if(defines.exists(addedwstatus.for_variable))
				{
					addedwstatus.for_has_var_backup = true;
					addedwstatus.for_var_backup = defines.find(addedwstatus.for_variable);
				}
				defines.create(addedwstatus.for_variable) = ftostr(addedwstatus.for_cur);
			}
		}
		else if (is("if") || is("while"))
		{
			if(0);
			else if (cond)
			{
				numtrue++;
				elsestatus[numif]=true;
			}
			else if (!cond)
			{
				elsestatus[numif]=false;
			}
			addedwstatus.cond = cond;
		}
		else if (is("elseif"))
		{
			if (!numif) asar_throw_error(1, error_type_block, error_id_misplaced_elseif);
			if (whilestatus[numif - 1].iswhile) asar_throw_error(1, error_type_block, error_id_elseif_in_while);
			if (numif==numtrue) numtrue--;
			if (cond && !elsestatus[numif])
			{
				numtrue++;
				elsestatus[numif]=true;
			}
		}
		// otherwise, must be assert command
		else if (pass == 2 && !cond)
		{
			if (errmsg) asar_throw_error(2, error_type_block, error_id_assertion_failed, (string(": ") + errmsg).data());
			else asar_throw_error(2, error_type_block, error_id_assertion_failed, ".");
		}
	}
	else if (is0("endif") || is0("endwhile") || is0("endfor"))
	{
		if (!numif)
			asar_throw_error(1, error_type_block, error_id_misplaced_endif);
		whiletracker& thisws = whilestatus[numif - 1];

		if((!thisws.is_for && !thisws.iswhile && !is("endif")) ||
				(thisws.iswhile && !is("endwhile")) ||
				(thisws.is_for && !is("endfor")))
			asar_throw_error(1, error_type_block, error_id_misplaced_endif);

		if (numif==numtrue) numtrue--;
		numif--;

		if(thisws.is_for)
		{
			if(single_line_for_tracker == 2) single_line_for_tracker = 3;
			if(moreonline)
			{
				// sabotage the whilestatus to prevent the loop running again
				// and spamming more of the same error
				thisws.for_cur = thisws.for_end;
				thisws.cond = false;
				asar_throw_error(0, error_type_block, error_id_bad_single_line_for);
			}

			if(thisws.cond)
			{
				if(thisws.for_has_var_backup)
					defines.create(thisws.for_variable) = thisws.for_var_backup;
				else
					defines.remove(thisws.for_variable);
			}
		}
	}
	else if (is0("else"))
	{
		if (!numif) asar_throw_error(1, error_type_block, error_id_misplaced_else);
		if (whilestatus[numif - 1].iswhile || whilestatus[numif - 1].is_for) asar_throw_error(1, error_type_block, error_id_else_in_while_loop);
		else if (numif==numtrue) numtrue--;
		else if (numif==numtrue+1 && !elsestatus[numif])
		{
			numtrue++;
			elsestatus[numif]=true;
		}
	}
	else if (numif!=numtrue) return;
	else if (asblock_pick(word, numwords))
	{
		if (pass == 2)
		{
			// RPG Hacker: This makes a pretty big assumption to calculate the size of opcodes.
			// However, this should currently only really be used for pseudo opcodes, where it should always be good enough.
			if (recent_opcode_num > 0)
			{
				int opcode_size = ((0xFFFFFF & realsnespos) - addrToLinePos) / recent_opcode_num;
				for (int i = 0; i < recent_opcode_num; ++i)
				{
					extern AddressToLineMapping addressToLineMapping;
					addressToLineMapping.includeMapping(get_current_file_name(), get_current_line() + 1, addrToLinePos + (i * opcode_size));
				}
			}
		}
		numopcodes += recent_opcode_num;
	}
	else if (is1("db") || is1("dw") || is1("dl") || is1("dd"))
	{
		autoptr<char**> pars=qpsplit(par, ',');
		verify_paren(pars);

		void (*do_write)(unsigned int);
		char first = to_lower(word[0][1]);
		if (first == 'b') do_write = &write1;
		else if (first == 'w') do_write = &write2;
		else if (first == 'l') do_write = &write3;
		else do_write = &write4;

		for (int i=0;pars[i];i++)
		{
			if (pars[i][0]=='"')
			{
				char * str=const_cast<char*>(safedequote(pars[i]));
				int codepoint = 0u;
				str += utf8_val(&codepoint, str);
				while ( codepoint != 0 && codepoint != -1 )
				{
					do_write(thetable.get_val(codepoint));
					str += utf8_val(&codepoint, str);
				}
				if (codepoint == -1) asar_throw_error(0, error_type_block, error_id_invalid_utf8);
			}
			else
			{
				do_write((pass==2)?getnum(pars[i]):0);
			}
		}
	}
	else if(word[0][0]=='%')
	{
		callmacro(strchr(block, '%')+1);
	}
	else if (is1("undef"))
	{
		string def = safedequote(par);

		if (defines.exists(def))
		{
			defines.remove(def);
		}
		else
		{
			asar_throw_error(0, error_type_block, error_id_define_not_found, def.data());
		}
	}
	else if (is0("error"))
	{
		asar_throw_error(0, error_type_block, error_id_error_command, ".");
	}
	else if (is0("warn"))
	{
		asar_throw_warning(2, warning_id_warn_command, ".");
	}
	else if (is1("error"))
	{
		string out = handle_print(par);
		// RPG Hacker: This used to be on pass 0, which had its merits (you don't want to miss a potentially critical
		// user-generated error, just because a bazillion other errors are thrown in passes before it). However, I
		// don't see how to support print functions with this without moving it to pass 2. Suggestions are welcome.
		asar_throw_error(2, error_type_block, error_id_error_command, (string(": ") + out).data());
	}
	else if (is1("warn"))
	{
		string out = handle_print(par);
		asar_throw_warning(2, warning_id_warn_command, (string(": ") + out).data());
	}
	else if (is1("warnings"))
	{
		if (stricmp(word[1], "push") == 0)
		{
			push_warnings();
		}
		else if (stricmp(word[1], "pull") == 0)
		{
			pull_warnings();
		}
		else
		{
			asar_throw_error(0, error_type_block, error_id_broken_command, "warnings", "Unknown parameter");
		}
	}
	else if (is2("warnings"))
	{
		if (stricmp(word[1], "enable") == 0)
		{
			asar_warning_id warnid = parse_warning_id_from_string(word[2]);

			if (warnid != warning_id_end)
			{
				set_warning_enabled(warnid, true);
			}
			else
			{
				asar_throw_error(0, error_type_block, error_id_invalid_warning_id, word[2], "warnings enable");
			}
		}
		else if (stricmp(word[1], "disable") == 0)
		{
			asar_warning_id warnid = parse_warning_id_from_string(word[2]);

			if (warnid != warning_id_end)
			{
				set_warning_enabled(warnid, false);
			}
			else
			{
				asar_throw_error(0, error_type_block, error_id_invalid_warning_id, word[2], "warnings disable");
			}
		}
		else
		{
			asar_throw_error(0, error_type_block, error_id_broken_command, "warnings", "Unknown parameter");
		}
	}
	else if(is1("global"))
	{
		if (!addlabel(word[1], -1, true))
		{
			asar_throw_error(1, error_type_block, error_id_invalid_global_label, word[1]);
		}
	}
	else if (is2("check"))
	{
		if (stricmp(word[1], "title") == 0)
		{
			// RPG Hacker: Removed trimming for now - I think requiring an exact match is probably
			// better here (not sure, though, it's worth discussing)
			string expected_title = safedequote(word[2]);
			// randomdude999: this also removes leading spaces because itrim gets stuck in an endless
			// loop when multi is true and one argument is empty
			// in hirom the rom needs to be an entire bank for it to have a title, other modes only need 0x8000 bytes
			if (romlen < ((mapper == hirom || mapper == exhirom) ? 0x10000 : 0x8000)) // too short
			{
				if (!ignoretitleerrors) // title errors shouldn't be ignored
					asar_throw_error(0, error_type_block, error_id_rom_too_short, expected_title.data());
				else // title errors should be ignored, throw a warning anyways
					asar_throw_warning(0, warning_id_rom_too_short, expected_title.data());
			}
			else {
				string actual_title;
				string actual_display_title;
				for (int i = 0;i < 21;i++)
				{
					unsigned char c = romdata[snestopc(0x00FFC0 + i)];
					actual_title += (char)c;
					// the replacements are from interface-cli.cpp
					if (c == 7) c = 14;
					if (c == 8) c = 27;
					if (c == 9) c = 26;
					if (c == '\r') c = 17;
					if (c == '\n') c = 25;
					if (c == '\0') c = 155;
					actual_display_title += (char)c;
				}
				if (strncmp(expected_title, actual_title, 21) != 0)
				{
					if (!ignoretitleerrors) // title errors shouldn't be ignored
						asar_throw_error(0, error_type_block, error_id_rom_title_incorrect, expected_title.data(), actual_display_title.data());
					else // title errors should be ignored, throw a warning anyways
						asar_throw_warning(0, warning_id_rom_title_incorrect, expected_title.data(), actual_display_title.data());
				}
			}
		}
		else if (stricmp(word[1], "bankcross") == 0)
		{
			if (0);
			else if (!stricmp(word[2], "off"))
			{
				 disable_bank_cross_errors = true;
			}
			else if (!stricmp(word[2], "half"))
			{
				disable_bank_cross_errors = false;
				check_half_banks_crossed = true;
			}
			else if (!stricmp(word[2], "full"))
			{
				disable_bank_cross_errors = false;
				check_half_banks_crossed = false;
			}
			else asar_throw_error(0, error_type_block, error_id_invalid_check);

		}
		else
		{
			asar_throw_error(0, error_type_block, error_id_invalid_check);
		}
	}
	else if (is0("asar") || is1("asar"))
	{
		if (!asarverallowed) asar_throw_error(0, error_type_block, error_id_start_of_file);
		if (!par) return;
		int dots=0;
		int dig=0;
		for (int i=0;par[i];i++)
		{
			if (par[i]=='.')
			{
				if (!dig) asar_throw_error(0, error_type_block, error_id_invalid_version_number);
				dig=0;
				dots++;
			}
			else if (is_digit(par[i])) dig++;
			else asar_throw_error(0, error_type_block, error_id_invalid_version_number);
		}
		if (!dig || !dots || dots>2) asar_throw_error(0, error_type_block, error_id_invalid_version_number);
		autoptr<char**> vers=split(par, '.');
		int vermaj=atoi(vers[0]);
		if (vermaj > asarver_maj) asar_throw_error(pass, error_type_fatal, error_id_asar_too_old);
		if (vermaj<asarver_maj) return;
		if (dots==1)
		{
			if (strlen(vers[1])!=2) asar_throw_error(0, error_type_block, error_id_invalid_version_number);
			//if (asarver_min<10 && asarver_bug<10 && strlen(vers[1])>2) error(0, "This version of Asar is too old for this patch.");
			int verminbug=atoi(vers[1]);
			int tmpver=asarver_bug;
			if (tmpver>9) tmpver=9;
			if (asarver_min*10+tmpver<verminbug) asar_throw_error(pass, error_type_fatal, error_id_asar_too_old);
		}
		else
		{
			int vermin=atoi(vers[1]);
			if (vermin>asarver_min) asar_throw_error(pass, error_type_fatal, error_id_asar_too_old);
			int verbug=atoi(vers[2]);
			if (vermin==asarver_min && verbug>asarver_bug) asar_throw_error(pass, error_type_fatal, error_id_asar_too_old);
		}
	}
	else if (is0("include") || is1("includefrom"))
	{
		if (!asarverallowed) asar_throw_error(0, error_type_block, error_id_start_of_file);
		if (in_top_level_file())
		{
			if (par) asar_throw_error(pass, error_type_fatal, error_id_cant_be_main_file, (string(" The main file is '") + par + "'.").data());
			else asar_throw_error(pass, error_type_fatal, error_id_cant_be_main_file, "");
		}
	}
	else if (is0("includeonce"))
	{
		const char* current_file = get_current_file_name();
		if (!file_included_once(current_file))
		{
			includeonce.append(current_file);
		}
	}
	else if (numwords==3 && !stricmp(word[1], "="))
	{
		if(word[0][0] == '\'' && word[0][1])
		{
			int codepoint;
			const char* char_start = word[0]+1;
			const char* after = char_start + utf8_val(&codepoint, char_start);
			if (codepoint == -1) asar_throw_error(0, error_type_block, error_id_invalid_utf8);
			if(after[0] == '\'' && after[1] == '\0') {
				thetable.set_val(codepoint, getnum(word[2]));
				if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
				return;
			} // todo: error checking here
		}
		// randomdude999: int cast b/c i'm too lazy to also mess with making setlabel()
		// unsigned, besides it wouldn't matter anyways.
		int num=(int)getnum(word[2]);
		if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_label_cross_assignment);

		const char* newlabelname = word[0];
		bool ismacro = false;

		if (newlabelname[0] == '?')
		{
			ismacro = true;
			newlabelname++;
		}

		if (ismacro && macrorecursion == 0)
		{
			asar_throw_error(0, error_type_block, error_id_macro_label_outside_of_macro);
		}

		if (!confirmname(newlabelname)) asar_throw_error(0, error_type_block, error_id_invalid_label_name);

		string completename;

		if (ismacro)
		{
			completename += STR":macro_" + dec(calledmacros) + "_";
		}

		completename += newlabelname;

		setlabel(ns + completename, num, true);
	}
	else if (assemblemapper(word, numwords)) {}
	else if (is1("org"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		freespaceend();
		unsigned int num=getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_org_label_forward);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex(num, 6).data());
		if ((mapper==lorom || mapper==exlorom) && (num&0x408000)==0x400000 && (num&0x700000)!=0x700000) asar_throw_warning(0, warning_id_set_middle_byte);
		//if (fastrom) num|=0x800000;
		snespos=(int)num;
		realsnespos=(int)num;
		startpos=(int)num;
		realstartpos=(int)num;
		snespos_valid = true;
	}
#define ret_error(errid) { asar_throw_error(0, error_type_block, errid); return; }
#define ret_error_params(errid, ...) { asar_throw_error(0, error_type_block, errid, __VA_ARGS__); return; }
	else if (is("struct"))
	{
		//verifysnespos();
		if (in_struct || in_sub_struct) ret_error(error_id_nested_struct);
		if (numwords < 2) ret_error(error_id_missing_struct_params);
		if (numwords > 4) ret_error(error_id_too_many_struct_params);
		if (!confirmname(word[1])) ret_error(error_id_invalid_struct_name);

		if (structs.exists(word[1]) && pass == 0) ret_error_params(error_id_struct_redefined, word[1]);

		static_struct = false;
		old_snespos = snespos;
		old_startpos = startpos;
		old_optimizeforbank = optimizeforbank;
		old_snespos_valid = snespos_valid;
		unsigned int base = 0;
		if (numwords == 3)
		{
			static_struct = true;
			base = getnum(word[2]);

			if (foundlabel && !foundlabel_static) static_struct = false;
		}

		bool old_in_struct = in_struct;
		bool old_in_sub_struct = in_sub_struct;
		in_struct = numwords == 2 || numwords == 3;
		in_sub_struct = numwords == 4;

#define ret_error_cleanup(errid) { in_struct = old_in_struct; in_sub_struct = old_in_sub_struct; asar_throw_error(0, error_type_block, errid); return; }
#define ret_error_params_cleanup(errid, ...) { in_struct = old_in_struct; in_sub_struct = old_in_sub_struct; asar_throw_error(0, error_type_block, errid, __VA_ARGS__); return; }

		if (numwords == 3)
		{
			if (base&~0xFFFFFF) ret_error_params_cleanup(error_id_snes_address_out_of_bounds, hex((unsigned int)base, 6).data());
			snespos = (int)base;
			startpos = (int)base;
		}
		else if (numwords == 4)
		{
			if (strcasecmp(word[2], "extends")) ret_error_cleanup(error_id_missing_extends);
			if (!confirmname(word[3])) ret_error_cleanup(error_id_struct_invalid_parent_name);
			string tmp_struct_parent = word[3];

			if (!structs.exists(tmp_struct_parent)) ret_error_params_cleanup(error_id_struct_not_found, tmp_struct_parent.data());
			snes_struct structure = structs.find(tmp_struct_parent);

			static_struct = structure.is_static;
			struct_parent = tmp_struct_parent;
			snespos = structure.base_end;
			startpos = structure.base_end;
		}

		push_pc();

		optimizeforbank = -1;

		struct_name = word[1];
		struct_base = snespos;
		realsnespos = 0;
		realstartpos = 0;
		snespos_valid = true;
#undef ret_error_cleanup
#undef ret_error_params_cleanup
	}
	else if (is("endstruct"))
	{
		if (numwords != 1 && numwords != 3) ret_error(error_id_invalid_endstruct_count);
		if (numwords == 3 && strcasecmp(word[1], "align")) ret_error(error_id_expected_align);
		if (!in_struct && !in_sub_struct) ret_error(error_id_endstruct_without_struct);

		int alignment = numwords == 3 ? (int)getnum(word[2]) : 1;
		if (alignment < 1) ret_error(error_id_alignment_too_small);

		snes_struct structure;
		structure.base_end = snespos;
		structure.struct_size = alignment * ((snespos - struct_base + alignment - 1) / alignment);
		structure.object_size = structure.struct_size;
		structure.is_static = static_struct;

		if (in_struct)
		{
			structs.create(struct_name) = structure;
		}
		else if (in_sub_struct)
		{
			snes_struct parent;
			parent = structs.find(struct_parent);

			if (parent.object_size < parent.struct_size + structure.struct_size) {
				parent.object_size = parent.struct_size + structure.struct_size;
			}

			structs.create(struct_parent + "." + struct_name) = structure;
			structs.create(struct_parent) = parent;
		}

		pop_pc();
		in_struct = false;
		in_sub_struct = false;
		snespos = old_snespos;
		startpos = old_startpos;
		optimizeforbank = old_optimizeforbank;
		snespos_valid = old_snespos_valid;
		static_struct = false;
	}
#undef ret_error
	else if(is("spcblock"))
	{
		//banned features when active: org, freespace(and variants), arch, mapper,namespace,pushns
		if(arch != arch_spc700)  asar_throw_error(0, error_type_block, error_id_spcblock_bad_arch);
		if(in_struct || in_sub_struct) asar_throw_error(0, error_type_block, error_id_spcblock_inside_struct);
		if(numwords < 2)  asar_throw_error(0, error_type_block, error_id_spcblock_too_few_args);
		if(numwords > 4)  asar_throw_error(0, error_type_block, error_id_spcblock_too_many_args);

		spcblock.destination = getnum(par);
		spcblock.type = spcblock_nspc;
		spcblock.macro_name = "";

		if (spcblock.destination&~0xFFFF) asar_throw_error(0, error_type_block, error_id_snes_address_out_of_bounds, hex(spcblock.destination, 6).data());

		if(numwords == 3)
		{
			if(!stricmp(word[2], "nspc")) spcblock.type = spcblock_nspc;
			else if(!stricmp(word[2], "custom")) asar_throw_error(0, error_type_block, error_id_custom_spcblock_missing_macro);
			else asar_throw_error(0, error_type_block, error_id_unknown_spcblock_type);
		}
		else if(numwords == 4)
		{
			if(!stricmp(word[2], "custom")) spcblock.type = spcblock_custom;
			else asar_throw_error(0, error_type_block, error_id_extra_spcblock_arg_for_type);

			if(macros.exists(word[3]))
			{
				macrodata *macro = macros.find(word[3]);
				if(!macro->variadic) asar_throw_error(0, error_type_block, error_id_spcblock_macro_must_be_varadic);
				if(macro->numargs != 3) asar_throw_error(0, error_type_block, error_id_spcblock_macro_invalid_static_args);
				spcblock.macro_name = word[3];
			}
			else asar_throw_error(0, error_type_block, error_id_spcblock_macro_doesnt_exist);
		}

		switch(spcblock.type)
		{
			case spcblock_nspc:
				spcblock.size_address=realsnespos;
				write2(0x0000);
				write2(spcblock.destination);
				snespos=(int)spcblock.destination;
				startpos=(int)spcblock.destination;
				spcblock.execute_address = -1u;
			break;
			case spcblock_custom:
				//this is a todo that probably won't be ready for 1.9
				//mostly so we can leverage some cleanups we make in 2.0 for practicality
				asar_throw_error(0, error_type_block, error_id_spcblock_custom_types_incomplete);
				push_pc();
				spcblock.old_mapper = mapper;
				mapper = norom;
			break;
			default:
				asar_throw_error(0, error_type_fatal, error_id_internal_error, "invalid spcblock type");
		}

		ns_backup = ns;
		ns = STR":SPCBLOCK:_" + ns_backup;
		in_spcblock = true;
	}
	else if(is0("endspcblock"))
	{
		if(!in_spcblock) asar_throw_error(0, error_type_block, error_id_endspcblock_without_spcblock);

		switch(spcblock.type)
		{
			case spcblock_nspc:
				if (pass==2)
				{
					int pcpos=snestopc(spcblock.size_address&0xFFFFFF);
					if (pcpos<0) asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex((unsigned int)realsnespos, 6).data());
					int num=snespos-startpos;
					writeromdata_byte(pcpos, (unsigned char)num);
					writeromdata_byte(pcpos+1, (unsigned char)(num >> 8));
				}
				if(spcblock.execute_address != -1u)
				{
					write2(0x0000);
					write2((unsigned int)spcblock.execute_address);
				}
			break;
			case spcblock_custom:
				mapper = spcblock.old_mapper;
				pop_pc();
			break;
			default:
				asar_throw_error(0, error_type_fatal, error_id_internal_error, "invalid spcblock type");
		}
		ns = ns_backup;
		in_spcblock = false;
	}
	else if (is1("startpos"))
	{
		if(!in_spcblock) asar_throw_error(0, error_type_block, error_id_startpos_without_spcblock);
		spcblock.execute_address=getnum(par);
	}
	else if (is1("base"))
	{
		if (!stricmp(par, "off"))
		{
			snespos=realsnespos;
			startpos=realstartpos;
			snespos_valid = realsnespos >= 0;
			return;
		}
		unsigned int num=getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_base_label_invalid);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex((unsigned int)num).data());
		snespos=(int)num;
		startpos=(int)num;
		optimizeforbank=-1;
		snespos_valid = realsnespos >= 0;
	}
	else if (is1("dpbase"))
	{
		unsigned int num=(int)getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_base_label_invalid);
		if (num&~0xFF00) asar_throw_error(1, error_type_block, error_id_bad_dp_base, hex((unsigned int)num, 6).data());
		dp_base = (int)num;
	}
	else if (is2("optimize"))
	{
		if (!stricmp(par, "dp"))
		{
			if (!stricmp(word[2], "none"))
			{
				optimize_dp = optimize_dp_flag::NONE;
				return;
			}
			if (!stricmp(word[2], "ram"))
			{
				optimize_dp = optimize_dp_flag::RAM;
				return;
			}
			if (!stricmp(word[2], "always"))
			{
				optimize_dp = optimize_dp_flag::ALWAYS;
				return;
			}
			asar_throw_error(1, error_type_block, error_id_bad_dp_optimize, word[2]);
		}
		if (!stricmp(par, "address"))
		{
			if (!stricmp(word[2], "default"))
			{
				optimize_address = optimize_address_flag::DEFAULT;
				return;
			}
			if (!stricmp(word[2], "ram"))
			{
				optimize_address = optimize_address_flag::RAM;
				return;
			}
			if (!stricmp(word[2], "mirrors"))
			{
				optimize_address = optimize_address_flag::MIRRORS;
				return;
			}
			asar_throw_error(1, error_type_block, error_id_bad_address_optimize, word[2]);
		}
		asar_throw_error(1, error_type_block, error_id_bad_optimize, par);
	}
	else if (is1("bank"))
	{
		if (!stricmp(par, "auto"))
		{
			optimizeforbank=-1;
			return;
		}
		if (!stricmp(par, "noassume"))
		{
			optimizeforbank=0x100;
			return;
		}
		unsigned int num=getnum(par);
		//if (forwardlabel) error(0, "bank Label is not valid");
		//if (foundlabel) num>>=16;
		if (num&~0x0000FF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex((unsigned int)num, 6).data());
		optimizeforbank=(int)num;
	}
	else if (is("freespace") || is("freecode") || is("freedata") || is("segment"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		string parstr;
		if (numwords==1) parstr="\n";//crappy hack: impossible character to cut out extra commas
		else if (numwords==2) parstr=word[1];
		else asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
		if (is("freecode")) parstr=STR"ram,"+parstr;
		if (is("freedata")) parstr=STR"noram,"+parstr;
		if (is("segment")) parstr = STR "norats," + parstr;
		autoptr<char**> pars=split(parstr.temp_raw(), ',');
		unsigned char fsbyte = 0x00;
		bool fixedpos=false;
		bool align=false;
		bool leakwarn=true;
		bool write_rats=true;
		int target_bank = -3;
		string pin_to_freespace = "";
		int search_start_pos = -1;

		for (int i=0;pars[i];i++)
		{
			if (pars[i][0]=='\n') {}
			else if (!stricmp(pars[i], "ram"))
			{
				if (target_bank!=-3) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				target_bank = -2;
			}
			else if (!stricmp(pars[i], "noram"))
			{
				if (target_bank!=-3) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				target_bank = -1;
			}
			else if (!stricmp(pars[i], "static"))
			{
				if (fixedpos) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				fixedpos=true;
			}
			else if (!stricmp(pars[i], "align"))
			{
				if (align) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				align=true;
			}
			else if (!stricmp(pars[i], "cleaned"))
			{
				if (!leakwarn) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				leakwarn=false;
			}
			else if (!stricmp(pars[i], "norats"))
			{
				write_rats=false;
			}
			else if (stribegin(pars[i], "bank="))
			{
				if(target_bank != -3) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				target_bank = getnum(pars[i] + 5);
				if(foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			}
			else if (stribegin(pars[i], "start="))
			{
				search_start_pos = getnum(pars[i] + 6);
				if(foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			}
			else if (stribegin(pars[i], "pin="))
			{
				// TODO: should we handle posneg labels here too?
				string pin_to = pars[i] + 4;
				const char* pin_to_c = pin_to.data();
				pin_to_freespace = labelname(&pin_to_c);
				if(*pin_to_c) asar_throw_error(0, error_type_block, error_id_invalid_label_name);
				// this is to throw an "undefined label" error with the proper callstack
				if(pass) labelval(pin_to);
			}
			else if (stribegin(pars[i], "cleanbyte="))
			{
				fsbyte = getnum(pars[i] + strlen("cleanbyte="));
				if(foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			}
			else
			{
				// for backwards compat i guess
				fsbyte = (unsigned char)getnum(pars[i]);
			}
		}
		if(target_bank == -3 && !write_rats) target_bank = -1;
		if(target_bank == -3) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
		// no point specifying anything about cleaning when not writing a rats tag
		if(!write_rats && (leakwarn || fixedpos)) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
		freespaceend();
		freespaceid=getfreespaceid();
		freespace_data& thisfs = freespaces[freespaceid];
		thisfs.cleanbyte = fsbyte;

		thisfs.pin_target = pin_to_freespace;
		if(pin_to_freespace) thisfs.pin_target_ns = ns;
		thisfs.write_rats = write_rats;
		thisfs.search_start = search_start_pos;
		thisfs.bank = target_bank;
		thisfs.flag_align = align;

		if (pass==0) snespos=0;
		if (pass==1)
		{
			if (fixedpos && thisfs.orgpos<0)
			{
				thisfs.pos = 0;
				thisfs.leaked = false;//mute some other errors
				asar_throw_error(1, error_type_block, error_id_static_freespace_autoclean);
			}
			if (fixedpos && thisfs.orgpos) thisfs.pos = snespos = thisfs.orgpos;
			else snespos = 0;
		}
		if (pass==2)
		{
			if (fixedpos && thisfs.orgpos == -1) return;//to kill some errors
			snespos=thisfs.pos;
			if (thisfs.leaked && leakwarn) asar_throw_warning(2, warning_id_freespace_leaked);
			freespaceuse += (write_rats ? 8 : 0) + thisfs.len;
		}
		thisfs.is_static = fixedpos;
		if (snespos < 0 && mapper == sa1rom) asar_throw_error(pass, error_type_fatal, error_id_no_freespace_in_mapped_banks, dec(thisfs.len).data());
		if (snespos < 0) asar_throw_error(pass, error_type_fatal, error_id_no_freespace, dec(thisfs.len).data());
		bytes+=write_rats ? 8 : 0;
		freespacestart=snespos;
		startpos=snespos;
		realstartpos=snespos;
		realsnespos=snespos;
		optimizeforbank=-1;
		ratsmetastate=write_rats ? ratsmeta_allow : ratsmeta_ban;
		snespos_valid = true;
	}
	else if (is1("prot"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		if (!ratsmetastate) asar_throw_error(2, error_type_block, error_id_prot_not_at_freespace_start);
		if (ratsmetastate==ratsmeta_used) step(-5);
		int num;
		autoptr<char**> pars=qpsplit(par, ',', &num);
		verify_paren(pars);
		write1('P');
		write1('R');
		write1('O');
		write1('T');
		if (num * 3 > 255) asar_throw_error(0, error_type_block, error_id_prot_too_many_entries);
		write1((unsigned int)(num*3));
		for (int i=0;i<num;i++)
		{
			//int num=getnum(pars[i]);
			const char * labeltest=pars[i];
			string testlabel = labeltest;
			snes_label lblval = labelval(&labeltest);
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.data());
			printf("proting %x\n", lblval.pos);
			write3(lblval.pos);
			if (pass==1) freespaces[lblval.freespace_id].leaked = false;
		}
		write1('S');
		write1('T');
		write1('O');
		write1('P');
		write1(0);
		ratsmetastate=ratsmeta_used;
	}
	else if (is1("autoclean") || is2("autoclean"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		if (numwords==3)
		{
			if(freespaceid > 0)
				asar_throw_error(0, error_type_block, error_id_autoclean_in_freespace);
			const char * labeltest = word[2];
			string testlabel = labeltest;
			snes_label lblval = labelval(&labeltest);
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.data());
			int targetid= lblval.freespace_id;
			if (pass==1) freespaces[targetid].leaked = false;
			int num = lblval.pos;
			if (strlen(par)>3 && !stricmp(par+3, ".l")) par[3]=0;
			if (!stricmp(par, "JSL") || !stricmp(par, "JML"))
			{
				int orgpos=read3(snespos+1);
				int ratsloc=freespaces[targetid].orgpos;
				int firstbyte = ((!stricmp(par, "JSL")) ? 0x22 : 0x5C);
				int pcpos=snestopc(snespos);
				if (/*pass==1 && */pcpos>=0 && pcpos<romlen_r && romdata_r[pcpos]==firstbyte)
				{
					ratsloc=ratsstart(orgpos)+8;
					freespaces[targetid].orglen=read2(ratsloc-4)+1;
					if (!freespaces[targetid].is_static && pass==1) removerats(orgpos, freespaces[targetid].cleanbyte);
				}
				else if (ratsloc<0) ratsloc=0;
				write1((unsigned int)firstbyte);
				write3((unsigned int)num);
				printf("lol. xd. %d %d %d %d, %d\n", pass, orgpos, ratsloc, num, pass==2?ratsstart(num):0);
				if (pass==2)
				{
					int start=ratsstart(num);
					if (start>=num || start<0)
					{
						asar_throw_error(2, error_type_block, error_id_autoclean_label_at_freespace_end);
					}

					extern AddressToLineMapping addressToLineMapping;
					addressToLineMapping.includeMapping(get_current_file_name(), get_current_line() + 1, addrToLinePos);
				}
				//freespaceorglen[targetid]=read2(ratsloc-4)+1;
				freespaces[targetid].orgpos = ratsloc;
			}
			else if (!stricmp(par, "dl"))
			{
				int orgpos=read3(snespos);
				int ratsloc=freespaces[targetid].orgpos;
				int start=ratsstart(num);
				if (pass==1 && num>=0)
				{
					ratsloc=ratsstart(orgpos)+8;
					if (!freespaces[targetid].is_static) removerats(orgpos, freespaces[targetid].cleanbyte);
				}
				else if (!ratsloc) ratsloc=0;
				if ((start==num || start<0) && pass==2)
					asar_throw_error(2, error_type_block, error_id_autoclean_label_at_freespace_end);
				write3((unsigned int)num);
				freespaces[targetid].orgpos = ratsloc;
				freespaces[targetid].orglen = read2(ratsloc-4)+1;
			}
			else asar_throw_error(0, error_type_block, error_id_broken_autoclean);
		}
		// weird gotcha: we don't know the freespace id here, so we don't know what clean_byte to use
		else if (pass==0) removerats((int)getnum(word[1]), 0x00);
	}
	else if (is0("pushpc"))
	{
		verifysnespos();
		pushpc[pushpcnum].arch=arch;
		pushpc[pushpcnum].snespos=snespos;
		pushpc[pushpcnum].snesstart=startpos;
		pushpc[pushpcnum].snesposreal=realsnespos;
		pushpc[pushpcnum].snesstartreal=realstartpos;
		pushpc[pushpcnum].freeid=freespaceid;
		pushpc[pushpcnum].freest=freespacestart;
		pushpcnum++;
		snespos=(int)0xFFFFFFFF;
		startpos= (int)0xFFFFFFFF;
		realsnespos= (int)0xFFFFFFFF;
		realstartpos= (int)0xFFFFFFFF;
		snespos_valid = false;
	}
	else if (is0("pullpc"))
	{
		if (!pushpcnum) asar_throw_error(0, error_type_block, error_id_pullpc_without_pushpc);
		pushpcnum--;
		freespaceend();
		if (arch != pushpc[pushpcnum].arch) asar_throw_error(0, error_type_block, error_id_pullpc_different_arch);
		snespos=pushpc[pushpcnum].snespos;
		startpos=pushpc[pushpcnum].snesstart;
		realsnespos=pushpc[pushpcnum].snesposreal;
		realstartpos=pushpc[pushpcnum].snesstartreal;
		freespaceid=pushpc[pushpcnum].freeid;
		freespacestart=pushpc[pushpcnum].freest;
		snespos_valid = true;
	}
	else if (is0("pushbase"))
	{
		basestack[basestacknum] = snespos;
		basestacknum++;
	}
	else if (is0("pullbase"))
	{
		if (!basestacknum) asar_throw_error(0, error_type_block, error_id_pullbase_without_pushbase);
		basestacknum--;
		snespos = basestack[basestacknum];
		startpos = basestack[basestacknum];

		if (snespos != realstartpos)
		{
			optimizeforbank = -1;
		}
	}
	else if (is0("pushns"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		pushns[pushnsnum].ns = ns;
		for(int i = 0; i < namespace_list.count; i++)
		{
			pushns[pushnsnum].namespace_list.append(namespace_list[i]);
		}
		pushns[pushnsnum].nested_namespaces = nested_namespaces;
		pushnsnum++;

		namespace_list.reset();
		ns = "";
		nested_namespaces = false;
	}
	else if (is0("pullns"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		if (!pushnsnum) asar_throw_error(0, error_type_block, error_id_pullns_without_pushns);
		pushnsnum--;
		ns = pushns[pushnsnum].ns;
		nested_namespaces = pushns[pushnsnum].nested_namespaces;
		namespace_list.reset();
		for(int i = 0; i < pushns[pushnsnum].namespace_list.count; i++)
		{
			namespace_list.append(pushns[pushnsnum].namespace_list[i]);
		}
	}
	else if (is1("namespace") || is2("namespace"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		bool leave = false;
		if (par)
		{
			if (!stricmp(par, "off"))
			{
				if (word[2]) asar_throw_error(0, error_type_block, error_id_invalid_namespace_use);
				leave = true;
			}
			else if (!stricmp(par, "nested"))
			{
				if (!word[2]) asar_throw_error(0, error_type_block, error_id_invalid_namespace_use);
				else if (!stricmp(word[2], "on")) nested_namespaces = true;
				else if (!stricmp(word[2], "off")) nested_namespaces = false;
			}
			else
			{
				if (word[2]) asar_throw_error(0, error_type_block, error_id_invalid_namespace_use);
				const char * tmpstr= safedequote(par);
				if (!confirmname(tmpstr)) asar_throw_error(0, error_type_block, error_id_invalid_namespace_name);
				if (!nested_namespaces)
				{
					namespace_list.reset();
				}
				namespace_list.append(tmpstr);
			}
		}
		else
		{
			leave = true;
		}

		if (leave)
		{
			if (nested_namespaces)
			{
				namespace_list.remove(namespace_list.count - 1);
			}
			else
			{
				namespace_list.reset();
			}
		}

		// recompute ns
		ns = "";
		for (int i = 0; i < namespace_list.count; i++)
		{
			ns += namespace_list[i];
			ns += "_";
		}
	}
	else if (is1("warnpc"))
	{
		unsigned int maxpos=getnum(par);
		if (freespaceid > 0) asar_throw_error(0, error_type_block, error_id_warnpc_in_freespace);
		if ((unsigned int)maxpos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_warnpc_broken_param);
		if ((unsigned int)snespos > maxpos) asar_throw_error(0, error_type_block, error_id_warnpc_failed, hex((unsigned int)snespos).data(), hex((unsigned int)maxpos, 6).data());
	}
#ifdef SANDBOX
	else if (is("incsrc") || is("incbin") || is("table"))
	{
		asar_throw_error(0, error_type_block, error_id_command_disabled);
	}
#endif
	else if (is1("incsrc"))
	{
		const char* current_file = get_current_file_name();
		string name;
		// RPG Hacker: Should this also throw on absolute paths?
		// E.g., on something starting with C:/ or whatever.
		if (strchr(par, '\\'))
		{
			asar_throw_error(0, error_type_block, error_id_platform_paths);
		}
		name=safedequote(par);
		assemblefile(name);
	}
	else if (is1("incbin") || is3("incbin"))
	{
		if (numwords == 4 && strcmp(word[2], "->")) asar_throw_error(0, error_type_block, error_id_broken_incbin);
		int len;
		int start=0;
		int end=0;
		if (strqchr(par, ':'))
		{
			char * lengths=strqchr(par, ':');
			*lengths=0;
			lengths++;

			char* split = strqpstr(lengths, "..");
			if(!split) asar_throw_error(0, error_type_block, error_id_broken_incbin);
			string start_str(lengths, split-lengths);
			if(start_str == "") asar_throw_error(0, error_type_block, error_id_broken_incbin);
			start = getnum(start_str);
			if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			string end_str(split+2);
			if(end_str == "") asar_throw_error(0, error_type_block, error_id_broken_incbin);
			end = getnum(end_str);
			if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
		}
		const char* current_file = get_current_file_name();
		string name;
		// RPG Hacker: Should this also throw on absolute paths?
		// E.g., on something starting with C:/ or whatever.
		if (strchr(par, '\\'))
		{
			asar_throw_error(0, error_type_block, error_id_platform_paths);
		}
		name = safedequote(par);
		char * data;//I couldn't find a way to get this into an autoptr
		if (!readfile(name, current_file, &data, &len)) asar_throw_error(0, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
		autoptr<char*> datacopy=data;
		if (!end) end=len;
		if(start < 0) asar_throw_error(0, error_type_block, error_id_file_offset_out_of_bounds, dec(start).data(), name.data());
		if (end < start || end > len || end < 0) asar_throw_error(0, error_type_block, error_id_file_offset_out_of_bounds, dec(end).data(), name.data());
		if (numwords==4)
		{
			asar_throw_warning(0, warning_id_feature_deprecated, "incbin with target location", "put an org before the incbin");
			if (!confirmname(word[3]))
			{
				int pos=(int)getnum(word[3]);
				if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
				int offset=snestopc(pos);
				if (offset + end - start > 0xFFFFFF) asar_throw_error(0, error_type_block, error_id_16mb_rom_limit);
				if (offset+end-start>romlen) romlen=offset+end-start;
				if (pass==2) writeromdata(offset, data+start, end-start);
			}
			else
			{
				int pos;
				if (pass==0)
				{
					if (end - start > 65536) asar_throw_error(0, error_type_block, error_id_incbin_64kb_limit);
					pos=getpcfreespace(end-start, false, true, false);
					if (pos < 0) asar_throw_error(0, error_type_block, error_id_no_freespace, dec(end - start).data());
					int foundfreespaceid=getfreespaceid();
					freespaces[foundfreespaceid].dont_find = true;
					freespaces[foundfreespaceid].pos = pctosnes(pos);
					setlabel(word[3], freespaces[foundfreespaceid].pos, false, foundfreespaceid);
					// is this necessary?
					writeromdata_bytes(pos, 0xFF, end-start);
				}
				if (pass==1)
				{
					getfreespaceid();//nothing to do here, but we want to tick the counter
				}
				if (pass==2)
				{
					int foundfreespaceid =getfreespaceid();
					if (freespaces[foundfreespaceid].leaked) asar_throw_warning(2, warning_id_freespace_leaked);
					writeromdata(snestopc(freespaces[foundfreespaceid].pos&0xFFFFFF), data+start, end-start);
					freespaceuse+=8+end-start;
				}
			}
		}
		else
		{
			for (int i=start;i<end;i++) write1((unsigned int)data[i]);
		}
	}
	else if (is("skip") || is("fill"))
	{
		if(numwords != 2 && numwords != 3 && numwords != 5) asar_throw_error(0, error_type_block, error_id_unknown_command);
		if(numwords > 2 && stricmp(word[1], "align")) asar_throw_error(0, error_type_block, error_id_unknown_command);
		if(numwords == 5 && stricmp(word[3], "offset")) asar_throw_error(0, error_type_block, error_id_unknown_command);
		int amount;
		if(numwords > 2)
		{
			int alignment = getnum(word[2]);
			if(foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			int offset = 0;
			if(numwords==5)
			{
				offset = getnum(word[4]);
				if(foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			}
			if(alignment > 0x800000) asar_throw_error(0, error_type_block, error_id_alignment_too_big);
			if(alignment < 1) asar_throw_error(0, error_type_block, error_id_alignment_too_small);
			if(alignment & (alignment-1)) asar_throw_error(0, error_type_block, error_id_invalid_alignment);
			// i just guessed this formula but it seems to work
			amount = (alignment - ((snespos - offset) & (alignment-1))) & (alignment-1);
		}
		else
		{
			amount = (int)getnum(par);
			if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
		}
		if(is("skip")) step(amount);
		else for(int i=0; i < amount; i++) write1(fillbyte[i%12]);

	}
	else if (is0("cleartable"))
	{
		cleartable();
	}
	else if (is0("pushtable"))
	{
		tablestack.append(thetable);
	}
	else if (is0("pulltable"))
	{
		if (tablestack.count <= 0) asar_throw_error(0, error_type_block, error_id_pulltable_without_table);
		thetable=tablestack[tablestack.count-1];
		tablestack.remove(tablestack.count-1);
	}
	else if (is3("function"))
	{
		if (stricmp(word[2], "=")) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
		if (!confirmqpar(word[1])) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
		string line=word[1];
		line.qnormalize();
		char * startpar=strqchr(line.data(), '(');
		if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
		*startpar=0;
		startpar++;
		if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_invalid_function_name);
		char * endpar=strqchr(startpar, ')');
		//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nulls
		if (endpar[1]) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
		*endpar=0;
		createuserfunc(line, startpar, word[3]);
	}
	else if (is1("print"))
	{
		string out = handle_print(par);
		if (pass==2) print(out);
	}
	else if (is1("reset"))
	{
		if(0);
		else if (!stricmp(par, "bytes")) bytes=0;
		else if (!stricmp(par, "freespaceuse")) freespaceuse=0;
		else asar_throw_error(2, error_type_block, error_id_unknown_variable);
	}
	else if (is1("padbyte") || is1("padword") || is1("padlong") || is1("paddword"))
	{
		int len = 0;
		if (is("padbyte")) len=1;
		if (is("padword")) len=2;
		if (is("padlong")) len=3;
		if (is("paddword")) len=4;
		unsigned int val=getnum(par);
		if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
		for (int i=0;i<12;i+=len)
		{
			unsigned int tmpval=val;
			for (int j=0;j<len;j++)
			{
				padbyte[i+j]=(unsigned char)tmpval;
				tmpval>>=8;
			}
		}
	}
	else if (is1("pad"))
	{
		if (freespaceid > 0) asar_throw_error(0, error_type_block, error_id_pad_in_freespace);
		int num=(int)getnum(par);
		if ((unsigned int)num & 0xFF000000) asar_throw_error(0, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex((unsigned int)num, 6).data());
		if (num>realsnespos)
		{
			int end=snestopc(num);
			int start=snestopc(realsnespos);
			int len=end-start;
			for (int i=0;i<len;i++) write1(padbyte[i%12]);
		}
	}
	else if (is1("fillbyte") || is1("fillword") || is1("filllong") || is1("filldword"))
	{
		int len = 0;
		if (is("fillbyte")) len=1;
		if (is("fillword")) len=2;
		if (is("filllong")) len=3;
		if (is("filldword")) len=4;
		unsigned int val= getnum(par);
		if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_block, error_id_no_labels_here);
		for (int i=0;i<12;i+=len)
		{
			unsigned int tmpval=val;
			for (int j=0;j<len;j++)
			{
				fillbyte[i+j]=(unsigned char)tmpval;
				tmpval>>=8;
			}
		}
	}
	else if (is1("arch"))
	{
		if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
		if (!stricmp(par, "65816")) { arch=arch_65816; return; }
		if (!stricmp(par, "spc700")) { arch=arch_spc700; return; }
		if (!stricmp(par, "superfx")) { arch=arch_superfx; return; }
	}
	else if (is0("{") || is0("}")) {}
	else
	{
		asar_throw_error(1, error_type_block, error_id_unknown_command);
	}

}

bool assemblemapper(char** word, int numwords)
{
	auto previous_mapper = mapper;
	if(0);
	else if (is0("lorom"))
	{
		//this also makes xkas set snespos to $008000 for some reason
		mapper=lorom;
	}
	else if (is0("hirom"))
	{
		//xkas makes this point to $C00000
		mapper=hirom;
	}
	else if (is0("exlorom"))
	{
		mapper = exlorom;
	}
	else if (is0("exhirom"))
	{
		mapper=exhirom;
	}
	else if (is0("sfxrom"))
	{
		mapper=sfxrom;
		//fastrom=false;
	}
	else if (is0("norom"))
	{
		//$000000 would be the best snespos for this, but I don't care
		mapper=norom;
		//fastrom=false;
		if(!force_checksum_fix)
			checksum_fix_enabled = false;//we don't know where the header is, so don't set the checksum
	}
	else if (is0("fullsa1rom"))
	{
		mapper=bigsa1rom;
		//fastrom=false;
	}
	else if (is("sa1rom"))
	{
		//fastrom=false;
		if (par)
		{
			if (word[2]) asar_throw_error(0, error_type_block, error_id_invalid_mapper);
			if (!is_digit(par[0]) || par[1]!=',' ||
					!is_digit(par[2]) || par[3]!=',' ||
					!is_digit(par[4]) || par[5]!=',' ||
					!is_digit(par[6]) || par[7]) asar_throw_error(0, error_type_block, error_id_invalid_mapper);
			int len;
			autoptr<char**> pars=qpsplit(par, ',', &len);
			verify_paren(pars);
			if (len!=4) asar_throw_error(0, error_type_block, error_id_invalid_mapper);
			sa1banks[0]=(par[0]-'0')<<20;
			sa1banks[1]=(par[2]-'0')<<20;
			sa1banks[4]=(par[4]-'0')<<20;
			sa1banks[5]=(par[6]-'0')<<20;
		}
		else
		{
			sa1banks[0]=0<<20;
			sa1banks[1]=1<<20;
			sa1banks[4]=2<<20;
			sa1banks[5]=3<<20;
		}
		mapper=sa1rom;
	}
	else return false;

	if(in_spcblock) asar_throw_error(0, error_type_block, error_id_feature_unavaliable_in_spcblock);
	if(!mapper_set){
		mapper_set = true;
	}else if(previous_mapper != mapper){
		asar_throw_warning(1, warning_id_mapper_already_set);
	}
	return true;
}
