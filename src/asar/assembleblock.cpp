#include "addr2line.h"
#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include "table.h"
#include "unicode.h"
#include "frozen/string.h"
#include "frozen/unordered_map.h"

#include "interface-shared.h"
#include "arch-shared.h"

int arch=arch_65816;

bool snespos_valid = false;
int snespos;
int realsnespos;
int startpos;
int realstartpos;

static bool mapper_set = false;
int label_counter = 0;

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

static struct spcblock_data{
	unsigned int destination;
	spcblock_type type;
	string macro_name;

	unsigned int size_address;
	mapper_t old_mapper;
}spcblock;

static inline void verifysnespos()
{
	if (!snespos_valid)
	{
		throw_err_block(0, err_missing_org);
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
		if ((inaddr&0x400000) == 0 && freespaceid == 0) {
			// we shouldn't get here in pass 2 inside a freespace anyways i think
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

static inline void step(int num)
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

void write1(unsigned int num)
{
	verifysnespos();
	if (pass==2)
	{
		int pcpos=snestopc(realsnespos&0xFFFFFF);
		if (pcpos<0)
		{
			movinglabelspossible=true;
			throw_err_block(2, err_snes_address_doesnt_map_to_rom, hex((unsigned int)realsnespos, 6).data());
		}
		writeromdata_byte(pcpos, (unsigned char)num, freespaceid != 0);
		if (pcpos>=romlen) {
			if(pcpos - romlen > 0) writeromdata_bytes(romlen, freespacebyte, pcpos - romlen, false);
			romlen=pcpos+1;
		}
	}
	if(pass == 1 && freespaceid == 0) {
		int pcpos = snestopc(realsnespos & 0xFFFFFF);
		if(pcpos < 0)
		{
			movinglabelspossible=true;
			throw_err_block(2, err_snes_address_doesnt_map_to_rom, hex((unsigned int)realsnespos, 6).data());
		}
		addromwrite(pcpos, 1);
		if (pcpos>=romlen) {
			if(pcpos - romlen > 0) writeromdata_bytes(romlen, freespacebyte, pcpos - romlen, false);
			romlen=pcpos+1;
		}
	}
	step(1);
	ratsmetastate=ratsmeta_ban;
}

static bool asblock_pick(const string& firstword, const char* par)
{
	if (arch==arch_spc700 || in_spcblock) {
		// TODO
		string dup(firstword);
		string duppar(par);
		char* words[] = { dup.raw(), duppar.raw() };
		return asblock_spc700(words, *par ? 2 : 1);
	}
	if (arch==arch_65816) {
		return asblock_65816(firstword, par);
	}
	if (arch==arch_superfx) {
		string dup(firstword);
		string duppar(par);
		char* words[] = { dup.raw(), duppar.raw() };
		return asblock_superfx(words, *par ? 2 : 1);
	}
	return true;
}

const char * safedequote(char * str)
{
	const char * tmp=dequote(str);
	if (!tmp) throw_err_block(0, err_garbage_near_quoted_string);
	return tmp;
}

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

//these are NOT used by the math parser - see asar_math.cpp for that
static int read2(int insnespos)
{
	int addr=snestopc(insnespos);
	if (addr<0 || addr+2>romlen_r) return -1;
	return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8);
}

static int read3(int insnespos)
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
	throw_err_block(0, err_invalid_opcode_length);
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
static string ns_backup;
autoarray<string> namespace_list;

autoarray<string> includeonce;

autoarray<freespace_data> freespaces;

// id of the next unused freespace.
static int freespaceidnext;
// id of the current freespace, or 0 if not in freespace.
int freespaceid;
// start address of the current freespace, used for computing the length of the
// current freespace.
static int freespacestart;
static freespace_data default_freespace_settings;

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
				if (!macrorecursion) throw_err_block(2, err_macro_label_outside_of_macro);
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

string labelname(const char ** rawname, bool define, bool is_addlabel)
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

	if (is_digit(*deref_rawname)) throw_err_block(2, err_invalid_label_name);
	if (*deref_rawname ==':')
	{
		deref_rawname++;
		name=":";
	}
	else if (!in_struct && !in_sub_struct)
	{
		for (i=0;(*deref_rawname =='.');i++) deref_rawname++;
		if (!is_ualnum(*deref_rawname)) throw_err_block(2, err_invalid_label_name);
		if (i)
		{
			if (!sublabellist || !(*sublabellist)[i - 1]) throw_err_block(2, err_label_missing_parent);
			name+=STR(*sublabellist)[i-1]+"_";
			issublabel = true;
		}
	}

	if (ismacro && !issublabel)
	{
		// RPG Hacker: Don't add the prefix for sublabels, because they already inherit it from
		// their parents' names.
		if (!macrorecursion || macrosublabels == nullptr) throw_err_block(2, err_macro_label_outside_of_macro);
		name = STR":macro_" + dec(calledmacros) + "_" + name;
	}


	if (in_struct || in_sub_struct)
	{
		if(is_addlabel && *deref_rawname != '.') throw_err_block(2, err_invalid_label_name);  //probably should be a better error. TODO!!!
		if(*deref_rawname == '.') {
			deref_rawname++;
			if(in_sub_struct)
			{
				name += struct_parent + ".";
			}
			name += struct_name;
			name += '.';
		}
	}

	if (!is_ualnum(*deref_rawname)) throw_err_block(2, err_invalid_label_name);

	while (is_ualnum(*deref_rawname) || *deref_rawname == '.')
	{
		name+=*(deref_rawname++);
	}

	if (define && i>=0)
	{
		(*sublabellist).reset(i);
		(*sublabellist)[i]=name;
	}
	return name;
#undef deref_rawname
}

static inline bool labelvalcore(const char ** rawname, snes_label * rval, bool define, bool shouldthrow)
{
	string name=labelname(rawname, define);
	if (ns && labels.exists(ns+name)) {*rval = labels.find(ns+name);}
	else if (labels.exists(name)) {*rval = labels.find(name);}
	else
	{
		if (shouldthrow && pass)
		{
			throw_err_block(2, err_label_not_found, name.data());
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

static void setlabel(string name, int loc=-1, bool is_static=false, int fsid=-1)
{
	if(fsid == -1) {
		fsid = 0;
		if (loc==-1)
		{
			verifysnespos();
			loc = snespos;
			// if base is not active:
			if(snespos == realsnespos) fsid = freespaceid;
			// if base is active, always treat the label as freespace 0, i.e. not freespace.
		}
	}

	snes_label label_data;
	label_data.pos = (unsigned int)loc;
	label_data.is_static = is_static;
	label_data.freespace_id = fsid;

	unsigned int labelpos;
	if (pass==0)
	{
		if (labels.exists(name))
		{
			movinglabelspossible=true;
			throw_err_block(0, err_label_redefined, name.data());
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
		if (!labels.exists(name)) throw_err_block(2, err_internal_error, "label created on 3rd pass");
		labelpos = labels.find(name).pos;
		if ((int)labelpos != loc && !movinglabelspossible)
		{
			if((unsigned int)loc>>16 != labelpos>>16)  throw_err_block(2, err_label_ambiguous, name.data());
			else if(labelpos == (dp_base + 0xFFu))   throw_err_block(2, err_label_ambiguous, name.data());
			else if(errored) return;
			else throw_err_block(2, err_internal_error, "moving label");
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

void checkbankcross()
{
	if (!snespos_valid) return;
	if (disable_bank_cross_errors) return;
	unsigned int mask = 0x7FFF0000 | (check_half_banks_crossed ? 0x8000 : 0);
	if (((snespos^startpos) & mask) && (((snespos - 1) ^ startpos) & mask))
	{
		throw_err_fatal(pass, err_bank_border_crossed, snespos);
	}
	// don't verify realsnespos when using norom. this allows making custom mappers where the file layout doesn't follow bank borders
	else if (mapper != norom && ((realsnespos^realstartpos) & mask) && (((realsnespos - 1) ^ realstartpos) & mask))
	{
		throw_err_fatal(pass, err_bank_border_crossed, realsnespos);
	}
}

static void freespaceend()
{
	if (freespaceid > 0)
	{
		freespaces[freespaceid].len = realsnespos-freespacestart;
		snespos=(int)0xFFFFFFFF;
		snespos_valid = false;
	}
	freespaceid = 0;
}

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
		found_rats_tags_initialized = false;
		found_rats_tags.clear();
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
	freespaceidnext=1;
	freespaceid=0;
	freespacebyte=0x00;
	incsrcdepth = 0;

	optimizeforbank = -1;
	optimize_dp = optimize_dp_flag::ALWAYS;
	dp_base = 0;
	optimize_address = optimize_address_flag::MIRRORS;

	in_struct = false;
	in_sub_struct = false;
	in_spcblock = false;

	disable_bank_cross_errors = false;
	check_half_banks_crossed = false;
	nested_namespaces = false;

	includeonce.reset();

	extern AddressToLineMapping addressToLineMapping;
	addressToLineMapping.reset();

	push_warnings(false);

	initmathcore();

	default_freespace_settings = {};
	default_freespace_settings.bank = -3;
	default_freespace_settings.search_start = -1;
	default_freespace_settings.write_rats = true;
	// rest are initialized to false/0/empty string

	callstack.clear();
#if defined(_WIN32) || !defined(NO_USE_THREADS)
	init_stack_use_check();
#endif
}

static void parse_freespace_arguments(freespace_data& thisfs, string& arguments) {
	if(arguments == "") return;
	autoptr<char**> pars=qpsplit(arguments.temp_raw(), ',');

	for (int i=0;pars[i];i++)
	{
		if (!stricmp(pars[i], "ram")) { thisfs.bank = -2; }
		else if (!stricmp(pars[i], "noram")) { thisfs.bank = -1; }
		else if (!stricmp(pars[i], "static")) { thisfs.is_static = true; }
		else if (!stricmp(pars[i], "nostatic")) { thisfs.is_static = false; }
		else if (!stricmp(pars[i], "align")) { thisfs.flag_align = true; }
		else if (!stricmp(pars[i], "noalign")) { thisfs.flag_align = false; }
		else if (!stricmp(pars[i], "cleaned")) { thisfs.flag_cleaned = true; }
		else if (!stricmp(pars[i], "nocleaned")) { thisfs.flag_cleaned = false; }
		else if (!stricmp(pars[i], "rats")) { thisfs.write_rats = true; }
		else if (!stricmp(pars[i], "norats")) { thisfs.write_rats = false; }
		else if (!stricmp(pars[i], "bankcross")) { thisfs.allow_bankcross = true; }
		else if (!stricmp(pars[i], "nobankcross")) { thisfs.allow_bankcross = false; }
		else if (stribegin(pars[i], "bank="))
		{
			thisfs.bank = parse_math_expr(pars[i] + 5)->evaluate_static().get_integer();
		}
		else if (stribegin(pars[i], "start="))
		{
			thisfs.search_start = parse_math_expr(pars[i] + 6)->evaluate_static().get_integer();
		}
		else if (stribegin(pars[i], "pin="))
		{
			// TODO: should we handle posneg labels here too?
			string pin_to = pars[i] + 4;
			const char* pin_to_c = pin_to.data();
			thisfs.pin_target = labelname(&pin_to_c);
			if(*pin_to_c) throw_err_block(0, err_invalid_label_name);
			// this is to throw an "undefined label" error with the proper callstack
			if(pass) labelval(pin_to);
			thisfs.pin_target_ns = ns;
		}
		else throw_err_block(0, err_invalid_freespace_request);
	}
}

static int get_freespace_pin_target(int target_id) {
	// union-find algorithm
	while(freespaces[target_id].pin_target_id != target_id) {
		// i love programming
		freespaces[target_id].pin_target_id =
			freespaces[freespaces[target_id].pin_target_id].pin_target_id;
		target_id = freespaces[target_id].pin_target_id;
	}
	return target_id;
}

static void resolve_pinned_freespaces() {
	for(int i = 1; i < freespaces.count; i++)
		// default to everyone being in a separate component
		freespaces[i].pin_target_id = i;
	for(int i = 1; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		if(fs.pin_target == "") continue;
		snes_label value;
		if(fs.pin_target_ns && labels.exists(fs.pin_target_ns + fs.pin_target))
			value = labels.find(fs.pin_target_ns + fs.pin_target);
		else if(labels.exists(fs.pin_target))
			value = labels.find(fs.pin_target);
		else continue; // the error for this is thrown in the freespace command during pass 2
		fs.pin_target_id = get_freespace_pin_target(value.freespace_id);
		fs.len = 0;
	}
	for(int i = 1; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		// just in case the pin target changed again or something
		fs.pin_target_id = get_freespace_pin_target(fs.pin_target_id);
	}
}

static void allocate_freespaces() {
	// compute real size of all pinned freespace blocks
	for(int i = 1; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		freespace_data& target = freespaces[fs.pin_target_id];
		target.total_len += fs.len;
		target.search_start = std::max(fs.search_start, target.search_start);
	}

	for(int i = 1; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		if(fs.is_static && fs.orgpos > 0) {
			fs.pos = fs.orgpos;
			continue;
		}
		// if this freespace is pinned to another one, set it later
		if(fs.pin_target_id != i) continue;
		// TODO: possibly fancier align
		fs.pos = getsnesfreespace(fs.total_len, fs.bank, true, !fs.allow_bankcross, fs.flag_align, fs.write_rats, fs.search_start);
		fs.used_len = fs.len;
	}
	// set pos for all pinned freespaces
	for(int i = 1; i < freespaces.count; i++) {
		freespace_data& fs = freespaces[i];
		if(fs.pin_target_id == i) continue;
		freespace_data& tgt = freespaces[fs.pin_target_id];
		fs.pos = tgt.pos + tgt.used_len;
		tgt.used_len += fs.len;
	}

	// relocate all labels that were in freespace to point them to their real location
	labels.each([](const char * key, snes_label & val) {
		if(val.freespace_id != 0) {
			val.pos += freespaces[val.freespace_id].pos;
		}
	});
}

void finishpass()
{
	verify_warnings();
	pull_warnings(false);

	if(in_spcblock) throw_err_block(0, err_missing_endspcblock);
	if (in_struct || in_sub_struct) throw_err_null(0, err_struct_without_endstruct);
	else if (pushpcnum) throw_err_null(0, err_pushpc_without_pullpc);
	else if (pushnsnum) throw_err_null(0, err_pushns_without_pullns);
	freespaceend();

	deinitmathcore();
	if(pass == 0) {
		resolve_pinned_freespaces();
	} else if(pass == 1) {
		allocate_freespaces();
		handle_cleared_rats_tags();
	}
#if defined(_WIN32) || !defined(NO_USE_THREADS)
	deinit_stack_use_check();
#endif
}

static bool addlabel(const char * label, int pos=-1, bool global_label = false)
{
	if (!label[0] || label[0]==':') return false;//colons are reserved for special labels

	const char* posneglabel = label;
	string posnegname = posneglabelname(&posneglabel, true);

	if (posnegname.length() > 0)
	{
		if (global_label) return false;
		if (*posneglabel != '\0' && *posneglabel != ':') throw_err_block(0, err_broken_label_definition);
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
		string name=labelname(&label, define, true);
		if (label[0]==':') label++;
		else if (requirecolon) throw_err_block(0, err_broken_label_definition);
		else if (global_label) return false;
		if (label[0]) throw_err_block(0, err_broken_label_definition);
		if (ns && !global_label) name=ns+name;
		setlabel(name, pos, ((in_struct || in_sub_struct) && static_struct));
		return true;
	}
	return false;
}

static void add_addr_to_line(int pos)
{
	if (pass == 2)
		addressToLineMapping.includeMapping(get_current_file_name(), get_current_line() + 1, pos);
}

static autoarray<bool> elsestatus;
int numtrue=0;//if 1 -> increase both
int numif = 0;  //if 0 or inside if 0 -> increase only numif

autoarray<whiletracker> whilestatus;


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


static string handle_print(const char* input)
{
	// evaluating this math can be unsafe in pass 0
	if(pass != 2) return "";
	string out;
	string buf = input;
	autoptr<char**> pars = qpsplit(buf.raw(), ',');
	verify_paren(pars);
	for (int i = 0; pars[i]; i++)
	{
		if (0);
		// leaving these here is kinda hacky, but whatever...
		else if (!stricmp(pars[i], "bytes")) out += dec(bytes);
		else if (!stricmp(pars[i], "freespaceuse")) out += dec(freespaceuse);
		else if (!stricmp(pars[i], "pc")) out += hex((unsigned int)(snespos & 0xFFFFFF), 6);
		else {
			math_val value = parse_math_expr(pars[i])->evaluate();
			out += value.get_str();
		}
	}
	return out;
}

void handle_autoclean(string& arg, int checkbyte, int write_pos)
{
	if(freespaceid > 0) throw_err_block(0, err_autoclean_in_freespace);

	const char* labeltest = arg.data();
	snes_label lblval = labelval(&labeltest);
	if (*labeltest) throw_err_block(0, err_label_not_found, arg.data());
	int num = lblval.pos;
	auto& targetfs = freespaces[lblval.freespace_id];

	if (pass == 1) {
		targetfs.leaked = false;
		int orig_pos = read3(checkbyte != -1 ? write_pos+1 : write_pos);
		int write_pos_pc = snestopc(write_pos);
		targetfs.orgpos = targetfs.orglen = -1;
		if(write_pos_pc >= 0 && write_pos_pc < romlen_r && (checkbyte == -1 || romdata_r[write_pos_pc] == checkbyte)) {
			int rats_loc = ratsstart(orig_pos);
			if(rats_loc != -1) {
				targetfs.orgpos = rats_loc + 8;
				targetfs.orglen = read2(rats_loc + 4) + 1;
				if(!targetfs.is_static) removerats(rats_loc + 8, freespacebyte);
			}
		}
	} else if(pass == 2) {
		if(targetfs.pos < 0) {
			// this freespace failed to allocate.
			// ratsstart is obviously not going to find a nonexistent freespace,
			// and would error too, we don't need 2 errors about the same thing.
			// especially if one of them is as weird as this one.
			return;
		}
		int start = ratsstart(num);
		if(start >= num || start < 0) throw_err_block(2, err_autoclean_label_at_freespace_end);
	}
}

namespace {

template<void (*do_write)(unsigned int)>
void cmd_write_data(const char* par) {
	int addrToLinePos = realsnespos & 0xFFFFFF;
	string line = par;
	autoptr<char**> pars=qpsplit(line.temp_raw(), ',');
	verify_paren(pars);

	for (int i=0;pars[i];i++)
	{
		if (pars[i][0]=='"') {
			const char * str = safedequote(pars[i]);
			int codepoint = 0u;
			str += utf8_val(&codepoint, str);
			while ( codepoint != 0 && codepoint != -1 )
			{
				do_write(thetable.get_val(codepoint));
				str += utf8_val(&codepoint, str);
			}
			if (codepoint == -1) throw_err_block(0, err_invalid_utf8);
		} else {
			do_write((pass==2) ? parse_math_expr(pars[i])->evaluate().get_integer() : 0);
		}
	}
	add_addr_to_line(addrToLinePos);
}

void cmd_assert(const char* par) {
	if(!*par) {
		throw_err_block(0, err_broken_command, "assert", "Missing condition.");
	}
	// todo optimize after adding some math helpers
	const char* message_start = strqpchr(par, ',');

	string cond_str;
	if(message_start) {
		cond_str.assign(par, message_start - par);
		message_start++; // eat the comma
	} else {
		cond_str = par;
	}

	if(pass != 2) return;

	bool cond = parse_math_expr(cond_str)->evaluate().get_bool();

	if(!cond) {
		if (message_start) throw_err_block(2, err_assertion_failed, (string(": ") + handle_print(message_start)).data());
		else throw_err_block(2, err_assertion_failed, ".");
	}
}

void cmd_undef(const char* par) {
	string temp = par;
	string def = safedequote(temp.raw());
	if (builtindefines.exists(def)) {
		throw_err_line(0, err_overriding_builtin_define, def.data());
	}

	if (defines.exists(def)) {
		defines.remove(def);
	} else {
		throw_err_block(0, err_define_not_found, def.data());
	}
}

void cmd_error(const char* par) {
	if(!*par) 
		throw_err_block(0, err_error_command, ".");
	string out = handle_print(par);
	throw_err_block(2, err_error_command, (string(": ") + out).data());
}

void cmd_warn(const char* par) {
	if(!*par) {
		throw_warning(0, warn_warn_command, ".");
		return;
	}
	string out = handle_print(par);
	throw_warning(2, warn_warn_command, (string(": ") + out).data());
}

void cmd_warnings(char** words, int numwords) {
	if(numwords == 1 && !stricmpwithlower(words[0], "push")) {
		push_warnings();
	} else if(numwords == 1 && !stricmpwithlower(words[0], "pull")) {
		pull_warnings();
	} else if(numwords == 2 && !stricmpwithlower(words[0], "enable")) {
		asar_warning_id warnid = parse_warning_id_from_string(words[1]);

		if (warnid != warning_id_end) {
			set_warning_enabled(warnid, true);
		} else {
			throw_warning(0, warn_invalid_warning_id, words[1], "warnings enable");
		}
	} else if(numwords == 2 && !stricmpwithlower(words[0], "disable")) {
		asar_warning_id warnid = parse_warning_id_from_string(words[1]);

		if (warnid != warning_id_end) {
			set_warning_enabled(warnid, false);
		} else {
			throw_warning(0, warn_invalid_warning_id, words[1], "warnings disable");
		}
	} else {
		throw_err_block(0, err_broken_command, "warnings", "Unknown parameter");
	}
}

void cmd_global(const char* par) {
	if(!addlabel(par, -1, true)) {
		throw_err_block(1, err_invalid_global_label, par);
	}
}

void cmd_check(char** words, int numwords) {
	if(numwords != 2) throw_err_block(0, err_invalid_check);

	if(!stricmpwithlower(words[0], "title")) {
		// RPG Hacker: Removed trimming for now - I think requiring an exact match is probably
		// better here (not sure, though, it's worth discussing)
		string expected_title = safedequote(words[1]);
		// in hirom the rom needs to be an entire bank for it to have a title, other modes only need 0x8000 bytes
		if (romlen < ((mapper == hirom || mapper == exhirom) ? 0x10000 : 0x8000)) // too short
		{
			if (!ignoretitleerrors) // title errors shouldn't be ignored
				throw_err_block(0, err_rom_too_short, expected_title.data());
			else // title errors should be ignored, throw a warning anyways
				throw_warning(0, warn_rom_too_short, expected_title.data());
		} else {
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
					throw_err_block(0, err_rom_title_incorrect, expected_title.data(), actual_display_title.data());
				else // title errors should be ignored, throw a warning anyways
					throw_warning(0, warn_rom_title_incorrect, expected_title.data(), actual_display_title.data());
			}
		}
	} else if(!stricmpwithlower(words[0], "bankcross")) {
		if(!stricmpwithlower(words[1], "off")) {
			disable_bank_cross_errors = true;
		} else if(!stricmpwithlower(words[1], "half")) {
			disable_bank_cross_errors = false;
			check_half_banks_crossed = true;
		} else if(!stricmpwithlower(words[1], "full")) {
			disable_bank_cross_errors = false;
			check_half_banks_crossed = false;
		} else {
			throw_err_block(0, err_invalid_check);
		}
	} else {
		throw_err_block(0, err_invalid_check);
	}
}

void cmd_asar(const char* par) {
	if(!asarverallowed) throw_err_block(0, err_start_of_file);
	if(!*par) return;
	int dots = 0;
	int dig = 0;
	for (int i=0;par[i];i++)
	{
		if (par[i]=='.')
		{
			if (!dig) throw_err_block(0, err_invalid_version_number);
			dig=0;
			dots++;
		}
		else if (is_digit(par[i])) dig++;
		else throw_err_block(0, err_invalid_version_number);
	}
	if (!dig || !dots || dots>2) throw_err_block(0, err_invalid_version_number);
	string vers_buf = par;
	autoptr<char**> vers = split(vers_buf.raw(), '.');
	int vermaj=atoi(vers[0]);
	if (vermaj > asarver_maj) throw_err_fatal(pass, err_asar_too_old);
	if (vermaj < asarver_maj) return;
	if (dots==1) {
		// todo: probably want to allow some more stuff here (it bans "asar 2.0" right now)
		if (strlen(vers[1])!=2) throw_err_block(0, err_invalid_version_number);
		int verminbug=atoi(vers[1]);
		int tmpver=asarver_bug;
		if (tmpver>9) tmpver=9;
		if (asarver_min*10+tmpver<verminbug) throw_err_fatal(pass, err_asar_too_old);
	} else {
		int vermin = atoi(vers[1]);
		if (vermin > asarver_min) throw_err_fatal(pass, err_asar_too_old);
		int verbug = atoi(vers[2]);
		if (vermin == asarver_min && verbug > asarver_bug) throw_err_fatal(pass, err_asar_too_old);
	}
}

void cmd_include(const char* par) {
	if(!asarverallowed) throw_err_block(0, err_start_of_file);
	if(*par) throw_err_block(0, err_broken_command, "include", "");
	if(in_top_level_file()) throw_err_fatal(pass, err_cant_be_main_file, "");
}

void cmd_includefrom(const char* par) {
	if(!asarverallowed) throw_err_block(0, err_start_of_file);
	if(!*par) throw_err_block(0, err_broken_command, "include", "");
	if(in_top_level_file()) throw_err_fatal(pass, err_cant_be_main_file, (string(" The main file is '") + par + "'.").data());
}

void cmd_includeonce(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "includeonce", "");

	const char* current_file = get_current_file_name();
	if (!file_included_once(current_file))
	{
		includeonce.append(current_file);
	}
}

void cmd_org(const char* par) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	freespaceend();
	auto math_expr = parse_math_expr(par);
	int64_t num = math_expr->evaluate_non_forward().get_integer();
	if (num&~0xFFFFFF) throw_err_block(1, err_snes_address_out_of_bounds, hex(num, 6).data());
	if ((mapper==lorom || mapper==exlorom) && (num&0x408000)==0x400000 && (num&0x700000)!=0x700000) throw_warning(0, warn_set_middle_byte);
	snespos=(int)num;
	realsnespos=(int)num;
	startpos=(int)num;
	realstartpos=(int)num;
	snespos_valid = true;
}

void cmd_struct(char** words, int numwords) {
		//verifysnespos();
		if (in_struct || in_sub_struct) throw_err_block(0, err_nested_struct);
		if (numwords < 1) throw_err_block(0, err_missing_struct_params);
		if (numwords > 3) throw_err_block(0, err_too_many_struct_params);
		if (!confirmname(words[0])) throw_err_block(0, err_invalid_struct_name);

		if (structs.exists(words[0]) && pass == 0) throw_err_block(0, err_struct_redefined, words[0]);

		static_struct = false;
		old_snespos = snespos;
		old_startpos = startpos;
		old_optimizeforbank = optimizeforbank;
		old_snespos_valid = snespos_valid;
		unsigned int base = 0;
		if (numwords == 2)
		{
			static_struct = true;
			auto base_expr = parse_math_expr(words[1]);
			base = base_expr->evaluate_non_forward().get_integer();

			if (base_expr->has_label() > 1) static_struct = false;
			if (pass > 0) {
				// foundlabel_static isn't accurate anymore
				if(structs.exists(words[0])) static_struct &= structs.find(words[0]).is_static;
			}
		}

		bool old_in_struct = in_struct;
		bool old_in_sub_struct = in_sub_struct;
		in_struct = numwords == 1 || numwords == 2;
		in_sub_struct = numwords == 3;

#define cleanup() (in_struct = old_in_struct, in_sub_struct = old_in_sub_struct)

		if (numwords == 2)
		{
			if (base&~0xFFFFFF) cleanup(), throw_err_block(0, err_snes_address_out_of_bounds, hex((unsigned int)base, 6).data());
			snespos = (int)base;
			startpos = (int)base;
		}
		else if (numwords == 3)
		{
			if (strcasecmp(words[1], "extends")) cleanup(), throw_err_block(0, err_missing_extends);
			if (!confirmname(words[2])) cleanup(), throw_err_block(0, err_struct_invalid_parent_name);
			string tmp_struct_parent = words[2];

			if (!structs.exists(tmp_struct_parent)) cleanup(), throw_err_block(0, err_struct_not_found, tmp_struct_parent.data());
			snes_struct structure = structs.find(tmp_struct_parent);

			static_struct = structure.is_static;
			struct_parent = tmp_struct_parent;
			snespos = structure.base_end;
			startpos = structure.base_end;
		}

		push_pc();

		optimizeforbank = -1;

		struct_name = words[0];
		struct_base = snespos;
		realsnespos = 0;
		realstartpos = 0;
		snespos_valid = true;

		if(in_sub_struct) {
			string labelname = struct_parent + "." + struct_name;
			setlabel(ns + labelname, snespos, static_struct);
		} else {
			setlabel(ns + struct_name, snespos, static_struct);
		}

#undef cleanup
}

void cmd_endstruct(char** words, int numwords) {
	if (numwords != 0 && numwords != 2) throw_err_block(0, err_invalid_endstruct_count);
	if (numwords == 2 && stricmpwithlower(words[0], "align")) throw_err_block(0, err_expected_align);
	if (!in_struct && !in_sub_struct) throw_err_block(0, err_endstruct_without_struct);

	int alignment = numwords == 2 ? (int)getnum(words[1]) : 1;
	if (alignment < 1) throw_err_block(0, err_alignment_too_small);

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

	// create a fake label for the end, so that datasize() acts nicer on the last label in the struct
	string endname = STR ":endstruct_";
	if(in_sub_struct) {
		endname += struct_parent + ".";
	}
	endname += struct_name;
	setlabel(endname, snespos, static_struct);

	pop_pc();
	in_struct = false;
	in_sub_struct = false;
	snespos = old_snespos;
	startpos = old_startpos;
	optimizeforbank = old_optimizeforbank;
	snespos_valid = old_snespos_valid;
	static_struct = false;
}

void cmd_spcblock(char** words, int num_words) {
	int addrToLinePos = realsnespos & 0xFFFFFF;
	//banned features when active: org, freespace(and variants), arch, mapper,namespace,pushns
	if(in_struct || in_sub_struct) throw_err_block(0, err_spcblock_inside_struct);
	if(num_words < 1)  throw_err_block(0, err_spcblock_too_few_args);
	if(num_words > 3)  throw_err_block(0, err_spcblock_too_many_args);

	spcblock.destination = getnum(words[0]);
	spcblock.type = spcblock_nspc;
	spcblock.macro_name = "";

	if (spcblock.destination&~0xFFFF) throw_err_block(0, err_snes_address_out_of_bounds, hex(spcblock.destination, 6).data());

	if(num_words == 2)
	{
		if(!stricmp(words[1], "nspc")) spcblock.type = spcblock_nspc;
		else if(!stricmp(words[1], "custom")) throw_err_block(0, err_custom_spcblock_missing_macro);
		else throw_err_block(0, err_unknown_spcblock_type);
	}
	else if(num_words == 3)
	{
		if(!stricmp(words[1], "custom")) spcblock.type = spcblock_custom;
		else throw_err_block(0, err_extra_spcblock_arg_for_type);

		if(macros.exists(words[2]))
		{
			macrodata *macro = macros.find(words[2]);
			if(!macro->variadic) throw_err_block(0, err_spcblock_macro_must_be_varadic);
			if(macro->numargs != 3) throw_err_block(0, err_spcblock_macro_invalid_static_args);
			spcblock.macro_name = words[2];
		}
		else throw_err_block(0, err_spcblock_macro_doesnt_exist);
	}

	switch(spcblock.type)
	{
		case spcblock_nspc:
			spcblock.size_address=realsnespos;
			write2(0x0000);
			write2(spcblock.destination);
			snespos=(int)spcblock.destination;
			startpos=(int)spcblock.destination;
			add_addr_to_line(addrToLinePos);
			break;
		case spcblock_custom:
			//this is a todo that probably won't be ready for 1.9
			//mostly so we can leverage some cleanups we make in 2.0 for practicality
			throw_err_block(0, err_spcblock_custom_types_incomplete);
			push_pc();
			spcblock.old_mapper = mapper;
			mapper = norom;
			break;
		default:
			throw_err_fatal(0, err_internal_error, "invalid spcblock type");
	}

	ns_backup = ns;
	ns = STR":SPCBLOCK:_" + ns_backup;
	in_spcblock = true;
}

void cmd_endspcblock(char** words, int num_words) {
	if(!in_spcblock) throw_err_block(0, err_endspcblock_without_spcblock);

	switch(spcblock.type)
	{
		case spcblock_nspc:
			if (pass==2)
			{
				int pcpos=snestopc(spcblock.size_address&0xFFFFFF);
				if (pcpos<0) throw_err_block(2, err_snes_address_doesnt_map_to_rom, hex((unsigned int)realsnespos, 6).data());
				// compute number of bytes written;
				// offset by 4 for the spcblock header itself
				int num = realsnespos - ((int)spcblock.size_address + 4);
				writeromdata_byte(pcpos, (unsigned char)num);
				writeromdata_byte(pcpos+1, (unsigned char)(num >> 8));
			}
			if (num_words == 2)
			{
				// todo allow spaces here
				if (strcmp(words[0], "execute")) throw_err_null(0, err_invalid_endspcblock_arg, words[0]);
				else
				{
					write2(0x0000);
					write2(pass == 2 ? (unsigned int)getnum(words[1]) : 0);
				}
			}
			else if (num_words != 0)
			{
				throw_err_null(0, err_unknown_endspcblock_format);
			}
			break;
		case spcblock_custom:
			mapper = spcblock.old_mapper;
			pop_pc();
			break;
		default:
			throw_err_fatal(0, err_internal_error, "invalid spcblock type");
	}
	ns = ns_backup;
	in_spcblock = false;
	snespos=realsnespos;
	startpos=realstartpos;
}

void cmd_base(const char* par) {
	if (!stricmp(par, "off"))
	{
		snespos=realsnespos;
		startpos=realstartpos;
		snespos_valid = realsnespos >= 0;
		return;
	}
	unsigned int num=parse_math_expr(par)->evaluate_non_forward().get_integer();
	if (num&~0xFFFFFF) throw_err_block(1, err_snes_address_out_of_bounds, hex((unsigned int)num).data());
	snespos=(int)num;
	startpos=(int)num;
	optimizeforbank=-1;
	snespos_valid = realsnespos >= 0;
}

void cmd_dpbase(const char* par) {
	unsigned int num=parse_math_expr(par)->evaluate_non_forward().get_integer();
	if (num&~0xFF00) throw_err_block(1, err_bad_dp_base, hex((unsigned int)num, 6).data());
	dp_base = (int)num;
}

void cmd_optimize(char** words, int num_words) {
	if(num_words != 2) throw_err_block(1, err_bad_optimize, "");

	if (!stricmp(words[0], "dp"))
	{
		if (!stricmp(words[1], "none"))
		{
			optimize_dp = optimize_dp_flag::NONE;
			return;
		}
		if (!stricmp(words[1], "ram"))
		{
			optimize_dp = optimize_dp_flag::RAM;
			return;
		}
		if (!stricmp(words[1], "always"))
		{
			optimize_dp = optimize_dp_flag::ALWAYS;
			return;
		}
		throw_err_block(1, err_bad_dp_optimize, words[1]);
	}
	if (!stricmp(words[0], "address"))
	{
		if (!stricmp(words[1], "default"))
		{
			optimize_address = optimize_address_flag::DEFAULT;
			return;
		}
		if (!stricmp(words[1], "ram"))
		{
			optimize_address = optimize_address_flag::RAM;
			return;
		}
		if (!stricmp(words[1], "mirrors"))
		{
			optimize_address = optimize_address_flag::MIRRORS;
			return;
		}
		throw_err_block(1, err_bad_address_optimize, words[1]);
	}
	throw_err_block(1, err_bad_optimize, words[0]);
}

void cmd_bank(const char* par) {
	if (!stricmp(par, "auto"))
	{
		optimizeforbank=-1;
		return;
	}
	if (!stricmp(par, "noassume"))
	{
		optimizeforbank=0x140;
		return;
	}
	unsigned int num=getnum(par);
	//if (forwardlabel) error(0, "bank Label is not valid");
	//if (foundlabel) num>>=16;
	if (num&~0x0000FF) throw_err_block(1, err_snes_address_out_of_bounds, hex((unsigned int)num, 6).data());
	optimizeforbank=(int)num;
}

enum class freespace_cmds {
	freespace,
	freecode,
	freedata,
	segment,
};

template<freespace_cmds the_cmd>
void cmd_freespace(const char* par) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);

	freespace_data this_fs_settings = default_freespace_settings;
	if (the_cmd == freespace_cmds::freecode) this_fs_settings.bank = -2;
	if (the_cmd == freespace_cmds::freedata) this_fs_settings.bank = -1;
	if (the_cmd == freespace_cmds::segment) this_fs_settings.write_rats = false;

	string parstr = par;
	parse_freespace_arguments(this_fs_settings, parstr);

	if(this_fs_settings.bank == -3 && !this_fs_settings.write_rats) this_fs_settings.bank = -1;
	if(this_fs_settings.bank == -3) throw_err_block(0, err_invalid_freespace_request);
	// no point specifying anything about cleaning when not writing a rats tag
	if(!this_fs_settings.write_rats &&
		(this_fs_settings.flag_cleaned || this_fs_settings.is_static))
		throw_err_block(0, err_invalid_freespace_request);
	if(!this_fs_settings.write_rats) this_fs_settings.flag_cleaned = true;
	freespaceend();
	freespaceid = freespaceidnext++;
	freespace_data& thisfs = freespaces[freespaceid];

	if (pass==0) {
		thisfs = this_fs_settings;
		thisfs.pos = -1;
		thisfs.leaked = true;
		thisfs.orgpos = -2;
		thisfs.orglen = -1;
		snespos=0;
	}
	if (pass==1)
	{
		if (thisfs.is_static && thisfs.orgpos == -2)
		{
			thisfs.pos = 0;
			thisfs.leaked = false;//mute some other errors
			throw_err_block(1, err_static_freespace_autoclean);
		}
		snespos = 0;
	}
	if (pass==2)
	{
		if (thisfs.is_static && thisfs.orgpos == -2) return;//to kill some errors (supposedly????)
		snespos=thisfs.pos;
		if (thisfs.leaked && !thisfs.flag_cleaned) throw_warning(2, warn_freespace_leaked);
		freespaceuse += (thisfs.write_rats ? 8 : 0) + thisfs.len;

		// add a mapping for the start of the rats tag
		if (thisfs.write_rats) add_addr_to_line(snespos-8);
	}
	if (snespos < 0 && mapper == sa1rom) throw_err_fatal(pass, err_no_freespace_in_mapped_banks, dec(thisfs.len).data());
	if (snespos < 0) throw_err_fatal(pass, err_no_freespace, dec(thisfs.len).data());
	bytes+=thisfs.write_rats ? 8 : 0;
	freespacestart=snespos;
	startpos=snespos;
	realstartpos=snespos;
	realsnespos=snespos;
	optimizeforbank=-1;
	ratsmetastate=thisfs.write_rats ? ratsmeta_allow : ratsmeta_ban;
	snespos_valid = true;
	// check this at the very end so that snespos gets set properly, to
	// prevent spurious errors later
	//...i guess this can still cause bankcross errors if the old freespace
	//happened to be very close to the end of a bank or something, but
	//whatever
	if (pass == 2 && thisfs.is_static && thisfs.orgpos > 0 && thisfs.len > thisfs.orglen)
		throw_err_block(2, err_static_freespace_growing);
}

void cmd_freespace_settings(const char* par) {
	string arg = par;
	parse_freespace_arguments(default_freespace_settings, arg);
}

void cmd_prot(const char* par) {
	int addrToLinePos = realsnespos & 0xFFFFFF;
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	if (!ratsmetastate) throw_err_block(2, err_prot_not_at_freespace_start);
	if (ratsmetastate==ratsmeta_used) step(-5);
	int num;
	string parbuf = par;
	autoptr<char**> pars=qpsplit(parbuf.raw(), ',', &num);
	verify_paren(pars);
	write1('P');
	write1('R');
	write1('O');
	write1('T');
	if (num * 3 > 255) throw_err_block(0, err_prot_too_many_entries);
	write1((unsigned int)(num*3));
	for (int i=0;i<num;i++)
	{
		const char * labeltest=pars[i];
		string testlabel = labeltest;
		snes_label lblval = labelval(&labeltest);
		if (*labeltest) throw_err_block(0, err_label_not_found, testlabel.data());
		write3(lblval.pos);
		if (pass==1) freespaces[lblval.freespace_id].leaked = false;
	}
	write1('S');
	write1('T');
	write1('O');
	write1('P');
	write1(0);
	ratsmetastate=ratsmeta_used;

	add_addr_to_line(addrToLinePos);
}

void cmd_autoclean(char** words, int num_words) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	if (num_words==2)
	{
		const char * labeltest = words[1];
		string testlabel = labeltest;
		if(!stricmpwithlower(words[0], "dl")) {
			handle_autoclean(testlabel, -1, snespos);

			add_addr_to_line(realsnespos & 0xFFFFFF);
			write3(pass==2 ? labelval(testlabel).pos : 0);
		} else {
			// other ones are handled in arch-65816
			throw_err_block(0, err_broken_autoclean);
		}
	}
	else if (num_words == 1) {
		if(pass==0) removerats(parse_math_expr(words[0])->evaluate_static().get_integer(), freespacebyte);
	} else {
		throw_err_block(0, err_broken_autoclean);
	}
}

void cmd_freespacebyte(const char* par) {
	freespacebyte = parse_math_expr(par)->evaluate_static().get_integer();
}

void cmd_pushpc(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pushpc", "");
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

void cmd_pullpc(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pullpc", "");
	if (!pushpcnum) throw_err_block(0, err_pullpc_without_pushpc);
	pushpcnum--;
	freespaceend();
	if (arch != pushpc[pushpcnum].arch) throw_err_block(0, err_pullpc_different_arch);
	snespos=pushpc[pushpcnum].snespos;
	startpos=pushpc[pushpcnum].snesstart;
	realsnespos=pushpc[pushpcnum].snesposreal;
	realstartpos=pushpc[pushpcnum].snesstartreal;
	freespaceid=pushpc[pushpcnum].freeid;
	freespacestart=pushpc[pushpcnum].freest;
	snespos_valid = true;
}

void cmd_pushbase(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pushbase", "");
	basestack[basestacknum] = snespos;
	basestacknum++;
}

void cmd_pullbase(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pullbase", "");
	if (!basestacknum) throw_err_block(0, err_pullbase_without_pushbase);
	basestacknum--;
	snespos = basestack[basestacknum];
	startpos = basestack[basestacknum];

	if (snespos != realstartpos)
	{
		optimizeforbank = -1;
	}
}

void cmd_pushns(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pushns", "");
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
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

void cmd_pullns(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pullns", "");
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	if (!pushnsnum) throw_err_block(0, err_pullns_without_pushns);
	pushnsnum--;
	ns = pushns[pushnsnum].ns;
	nested_namespaces = pushns[pushnsnum].nested_namespaces;
	namespace_list.reset();
	for(int i = 0; i < pushns[pushnsnum].namespace_list.count; i++)
	{
		namespace_list.append(pushns[pushnsnum].namespace_list[i]);
	}
}

void cmd_namespace(char** words, int num_words) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	bool leave = false;
	if (num_words >= 1)
	{
		if (!stricmp(words[0], "off"))
		{
			if (num_words != 1) throw_err_block(0, err_invalid_namespace_use);
			leave = true;
		}
		else if (!stricmp(words[0], "nested"))
		{
			if (num_words != 2) throw_err_block(0, err_invalid_namespace_use);
			else if (!stricmp(words[1], "on")) nested_namespaces = true;
			else if (!stricmp(words[1], "off")) nested_namespaces = false;
		}
		else
		{
			if (num_words != 1) throw_err_block(0, err_invalid_namespace_use);
			const char * tmpstr= safedequote(words[0]);
			if (!confirmname(tmpstr)) throw_err_block(0, err_invalid_namespace_name);
			if (!nested_namespaces)
			{
				namespace_list.reset();
			}
			namespace_list.append(tmpstr);
		}
	}
	else
	{
		throw_err_block(0, err_invalid_namespace_use);
		//leave = true;
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

void cmd_incsrc(const char* par) {
	// RPG Hacker: Should this also throw on absolute paths?
	// E.g., on something starting with C:/ or whatever.
	if (strchr(par, '\\'))
	{
		throw_err_block(0, err_platform_paths);
	}
	string temp = par;
	const char* name = safedequote(temp.raw());
	assemblefile(name);
}

void cmd_incbin(const char* par) {
	int addrToLinePos = realsnespos & 0xFFFFFF;
	int len;
	int start=0;
	int end=0;
	string temp = par;
	const char* lengths = strqchr(temp, ':');
	if (lengths)
	{
		lengths++;

		const char* split = strqpstr(lengths, "..");
		if(!split) throw_err_block(0, err_broken_incbin);
		string start_str(lengths, split-lengths);
		if(start_str == "") throw_err_block(0, err_broken_incbin);
		start = parse_math_expr(start_str)->evaluate_non_forward().get_integer();
		string end_str(split+2);
		if(end_str == "") throw_err_block(0, err_broken_incbin);
		end = parse_math_expr(end_str)->evaluate_non_forward().get_integer();
		// make temp just the filename without the lengths
		temp.truncate(lengths-1 - temp.data());
	}
	const char* current_file = get_current_file_name();
	// RPG Hacker: Should this also throw on absolute paths?
	// E.g., on something starting with C:/ or whatever.
	if (strchr(par, '\\'))
	{
		throw_err_block(0, err_platform_paths);
	}
	const char* name = safedequote(temp.raw());

	char * data;//I couldn't find a way to get this into an autoptr
	if (!readfile(name, current_file, &data, &len)) throw_vfile_error(0, asar_get_last_io_error(), name);
	autoptr<char*> datacopy=data;
	if (!end) end=len;
	if(start < 0) throw_err_block(1, err_file_offset_out_of_bounds, dec(start).data(), name);
	if (end < start || end > len || end < 0) throw_err_block(1, err_file_offset_out_of_bounds, dec(end).data(), name);

	for (int i=start;i<end;i++) write1((unsigned int)data[i]);
	add_addr_to_line(addrToLinePos);
}

template<bool is_fill>
void cmd_skip_fill(char** words, int num_words) {
	const char* cmdname = is_fill ? "fill" : "skip";
	if(num_words != 1 && num_words != 2 && num_words != 4) throw_err_block(0, err_broken_command, cmdname, "");
	if(num_words > 1 && stricmp(words[0], "align")) throw_err_block(0, err_broken_command, cmdname, "expected \"align\"");
	if(num_words == 4 && stricmp(words[2], "offset")) throw_err_block(0, err_broken_command, cmdname, "expected \"offset\"");
	int amount;
	if(num_words > 1)
	{
		int alignment = parse_math_expr(words[1])->evaluate_static().get_integer();
		int offset = 0;
		if(num_words==4)
		{
			offset = parse_math_expr(words[3])->evaluate_static().get_integer();
		}
		if(alignment > 0x800000) throw_err_block(0, err_alignment_too_big);
		if(alignment < 1) throw_err_block(0, err_alignment_too_small);
		if(alignment & (alignment-1)) throw_err_block(0, err_invalid_alignment);
		// i just guessed this formula but it seems to work
		amount = (alignment - ((snespos - offset) & (alignment-1))) & (alignment-1);
		// TODO: set current freespace align to at least alignment
	}
	else
	{
		amount = parse_math_expr(words[0])->evaluate_non_forward().get_integer();
	}
	if(!is_fill) step(amount);
	else
	{
		add_addr_to_line(realsnespos & 0xFFFFFF);
		for(int i=0; i < amount; i++) write1(fillbyte[i%12]);
	}
}

void cmd_cleartable(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "cleartable", "");
	cleartable();
}

void cmd_pushtable(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pushtable", "");
	tablestack.append(thetable);
}

void cmd_pulltable(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "pulltable", "");
	if (tablestack.count <= 0) throw_err_block(0, err_pulltable_without_table);
	thetable=tablestack[tablestack.count-1];
	tablestack.remove(tablestack.count-1);
}

void cmd_function(const char* par) {
	string parbuf = par;
	parbuf.qnormalize(); // todo what's the point of doing this here in particular???
	char* name_and_args = parbuf.raw();
	char* fn_body = strchr(name_and_args, '=');
	if(!fn_body) throw_err_block(0, err_broken_function_declaration);
	*fn_body = 0;
	fn_body++;
	if(!confirmqpar(name_and_args)) throw_err_block(0, err_broken_function_declaration);
	char* startpar = strqchr(name_and_args, '(');
	if (!startpar) throw_err_block(0, err_broken_function_declaration);
	*startpar = 0;
	startpar++;
	if(!confirmname(name_and_args)) throw_err_block(0, err_invalid_function_name);
	char* endpar = strqchr(startpar, ')');
	*endpar = 0;
	endpar++;
	// it's legal for there to be spaces between ) and =
	while(*endpar == ' ') endpar++;
	if(*endpar) throw_err_block(0, err_broken_function_declaration);

	createuserfunc(name_and_args, startpar, fn_body);
}

void cmd_print(const char* par) {
	string out = handle_print(par);
	if(pass == 2) print(out);
}

void cmd_reset(const char* par) {
	if(0);
	else if (!stricmp(par, "bytes")) bytes=0;
	else if (!stricmp(par, "freespaceuse")) freespaceuse=0;
	else throw_err_block(2, err_unknown_variable);
}

template<int width, bool is_fill>
void cmd_padbytes(const char* par) {
	unsigned int val = parse_math_expr(par)->evaluate_static().get_integer();
	for (int i=0;i<12;i+=width)
	{
		unsigned int tmpval=val;
		for (int j=0;j<width;j++)
		{
			(is_fill ? fillbyte : padbyte)[i+j] = (unsigned char)tmpval;
			tmpval>>=8;
		}
	}
}

void cmd_pad(const char* par) {
	if (freespaceid > 0) throw_err_block(0, err_pad_in_freespace);
	int num = parse_math_expr(par)->evaluate_non_forward().get_integer();
	if ((unsigned int)num & 0xFF000000) throw_err_block(0, err_snes_address_doesnt_map_to_rom, hex((unsigned int)num, 6).data());
	if (num>realsnespos)
	{
		int end=snestopc(num);
		int start=snestopc(realsnespos);
		int len=end-start;
		add_addr_to_line(realsnespos & 0xFFFFFF);
		for (int i=0;i<len;i++) write1(padbyte[i%12]);
	}
}

void cmd_arch(const char* par) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	if (!stricmp(par, "65816")) { arch=arch_65816; return; }
	if (!stricmp(par, "spc700")) { arch=arch_spc700; return; }
	if (!stricmp(par, "superfx")) { arch=arch_superfx; return; }
	throw_err_block(0, err_broken_command, "arch", "Invalid architecture, expected one of 65816, spc700, superfx");
}

void cmd_braces(const char* par) {
	if(*par) throw_err_block(0, err_broken_command, "{ / }", "");
}

template<void (*F)(const char*)>
void wrap_mapper(const char* par) {
	if(in_spcblock) throw_err_block(0, err_feature_unavaliable_in_spcblock);
	mapper_t previous_mapper = mapper;
	F(par);
	if(!mapper_set){
		mapper_set = true;
	}else if(previous_mapper != mapper){
		throw_warning(1, warn_mapper_already_set);
	}
}

template<mapper_t the_mapper>
void mapper_simple(const char* par) {
	if(*par) throw_err_block(0, err_invalid_mapper);
	mapper = the_mapper;
}

void mapper_norom(const char* par) {
	if(*par) throw_err_block(0, err_invalid_mapper);
	//$000000 would be the best snespos for this, but I don't care
	mapper = norom;
	if(!force_checksum_fix)
		checksum_fix_enabled = false;//we don't know where the header is, so don't set the checksum
}

void mapper_sa1rom(const char* par) {
	if (*par) {
		// todo why doesn't this allow math?
		if (!is_digit(par[0]) || par[1]!=',' ||
			!is_digit(par[2]) || par[3]!=',' ||
			!is_digit(par[4]) || par[5]!=',' ||
			!is_digit(par[6]) || par[7]) throw_err_block(0, err_invalid_mapper);
		/*
		// this part is just useless ???
		int len;
		string parbuf = par;
		autoptr<char**> pars=qpsplit(parbuf.raw(), ',', &len);
		verify_paren(pars);
		if (len!=4) throw_err_block(0, err_invalid_mapper); */
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

using command_fn_t = void(*)(const char*);

template<void (*F)(char**, int)>
void wrap_split(const char* par) {
	string temp(par);
	int num_words;
	autoptr<char**> word = qsplit(temp.raw(), ' ', &num_words);
	if(num_words == 1 && word[0][0] == 0) num_words--;
	F(word, num_words);
}

}

static constexpr auto normal_commands = frozen::make_unordered_map<frozen::string, command_fn_t>({
	{ "db", cmd_write_data<write1> },
	{ "dw", cmd_write_data<write2> },
	{ "dl", cmd_write_data<write3> },
	{ "dd", cmd_write_data<write4> },
	{ "assert", cmd_assert },
	{ "undef", cmd_undef },
	{ "error", cmd_error },
	{ "warn", cmd_warn },
	{ "warnings", wrap_split<cmd_warnings> },
	{ "global", cmd_global },
	{ "check", wrap_split<cmd_check> },
	{ "asar", cmd_asar },
	{ "include", cmd_include },
	{ "includefrom", cmd_includefrom },
	{ "includeonce", cmd_includeonce },
	{ "org", cmd_org },
	{ "struct", wrap_split<cmd_struct> },
	{ "endstruct", wrap_split<cmd_endstruct> },
	{ "spcblock", wrap_split<cmd_spcblock> },
	{ "endspcblock", wrap_split<cmd_endspcblock> },
	{ "base", cmd_base },
	{ "dpbase", cmd_dpbase },
	{ "optimize", wrap_split<cmd_optimize> },
	{ "bank", cmd_bank },
	{ "freespace", cmd_freespace<freespace_cmds::freespace> },
	{ "freecode", cmd_freespace<freespace_cmds::freecode> },
	{ "freedata", cmd_freespace<freespace_cmds::freedata> },
	{ "segment", cmd_freespace<freespace_cmds::segment> },
	{ "freespace_settings", cmd_freespace_settings },
	{ "prot", cmd_prot },
	{ "autoclean", wrap_split<cmd_autoclean> },
	{ "freespacebyte", cmd_freespacebyte },
	{ "pushpc", cmd_pushpc },
	{ "pullpc", cmd_pullpc },
	{ "pushbase", cmd_pushbase },
	{ "pullbase", cmd_pullbase },
	{ "pushns", cmd_pushns },
	{ "pullns", cmd_pullns },
	{ "namespace", wrap_split<cmd_namespace> },
	{ "incsrc", cmd_incsrc },
	{ "incbin", cmd_incbin },
	{ "skip", wrap_split<cmd_skip_fill<false>> },
	{ "fill", wrap_split<cmd_skip_fill<true>> },
	{ "cleartable", cmd_cleartable },
	{ "pushtable", cmd_pushtable },
	{ "pulltable", cmd_pulltable },
	{ "function", cmd_function },
	{ "print", cmd_print },
	{ "reset", cmd_reset },
	{ "padbyte", cmd_padbytes<1, false> },
	{ "padword", cmd_padbytes<2, false> },
	{ "padlong", cmd_padbytes<3, false> },
	{ "paddword", cmd_padbytes<4, false> },
	{ "fillbyte", cmd_padbytes<1, true> },
	{ "fillword", cmd_padbytes<2, true> },
	{ "filllong", cmd_padbytes<3, true> },
	{ "filldword", cmd_padbytes<4, true> },
	{ "pad", cmd_pad },
	{ "arch", cmd_arch },
	{ "{", cmd_braces },
	{ "}", cmd_braces },

	{ "lorom", wrap_mapper<mapper_simple<lorom>> },
	{ "hirom", wrap_mapper<mapper_simple<hirom>> },
	{ "exlorom", wrap_mapper<mapper_simple<exlorom>> },
	{ "exhirom", wrap_mapper<mapper_simple<exhirom>> },
	{ "sfxrom", wrap_mapper<mapper_simple<sfxrom>> },
	{ "norom", wrap_mapper<mapper_norom> },
	{ "sa1rom", wrap_mapper<mapper_sa1rom> },
	{ "fullsa1rom", wrap_mapper<mapper_simple<bigsa1rom>> },
});

namespace control_flow {

enum cf_cmds {
	c_if,
	c_elseif,
	c_while,
	c_for,
};

// todo possibly this would be cleaner if `for` was refactored out entirely?
template<cf_cmds type>
void cf_start(const char* par, int& single_line_for_tracker) {
	const char* typ_name =
		type == c_if ? "if" :
		type == c_elseif ? "elseif" :
		type == c_while ? "while" : "for";
	whiletracker wstatus;
	wstatus.startline = cur_logical_lineno;
	wstatus.iswhile = type == c_while;
	wstatus.cond = false;
	wstatus.is_for = false;
	wstatus.for_start = wstatus.for_end = wstatus.for_cur = 0;
	wstatus.for_has_var_backup = false;
	if(type == c_for) wstatus.is_for = true;

	bool is_for_cont = false;
	// if this is a for loop and a whilestatus entry already exists at this level,
	// and the for loop isn't finished, this is a continuation of the for loop
	if ((type == c_for)
		&& whilestatus.count > numif && whilestatus[numif].is_for
		&& whilestatus[numif].for_cur < whilestatus[numif].for_end) {
		is_for_cont = true;
	}
	whiletracker& addedwstatus = is_for_cont ? whilestatus[numif] : (whilestatus[numif] = wstatus);
	//handle nested if statements
	if (numtrue!=numif && !(type == c_elseif && numtrue+1==numif))
	{
		if (type != c_elseif) numif++;
		return;
	}
	if (type != c_elseif) numif++;

	bool cond;
	if(type != c_for)
	{
		if(!*par) throw_err_block(0, err_broken_command, typ_name, "Missing condition.");
		cond = parse_math_expr(par)->evaluate_static().get_bool();
	}

	if (type == c_for)
	{
		if(!*par) throw_err_block(0, err_broken_for_loop, "missing loop range");
		if(single_line_for_tracker != 1)
		{
			numif--;
			throw_err_line(0, err_bad_single_line_for);
		}

		if(!is_for_cont)
		{
			if(for_loop_expansion_warning) throw_warning(0, warn_for_loop_define);
			const char* past_eq = strchr(par, '=');
			if(!past_eq)
				throw_err_block(0, err_broken_for_loop, "missing loop range");

			string varname(par, past_eq - par);
			past_eq += 1;
			strip_whitespace(varname);
			if(!validatedefinename(varname))
				throw_err_block(0, err_broken_for_loop, "invalid define name");

			const char* range_sep = strqpstr(past_eq, "..");
			if(!range_sep)
				throw_err_block(0, err_broken_for_loop, "invalid loop range");

			string for_start(past_eq, range_sep - past_eq);
			strip_whitespace(for_start);
			string for_end(range_sep+2);
			strip_whitespace(for_end);

			addedwstatus.for_start = parse_math_expr(for_start)->evaluate_static().get_integer();
			addedwstatus.for_end = parse_math_expr(for_end)->evaluate_static().get_integer();

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
	else if (type == c_if || type == c_while)
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
	else if (type == c_elseif)
	{
		if (!numif) throw_err_block(1, err_misplaced_elseif);
		if (whilestatus[numif - 1].iswhile) throw_err_block(1, err_elseif_in_while);
		if (numif==numtrue) numtrue--;
		if (cond && !elsestatus[numif])
		{
			numtrue++;
			elsestatus[numif]=true;
		}
	}
}

template<cf_cmds type>
void cf_end(const char* par, int& single_line_for_tracker) {
	if(*par) throw_err_block(0, err_broken_command, "end<...>", "");
	if (!numif)
		throw_err_block(1, err_misplaced_endif);
	whiletracker& thisws = whilestatus[numif - 1];

	if((!thisws.is_for && !thisws.iswhile && type != c_if) ||
		(thisws.iswhile && type != c_while) ||
		(thisws.is_for && type != c_for))
		throw_err_block(1, err_misplaced_endif);

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
			throw_err_block(0, err_bad_single_line_for);
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

void cf_else(const char* par, int& single_line_for_tracker) {
	if(*par) throw_err_block(0, err_broken_command, "else", "");
	if (!numif) throw_err_block(1, err_misplaced_else);
	if (whilestatus[numif - 1].iswhile || whilestatus[numif - 1].is_for) throw_err_block(1, err_else_in_while_loop);
	else if (numif==numtrue) numtrue--;
	else if (numif==numtrue+1 && !elsestatus[numif])
	{
		numtrue++;
		elsestatus[numif]=true;
	}
}

}

static constexpr auto control_flow_commands = frozen::make_unordered_map<frozen::string, void(*)(const char*, int&)>({
	{ "if", control_flow::cf_start<control_flow::c_if> },
	{ "elseif", control_flow::cf_start<control_flow::c_elseif> },
	{ "while", control_flow::cf_start<control_flow::c_while> },
	{ "for", control_flow::cf_start<control_flow::c_for> },
	{ "endif", control_flow::cf_end<control_flow::c_if> },
	{ "endwhile", control_flow::cf_end<control_flow::c_while> },
	{ "endfor", control_flow::cf_end<control_flow::c_for> },
	{ "else", control_flow::cf_else },
});

static void cmd_label_assign(const char* firstword, const char* params) {
	if (firstword[0] == '\'' && firstword[1]) {
		int codepoint;
		const char* char_start = firstword + 1;
		const char* after = char_start + utf8_val(&codepoint, char_start);
		if (codepoint == -1)
			throw_err_block(0, err_invalid_utf8);
		if (after[0] == '\'' && after[1] == '\0') {
			thetable.set_val(codepoint, parse_math_expr(params + 2)->evaluate_static().get_integer());
			return;
		} else {
			throw_err_block(0, err_invalid_character);
		}
	}
	const char* newlabelname = firstword;
	bool ismacro = false;

	if (newlabelname[0] == '?') {
		ismacro = true;
		newlabelname++;
	}

	if (ismacro && macrorecursion == 0) {
		throw_err_block(0, err_macro_label_outside_of_macro);
	}

	if (!confirmname(newlabelname))
		throw_err_block(0, err_invalid_label_name);

	string completename;

	if (ismacro) {
		completename += STR ":macro_" + dec(calledmacros) + "_";
	}

	completename += newlabelname;

	auto expr = parse_math_expr(params + 2);
	bool is_static = expr->has_label() <= 1;
	int fs_id = -1;
	int64_t num;
	snes_label base; int64_t offset;
	if (expr->is_label_offset(base, offset)) {
		num = base.pos + offset;
		fs_id = base.freespace_id;
	} else {
		num = expr->evaluate_non_forward().get_integer();
	}

	if (num&~0xFFFFFF) throw_err_block(1, err_snes_address_out_of_bounds, hex(num, 6).data());

	setlabel(ns + completename, num, is_static, fs_id);
}

// single_line_for_tracker is:
// 0 if not in first block of line, not in (single-line) for loop
// 1 if first block of line
// 2 if in single-line for loop
// 3 if after endfor (of a single-line loop)
void assembleblock(const char *block, int &single_line_for_tracker) {
	// don't do this here; assembleline already did it for us
	// callstack_push cs_push(callstack_entry_type::BLOCK, block);
	if (!*block) return;

	// assumption: if the first word of a line contains quotes/spaces, it's going
	// to be invalid anyways. well, except for character assignment......
	string firstword;
	const char *params = block;
	grab_until_space(firstword, params);

	// when writing out the data for the addrToLine mapping,
	// we want to write out the snespos we had before writing opcodes
	int addrToLinePos = realsnespos & 0xFFFFFF;

	string firstword_lower = firstword;
	lower(firstword_lower);

	if (auto it = control_flow_commands.find(frozen::string(firstword_lower.data(), firstword_lower.length())); it != control_flow_commands.end()) {
		return it->second(params, single_line_for_tracker);
	} else if (numif != numtrue) {
		return;
	}

	bool addlabeled = false;
	// while first word is non-empty,
	// and rest {is empty or doesn't start with " = "},
	// try to run addlabel
	while (firstword[0]
		&& !(params[0] && params[1] && stribegin(params, "= "))
		&& addlabel(firstword)) {
		// addlabel was ok, move to the next word
		addlabeled = true;
		// if there's no next word, we're done here
		if (!*params) return;
		// otherwise pull next word into firstword
		grab_until_space(firstword, params);
	}

	firstword_lower = firstword;
	lower(firstword_lower);

	// recheck for any of the conditionals tested above
	if (addlabeled && control_flow_commands.find(frozen::string(firstword_lower.data(), firstword_lower.length())) != control_flow_commands.end()) {
		throw_err_block(0, err_label_before_if, firstword.data());
	}

	if (asblock_pick(firstword_lower, params)) {
		add_addr_to_line(addrToLinePos);
		return;
	}

	if (firstword[0] == '%') {
		// imo using strchr here is jank, but it works i guess
		// (labels can't contain %)
		return callmacro(strchr(block, '%') + 1);
	}

	if (auto it = normal_commands.find(frozen::string(firstword_lower.data(), firstword_lower.length())); it != normal_commands.end()) {
		return it->second(params);
	}

	if (stribegin(params, "= ")) return cmd_label_assign(firstword, params);

	throw_err_block(1, err_unknown_command);
}
