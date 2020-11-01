#include "addr2line.h"
#include "asar.h"
#include "libstr.h"
#include "libsmw.h"
#include "assocarr.h"
#include "autoarray.h"
#include "warnings.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include "platform/file-helpers.h"
#include <cinttypes>

#include "interface-shared.h"
#include "arch-shared.h"

int arch=arch_65816;

int snespos;
int realsnespos;
int startpos;
int realstartpos;

bool emulatexkas;
bool mapper_set = false;
static bool specifiedasarver = false;

static int old_snespos;
static int old_startpos;
static int old_optimizeforbank;
static int struct_base;
static string struct_name;
static string struct_parent;
static bool in_struct = false;
static bool in_sub_struct = false;

assocarr<snes_struct> structs;

static bool movinglabelspossible = false;

static bool disable_bank_cross_errors = false;

int bytes;

static enum {
	ratsmeta_ban,
	ratsmeta_allow,
	ratsmeta_used,
} ratsmetastate=ratsmeta_ban;

int snestopc_pick(int addr)
{
	return snestopc(addr);
}

inline void verifysnespos()
{
	if (snespos<0 || realsnespos<0)
	{
		asar_throw_warning(0, warning_id_missing_org);
		snespos=0x008000;
		realsnespos=0x008000;
		startpos=0x008000;
		realstartpos=0x008000;
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
			asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex6((unsigned int)realsnespos).data());
		}
		writeromdata_byte(pcpos, (unsigned char)num);
		if (pcpos>=romlen) romlen=pcpos+1;
	}
	step(1);
	ratsmetastate=ratsmeta_ban;
}

void write1_pick(unsigned int num)
{
	write1_65816(num);
}

static bool asblock_pick(char** word, int numwords)
{
	if (arch==arch_65816) return asblock_65816(word, numwords);
	if (arch==arch_spc700) return asblock_spc700(word, numwords);
	if (arch==arch_spc700_inline) return asblock_spc700(word, numwords);
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
int read1(int insnespos)
{
	int addr=snestopc(insnespos);
	if (addr<0 || addr+1>romlen_r) return -1;
	return
			 romdata_r[addr  ]     ;
}

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
	if (c=='d') return 4;
	asar_throw_error(0, error_type_block, error_id_invalid_opcode_length);
	return -1;
}

assocarr<unsigned int> labels;
static autoarray<int> poslabels;
static autoarray<int> neglabels;

autoarray<int>* macroposlabels;
autoarray<int>* macroneglabels;

autoarray<string> sublabels;
autoarray<string>* macrosublabels;

// randomdude999: ns is still the string to prefix to all labels, it's calculated whenever namespace_list is changed
string ns;
autoarray<string> namespace_list;

//bool fastrom=false;

autoarray<string> includeonce;

bool confirmname(const char * name)
{
	if (!name[0]) return false;
	if (is_digit(name[0])) return false;
	for (int i=0;name[i];i++)
	{
		if (!is_alnum(name[i]) && name[i]!='_') return false;
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
				output = S":pos_" + dec(depth) + "_" + dec(poslabels[depth]);
				if (define) poslabels[depth]++;
			}
			else
			{
				*input = label;
				if (define) neglabels[depth]++;
				output = S":neg_" + dec(depth) + "_" + dec(neglabels[depth]);
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
					output = S":macro_" + dec(calledmacros) + S"_pos_" + dec(depth) + "_" + dec((*macroposlabels)[depth]);
					if (define) (*macroposlabels)[depth]++;
				}
				else
				{
					*input = label;
					if (define) (*macroneglabels)[depth]++;
					output = S":macro_" + dec(calledmacros) + S"_neg_" + dec(depth) + "_" + dec((*macroneglabels)[depth]);
				}
			}
		}
	}

	return output;
}

static string labelname(const char ** rawname, bool define=false)
{
#define deref_rawname (*rawname)
	bool ismacro = (deref_rawname[0] == '?');

	if (ismacro)
	{
		deref_rawname++;
	}

	bool issublabel = false;

	string name;
	int i=-1;

	autoarray<string>* sublabellist = &sublabels;
	if (ismacro)
	{
		sublabellist = macrosublabels;
	}

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
		if (emulatexkas && i>1) asar_throw_warning(1, warning_id_convert_to_asar);
		if (i)
		{
			if (!sublabellist || !(*sublabellist)[i - 1]) asar_throw_error(1, error_type_block, error_id_label_missing_parent);
			name+=S(*sublabellist)[i-1]+"_";
			issublabel = true;
		}
	}

	if (ismacro && !issublabel)
	{
		// RPG Hacker: Don't add the prefix for sublabels, because they already inherit it from
		// their parents' names.
		if (!macrorecursion || macrosublabels == nullptr) asar_throw_error(1, error_type_block, error_id_macro_label_outside_of_macro);
		name = S":macro_" + dec(calledmacros) + "_" + name;
	}

	if(in_sub_struct)
	{
		name += struct_parent + ".";
	}

	if (in_struct || in_sub_struct)
	{
		name += struct_name;
		name += '.';
		deref_rawname++;
	}

	if (!is_ualnum(*deref_rawname)) asar_throw_error(1, error_type_block, error_id_invalid_label_name);

	while (is_ualnum(*deref_rawname) || *deref_rawname == '.' || *deref_rawname == '[')
	{
		if(!in_struct && !in_sub_struct && *deref_rawname == '[')
		{
			bool invalid = true;
			while (isprint(*deref_rawname))
			{
				if (*(deref_rawname++) == ']')
				{
					invalid = false;
					break;
				}
			}
			if (invalid)
			{
				asar_throw_error(1, error_type_block, error_id_invalid_label_missing_closer);
			}
		}
		else if (*deref_rawname == '{')
		{
			asar_throw_error(1, error_type_block, error_id_array_invalid_inside_structs);
		}

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

inline bool labelvalcore(const char ** rawname, unsigned int * rval, bool define, bool shouldthrow)
{
	string name=labelname(rawname, define);
	unsigned int rval_=0;
	if (ns && labels.exists(S ns+name)) {rval_ = labels.find(S ns+name);}
	else if (labels.exists(name)) {rval_ = labels.find(name);}
	else
	{
		if (shouldthrow && pass)
		{
			asar_throw_error(1, error_type_block, error_id_label_not_found, name.data());
		}
		if (rval) *rval=(unsigned int)-1;
		return false;
	}
	if (rval)
	{
		*rval=rval_;
		//if (fastrom && (rval_&0x700000)!=0x700000) *rval|=0x800000;
	}
	return true;
}

unsigned int labelval(const char ** rawname, bool define)
{
	unsigned int rval;
	labelvalcore(rawname, &rval, define, true);
	return rval;
}

unsigned int labelval(char ** rawname, bool define)
{
	unsigned int rval;
	labelvalcore(const_cast<const char**>(rawname), &rval, define, true);
	return rval;
}

unsigned int labelval(string name, bool define)
{
	const char * rawname=name;
	unsigned int rval;
	labelvalcore(&rawname, &rval, define, true);
	return rval;
}

bool labelval(const char ** rawname, unsigned int * rval, bool define)
{
	return labelvalcore(rawname, rval, define, false);
}

bool labelval(char ** rawname, unsigned int * rval, bool define)
{
	return labelvalcore(const_cast<const char**>(rawname), rval, define, false);
}

bool labelval(string name, unsigned int * rval, bool define)
{
	const char * str=name;
	return labelvalcore(&str, rval, define, false);
}

static void setlabel(string name, int loc=-1)
{
	if (loc==-1)
	{
		verifysnespos();
		loc=snespos;
	}
	unsigned int labelpos;
	if (pass==0)
	{
		if (labels.exists(name))
		{
			movinglabelspossible=true;
			asar_throw_error(0, error_type_block, error_id_label_redefined, name.data());
		}
		labels.create(name) = (unsigned int)loc;
	}
	else if (pass==1)
	{
		labels.create(name) = (unsigned int)loc;
	}
	else if (pass==2)
	{
		//all label locations are known at this point, add a sanity check
		if (!labels.exists(name)) asar_throw_error(2, error_type_block, error_id_label_on_third_pass);
		labelpos = labels.find(name);
		if ((int)labelpos != loc && !movinglabelspossible) asar_throw_error(2, error_type_block, error_id_label_moving);
	}
}


chartabledata table;
static autoarray<chartabledata> tablestack;

static int freespacepos[256];
static int freespacelen[256];
static int freespaceidnext;
static int freespaceid;
static int freespacestart;
int freespaceextra;

static bool freespaceleak[256];
static string freespacefile[256];

static int freespaceorgpos[256];
static int freespaceorglen[256];
static bool freespacestatic[256];
static unsigned char freespacebyte[256];

static void cleartable()
{
	for (unsigned int i=0;i<256;i++) table.table[i]=i;
}

struct pushable {
	int arch;
	int snespos;
	int snesstart;
	int snesposreal;
	int snesstartreal;
	int freeid;
	int freeex;
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

static unsigned char fillbyte[12];
static unsigned char padbyte[12];

static bool nested_namespaces = false;

static int getfreespaceid()
{
	static const int max_num_freespaces = 125;
	if (freespaceidnext > max_num_freespaces) asar_throw_error(pass, error_type_fatal, error_id_freespace_limit_reached, max_num_freespaces);
	return freespaceidnext++;
}

void checkbankcross()
{
	if (snespos<0 && realsnespos<0 && startpos<0 && realstartpos<0) return;
	if (disable_bank_cross_errors) return;
	if ( (((snespos^    startpos) & 0x7FFF0000) && (((snespos - 1) ^ startpos) & 0x7FFF0000))
		|| (((realsnespos^realstartpos) & 0x7FFF0000) && (((realsnespos - 1) ^ realstartpos) & 0x7FFF0000)) )
	{
		asar_throw_error(pass, error_type_fatal, error_id_bank_border_crossed);
	}
}

static void freespaceend()
{
	checkbankcross();
	if ((snespos&0x7F000000) && ((unsigned int)snespos&0x80000000)==0)
	{
		freespacelen[freespaceid]=snespos-freespacestart+freespaceextra;
		snespos=(int)0xFFFFFFFF;
	}
	freespaceextra=0;
}

int numopcodes;

bool warnxkas;

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

void initstuff()
{
	if (pass==0)
	{
		for (int i=0;i<256;i++)
		{
			freespaceleak[i]=true;
			freespaceorgpos[i]=-2;
			freespaceorglen[i]=-1;
			freespacebyte[i] = 0x00;
		}
	}
	arch=arch_65816;
	mapper=lorom;
	mapper_set = false;
	reallycalledmacros=0;
	calledmacros=0;
	macrorecursion=0;
	repeatnext=1;
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
	bytes=0;
	memset(fillbyte, 0, sizeof(fillbyte));
	memset(padbyte, 0, sizeof(padbyte));
	snespos=(int)0xFFFFFFFF;
	realsnespos= (int)0xFFFFFFFF;
	startpos= (int)0xFFFFFFFF;
	realstartpos= (int)0xFFFFFFFF;
	//fastrom=false;
	freespaceidnext=1;
	freespaceid=1;
	freespaceextra=0;
	numopcodes=0;
	specifiedasarver = false;
	incsrcdepth = 0;
	
	optimizeforbank = -1;
	optimize_dp = optimize_dp_flag::NONE;
	dp_base = 0;
	optimize_address = optimize_address_flag::DEFAULT;

	in_struct = false;
	in_sub_struct = false;

	math_pri=false;
	math_round=true;

	if (arch==arch_65816) asinit_65816();
	if (arch==arch_spc700) asinit_spc700();
	if (arch==arch_spc700_inline) asinit_spc700();
	if (arch==arch_superfx) asinit_superfx();

	warnxkas=false;
	emulatexkas=false;
	disable_bank_cross_errors = false;
	nested_namespaces = false;

	thisfilename = "";

	includeonce.reset();

	extern AddressToLineMapping addressToLineMapping;
	addressToLineMapping.reset();

	push_warnings(false);

	initmathcore();
}


//void nerf(const string& left, string& right){puts(S left+" = "+right);}

void finishpass()
{
	verify_warnings();
	pull_warnings(false);

//defines.traverse(nerf);
	if (in_struct || in_sub_struct) asar_throw_error(pass, error_type_null, error_id_struct_without_endstruct);
	else if (pushpcnum && pass == 0) asar_throw_error(pass, error_type_null, error_id_pushpc_without_pullpc);
	freespaceend();
	if (arch==arch_65816) asend_65816();
	if (arch==arch_spc700) asend_spc700();
	if (arch==arch_spc700_inline) asend_spc700();
	if (arch==arch_superfx) asend_superfx();

	deinitmathcore();
}

static bool addlabel(const char * label, int pos=-1)
{
	if (!label[0] || label[0]==':') return false;//colons are reserved for special labels

	const char* posneglabel = label;
	string posnegname = posneglabelname(&posneglabel, true);

	if (posnegname.length() > 0)
	{
		if (*posneglabel != '\0' && *posneglabel != ':') asar_throw_error(0, error_type_block, error_id_broken_label_definition);
		setlabel(posnegname, pos);
		return true;
	}
	if (label[strlen(label)-1]==':' || label[0]=='.' || label[0]=='?' || label[0] == '#')
	{
		if (!label[1]) return false;

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
		if (label[0]) asar_throw_error(0, error_type_block, error_id_broken_label_definition);
		if (ns) name=S ns+name;
		setlabel(name, pos);
		return true;
	}
	return false;
}

static autoarray<bool> elsestatus;
int numtrue=0;//if 1 -> increase both
int numif = 0;  //if 0 or inside if 0 -> increase only numif

autoarray<whiletracker> whilestatus;

static int freespaceuse=0;


static void push_pc()
{
	pushpc[pushpcnum].arch=arch;
	pushpc[pushpcnum].snespos=snespos;
	pushpc[pushpcnum].snesstart=startpos;
	pushpc[pushpcnum].snesposreal=realsnespos;
	pushpc[pushpcnum].snesstartreal=realstartpos;
	pushpc[pushpcnum].freeid=freespaceid;
	pushpc[pushpcnum].freeex=freespaceextra;
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
	freespaceextra=pushpc[pushpcnum].freeex;
	freespacestart=pushpc[pushpcnum].freest;
}


void assembleblock(const char * block)
{
	string tmp=block;
	int numwords;
	char ** word = qsplit(tmp.temp_raw(), " ", &numwords);
	string resolved;
	// when writing out the data for the addrToLine mapping,
	// we want to write out the snespos we had before writing opcodes
	int addrToLinePos = realsnespos & 0xFFFFFF;

#define is(test) (!stricmpwithlower(word[0], test))
#define is0(test) (numwords==1 && !stricmpwithlower(word[0], test))
#define is1(test) (numwords==2 && !stricmpwithlower(word[0], test))
#define is2(test) (numwords==3 && !stricmpwithlower(word[0], test))
#define is3(test) (numwords==4 && !stricmpwithlower(word[0], test))
#define par word[1]

	// RPG Hacker: Hack to fix the bug where defines in elseifs would never get resolved
	// This really seems like the only possible place for the fix
	if (is("elseif") && numtrue+1==numif)
	{
		resolvedefines(resolved, block);
		word = qsplit(resolved.temp_raw(), " ", &numwords);
	}

	autoptr<char**> wordcopy=word;
	while (numif==numtrue && word[0] && (!word[1] || strcmp(word[1], "=") != 0) && addlabel(word[0]))
	{
		word++;
		numwords--;
	}
	if (!word[0] || !word[0][0]) return;
	if (numif==numtrue && word[0][0]=='%')
	{
		if (!macrorecursion)
		{
			callerfilename=thisfilename;
			callerline=thisline;
		}
		callmacro(strchr(block, '%')+1);
		if (!macrorecursion)
		{
			callerfilename= nullptr;
			callerline=-1;
		}
		return;
	}
	if (is("if") || is("elseif") || is("assert") || is("while"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		const char * errmsg= nullptr;
		whiletracker wstatus;		
		wstatus.startline = thisline;
		wstatus.iswhile = false;
		wstatus.cond = false;
		if (is("while")) wstatus.iswhile = true;
		whiletracker& addedwstatus = (whilestatus[numif] = wstatus);
		if (is("assert"))
		{
			char * rawerrmsg=strqrchr(word[numwords-1], ',');
			if (rawerrmsg)
			{
				*rawerrmsg=0;
				errmsg= safedequote(rawerrmsg+1);
			}
		}
		if (numtrue!=numif && !(is("elseif") && numtrue+1==numif))
		{
			if ((is("if") || is("while")) && !moreonline) numif++;
			return;
		}
		if ((is("if") || is("while")) && !moreonline) numif++;
		bool cond;

		bool isassert = is("assert");

		char ** nextword=word+1;
		char * condstr= nullptr;
		while (true)
		{
			if (!nextword[0]) asar_throw_error(0, error_type_block, error_id_broken_conditional, word[0]);
			bool thiscond = false;
			if (!nextword[1] || !strcmp(nextword[1], "&&") || !strcmp(nextword[1], "||"))
			{
				if (nextword[0][0] == '!')
				{
					asar_throw_warning(0, warning_id_if_not_condition_deprecated);
					double val = getnumdouble(nextword[0]+1);
					if (foundlabel && !isassert) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
					thiscond = !(val > 0);
				}
				else
				{
					double val = getnumdouble(nextword[0]);
					if (foundlabel && !isassert) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
					thiscond = (val > 0);
				}

				if (condstr && nextword[1])
				{
					if (strcmp(condstr, nextword[1])) asar_throw_error(1, error_type_block, error_id_invalid_condition);
				}
				else condstr=nextword[1];
				nextword+=2;
			}
			else
			{
				if (!nextword[2]) asar_throw_error(0, error_type_block, error_id_broken_conditional, word[0]);
				double par1=getnumdouble(nextword[0]);
				if (foundlabel && !isassert) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
				double par2=getnumdouble(nextword[2]);
				if (foundlabel && !isassert) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
				if(0);
				else if (!strcmp(nextword[1], ">"))  thiscond=(par1>par2);
				else if (!strcmp(nextword[1], "<"))  thiscond=(par1<par2);
				else if (!strcmp(nextword[1], ">=")) thiscond=(par1>=par2);
				else if (!strcmp(nextword[1], "<=")) thiscond=(par1<=par2);
				else if (!strcmp(nextword[1], "="))  thiscond=(par1==par2);
				else if (!strcmp(nextword[1], "==")) thiscond=(par1==par2);
				else if (!strcmp(nextword[1], "!=")) thiscond=(par1!=par2);
				//else if (!strcmp(nextword[1], "<>")) thiscond=(par1!=par2);
				else asar_throw_error(0, error_type_block, error_id_broken_conditional, word[0]);

				if (condstr && nextword[3])
				{
					if (strcmp(condstr, nextword[3])) asar_throw_error(1, error_type_block, error_id_invalid_condition);
				}
				else condstr=nextword[3];
				nextword+=4;
			}
			if (condstr)
			{
				if (!strcmp(condstr, "&&")) { if(thiscond) continue; }
				else if (!strcmp(condstr, "||")) { if(!thiscond) continue; }
				else asar_throw_error(0, error_type_block, error_id_broken_conditional, word[0]);
			}
			cond=thiscond;
			break;
		}
		//if (numwords==4)
		//{
		//	int par1=getnum(word[1]);
		//	if (foundlabel) error(1, S"Label in "+lower(word[0])+" command");
		//	int par2=getnum(word[3]);
		//	if (foundlabel) error(1, S"Label in "+lower(word[0])+" command");
		//	if(0);
		//	else if (!strcmp(word[2], ">"))  cond=(par1>par2);
		//	else if (!strcmp(word[2], "<"))  cond=(par1<par2);
		//	else if (!strcmp(word[2], ">=")) cond=(par1>=par2);
		//	else if (!strcmp(word[2], "<=")) cond=(par1<=par2);
		//	else if (!strcmp(word[2], "="))  cond=(par1==par2);
		//	else if (!strcmp(word[2], "==")) cond=(par1==par2);
		//	else if (!strcmp(word[2], "!=")) cond=(par1!=par2);
		//	//else if (!stricmp(word[2], "<>")) cond=(par1!=par2);
		//	else error(0, S"Broken "+lower(word[0])+" command");
		//}
		//else if (*par=='!')
		//{
		//	int val=getnum(par+1);
		//	if (foundlabel) error(1, "Label in if or assert command");
		//	cond=!(val>0);
		//}
		//else
		//{
		//	int val=getnum(par);
		//	if (foundlabel) error(1, "Label in if or assert command");
		//	cond=(val>0);
		//}

		if (is("if") || is("while"))
		{
			if(0);
			else if (cond && moreonline) {}
			else if (cond && !moreonline)
			{
				numtrue++;
				elsestatus[numif]=true;
			}
			else if (!cond && moreonline) moreonline=false;
			else if (!cond && !moreonline)
			{
				elsestatus[numif]=false;
			}
			addedwstatus.cond = cond;
		}
		else if (is("elseif"))
		{
			if (!numif) asar_throw_error(1, error_type_block, error_id_misplaced_elseif);
			if (whilestatus[numif - 1].iswhile) asar_throw_error(1, error_type_block, error_id_elseif_in_while);
			if (moreonline) asar_throw_error(1, error_type_block, error_id_elseif_in_singleline);
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
	else if (is0("endif"))
	{
		if (!numif) asar_throw_error(1, error_type_block, error_id_misplaced_endif);
		if (numif==numtrue) numtrue--;
		numif--;
	}
	else if (is0("else"))
	{
		if (!numif) asar_throw_error(1, error_type_block, error_id_misplaced_else);
		if (whilestatus[numif - 1].iswhile) asar_throw_error(1, error_type_block, error_id_else_in_while_loop);
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
			extern AddressToLineMapping addressToLineMapping;
			addressToLineMapping.includeMapping(thisfilename.data(), thisline + 1, addrToLinePos);
		}
		numopcodes++;
	}
	else if (is1("undef"))
	{
		string def;
		// RPG Hacker: Not sure if we should allow this?
		// Currently, the manual states that we should not include the
		// exclamation mark, and I believe that this is for the better
		// because I can see this leading to ambiguities or causing
		// problems. If we add this back in, we should definitely
		// also added it to the defined() function for consistency, though.
		// Well, actually I just check and we can't support this in
		// defined() (the defined is already replaced at that point), so
		// I think we should not support it here, either.
		/*if (*par == '!') def = S safedequote(par) + 1;
		else*/ def = S safedequote(par);

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
		asar_throw_error(0, error_type_block, error_id_error_command, (string(": ") + safedequote(par)).data());
	}
	else if (is1("warn"))
	{
		asar_throw_warning(2, warning_id_warn_command, (string(": ") + safedequote(par)).data());
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
				asar_throw_error(0, error_type_null, error_id_invalid_warning_id, "warnings enable", (int)(warning_id_start + 1), (int)(warning_id_end - 1));
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
				asar_throw_error(0, error_type_null, error_id_invalid_warning_id, "warnings disable", (int)(warning_id_start + 1), (int)(warning_id_end - 1));
			}
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
			//expected_title = itrim(expected_title.data(), " ", " ", true); // remove trailing spaces
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
				//actual_display_title = itrim(actual_display_title.data(), " ", " ", true); // remove trailing spaces
				//actual_title = itrim(actual_title.data(), " ", " ", true); // remove trailing spaces
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
			bool val = false;
			if (0);
			else if (!stricmp(word[2], "on")) val = true;
			else if (!stricmp(word[2], "off")) val = false;
			else asar_throw_error(0, error_type_block, error_id_invalid_check);

			disable_bank_cross_errors = !val;
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
		if (emulatexkas) asar_throw_error(0, error_type_block, error_id_xkas_asar_conflict);
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
		autoptr<char**> vers=split(par, ".");
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
		specifiedasarver = true;
	}
	else if (is0("xkas"))
	{
		if (!asarverallowed) asar_throw_error(0, error_type_block, error_id_start_of_file);
		if (incsrcdepth != 1 && !emulatexkas) asar_throw_error(0, error_type_block, error_id_command_in_non_root_file);
		if (specifiedasarver) asar_throw_error(0, error_type_block, error_id_xkas_asar_conflict);
		asar_throw_warning(0, warning_id_xkas_deprecated);
		emulatexkas=true;
		optimizeforbank=0x100;
		if(!force_checksum_fix)
			checksum_fix_enabled = false;
		sublabels[0]=":xkasdefault:";
	}
	else if (is0("include") || is1("includefrom"))
	{
		if (!asarverallowed) asar_throw_error(0, error_type_block, error_id_start_of_file);
		if (istoplevel)
		{
			if (par) asar_throw_error(pass, error_type_fatal, error_id_cant_be_main_file, (string("The main file is '") + S par + S "'.").data());
			else asar_throw_error(pass, error_type_fatal, error_id_cant_be_main_file, "");
		}
	}
	else if (is0("includeonce"))
	{
		if (!file_included_once(thisfilename))
		{
			includeonce.append(thisfilename);
		}
	}
	else if (is1("db") || is1("dw") || is1("dl") || is1("dd"))
	{
		int len = 0;
		if (!confirmqpar(par)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
		if (is1("db")) len=1;
		if (is1("dw")) len=2;
		if (is1("dl")) len=3;
		if (is1("dd")) len=4;
		autoptr<char**> pars=qpsplit(par, ",");
		for (int i=0;pars[i];i++)
		{
			if (pars[i][0]=='"')
			{
				if (!strcmp(pars[i],"\"STAR\"") && !emulatexkas)
					asar_throw_warning(0, warning_id_xkas_patch);
				for (char * str=const_cast<char*>(safedequote(pars[i]));*str;str++)
				{
					if (len==1) write1(table.table[(size_t)*str]);
					if (len==2) write2(table.table[(size_t)*str]);
					if (len==3) write3(table.table[(size_t)*str]);
					if (len==4) write4(table.table[(size_t)*str]);
				}
			}
			else
			{
				const char * math=pars[i];
				if (math[0]=='#') math++;
				unsigned int num=(pass!=0)?getnum(math):0;
				if (len == 1) write1(num);
				if (len == 2) write2(num);
				if (len == 3) write3(num);
				if (len == 4) write4(num);
			}
		}
	}
	else if (numwords==3 && !stricmp(word[1], "="))
	{
		if (word[0][0]=='\'' && word[0][1] && word[0][2]=='\'' && word[0][3]=='\0')
		{
			table.table[(unsigned char)word[0][1]]=getnum(word[2]);
			return;
		}
		// randomdude999: int cast b/c i'm too lazy to also mess with making setlabel()
		// unsigned, besides it wouldn't matter anyways.
		int num=(int)getnum(word[2]);
		if (foundlabel) asar_throw_error(0, error_type_block, error_id_label_cross_assignment);

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
			completename += S":macro_" + dec(calledmacros) + "_";
		}

		completename += newlabelname;

		setlabel(completename, num);
	}
	else if (is1("org"))
	{
		freespaceend();
		unsigned int num=getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_org_label_forward);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6(num).data());
		if ((mapper==lorom || mapper==exlorom) && (num&0x408000)==0x400000 && (num&0x700000)!=0x700000) asar_throw_warning(0, warning_id_set_middle_byte);
		//if (fastrom) num|=0x800000;
		snespos=(int)num;
		realsnespos=(int)num;
		startpos=(int)num;
		realstartpos=(int)num;
	}
#define ret_error(errid) { asar_throw_error(0, error_type_block, errid); return; }
#define ret_error_params(errid, ...) { asar_throw_error(0, error_type_block, errid, __VA_ARGS__); return; }
	else if (is("struct"))
	{
		if (in_struct || in_sub_struct) ret_error(error_id_nested_struct);
		if (numwords < 2) ret_error(error_id_missing_struct_params);
		if (numwords > 4) ret_error(error_id_too_many_struct_params);
		if (!confirmname(word[1])) ret_error(error_id_invalid_struct_name);

		if (structs.exists(word[1]) && pass == 0) ret_error_params(error_id_struct_redefined, word[1]);

		old_snespos = snespos;
		old_startpos = startpos;
		old_optimizeforbank = optimizeforbank;
		unsigned int base = 0;
		if (numwords == 3)
		{
			base = getnum(word[2]);
		}

		bool old_in_struct = in_struct;
		bool old_in_sub_struct = in_sub_struct;
		in_struct = numwords == 2 || numwords == 3;
		in_sub_struct = numwords == 4;

#define ret_error_cleanup(errid) { in_struct = old_in_struct; in_sub_struct = old_in_sub_struct; asar_throw_error(0, error_type_block, errid); return; }
#define ret_error_params_cleanup(errid, ...) { in_struct = old_in_struct; in_sub_struct = old_in_sub_struct; asar_throw_error(0, error_type_block, errid, __VA_ARGS__); return; }

		if (numwords == 3)
		{
			if (base&~0xFFFFFF) ret_error_params_cleanup(error_id_snes_address_out_of_bounds, hex6((unsigned int)base).data());
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

			struct_parent = tmp_struct_parent;
			snespos = structure.base_end;
			startpos = structure.base_end;
		}

		push_pc();

		optimizeforbank = -1;

		struct_name = word[1];
		struct_base = snespos;
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
	}
#undef ret_error
	else if (is1("base"))
	{
		if (!stricmp(par, "off"))
		{
			snespos=realsnespos;
			startpos=realstartpos;
			return;
		}
		unsigned int num=getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_base_label_invalid);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).data());
		snespos=(int)num;
		startpos=(int)num;
		optimizeforbank=-1;
	}
	else if (is1("dpbase"))		
	{		
		unsigned int num=(int)getnum(par);		
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_base_label_invalid);		
		if (num&~0xFF00) asar_throw_error(1, error_type_block, error_id_bad_dp_base, hex6((unsigned int)num).data());		
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
		if (num&~0x0000FF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).data());
		optimizeforbank=(int)num;
	}
	else if (is("freespace") || is("freecode") || is("freedata"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		string parstr;
		if (numwords==1) parstr="\n";//crappy hack: impossible character to cut out extra commas
		else if (numwords==2) parstr=word[1];
		else asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
		if (is("freecode")) parstr=S"ram,"+parstr;
		if (is("freedata")) parstr=S"noram,"+parstr;
		autoptr<char**> pars=split(parstr.temp_raw(), ",");
		unsigned char fsbyte = 0x00;
		int useram=-1;
		bool fixedpos=false;
		bool align=false;
		bool leakwarn=true;
		for (int i=0;pars[i];i++)
		{
			if (pars[i][0]=='\n') {}
			else if (!stricmp(pars[i], "ram"))
			{
				if (useram!=-1) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				useram=1;
			}
			else if (!stricmp(pars[i], "noram"))
			{
				if (useram!=-1) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
				useram=0;
			}
			else if (!stricmp(pars[i], "static") || !stricmp(pars[i], "fixed"))
			{
				if (!stricmp(pars[i], "fixed")) asar_throw_warning(0, warning_id_fixed_deprecated);
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
			else
			{
				fsbyte = (unsigned char)getnum(pars[i]);
			}
		}
		if (useram==-1) asar_throw_error(0, error_type_block, error_id_invalid_freespace_request);
		if ((mapper == hirom || mapper == exhirom) && useram) asar_throw_error(0, error_type_block, error_id_no_banks_with_ram_mirrors);
		if (mapper == norom) asar_throw_error(0, error_type_block, error_id_no_freespace_norom);
		freespaceend();
		freespaceid=getfreespaceid();
		freespacebyte[freespaceid] = fsbyte;
		if (pass==0) snespos=(freespaceid<<24)|0x8000;
		if (pass==1)
		{
			if (fixedpos && freespaceorgpos[freespaceid]<0)
			{
				freespacepos[freespaceid]=0x008000;
				freespaceleak[freespaceid]=false;//mute some other errors
				asar_throw_error(1, error_type_block, error_id_static_freespace_autoclean);
			}
			if (fixedpos && freespaceorgpos[freespaceid]) freespacepos[freespaceid]=snespos=(freespaceid<<24)|freespaceorgpos[freespaceid];
			else freespacepos[freespaceid]=snespos=(freespaceid<<24)|getsnesfreespace(freespacelen[freespaceid], (useram != 0), true, true, align, freespacebyte[freespaceid]);
		}
		if (pass==2)
		{
			if (fixedpos && freespaceorgpos[freespaceid]==-1) return;//to kill some errors
			snespos=(freespaceid<<24)|freespacepos[freespaceid];
			resizerats(snespos, freespacelen[freespaceid]);
			if (freespaceleak[freespaceid] && leakwarn) asar_throw_warning(2, warning_id_freespace_leaked);
			if (fixedpos && freespaceorgpos[freespaceid]>0 && freespacelen[freespaceid]>freespaceorglen[freespaceid])
				asar_throw_error(2, error_type_block, error_id_static_freespace_growing);
			freespaceuse+=8+freespacelen[freespaceid];
		}
		freespacestatic[freespaceid]=fixedpos;
		if (snespos < 0 && mapper == sa1rom) asar_throw_error(pass, error_type_fatal, error_id_no_freespace_in_mapped_banks, dec(freespacelen[freespaceid]).data());
		if (snespos < 0) asar_throw_error(pass, error_type_fatal, error_id_no_freespace, dec(freespacelen[freespaceid]).data());
		bytes+=8;
		freespacestart=snespos;
		startpos=snespos;
		realstartpos=snespos;
		realsnespos=snespos;
		optimizeforbank=-1;
		ratsmetastate=ratsmeta_allow;
	}
	else if (is1("prot"))
	{
		if (!confirmqpar(par)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
		if (!ratsmetastate) asar_throw_error(2, error_type_block, error_id_prot_not_at_freespace_start);
		if (ratsmetastate==ratsmeta_used) step(-5);
		int num;
		autoptr<char**> pars=qpsplit(par, ",", &num);
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
			int labelnum=(int)labelval(&labeltest);
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.data());
			write3((unsigned int)labelnum);
			if (pass==1) freespaceleak[labelnum >>24]=false;
		}
		write1('S');
		write1('T');
		write1('O');
		write1('P');
		write1(0);
		ratsmetastate=ratsmeta_used;
	}
	else if (is1("autoclean") || is2("autoclean") || is1("autoclear") || is2("autoclear"))
	{
		if (is1("autoclear") || is2("autoclear")) asar_throw_warning(0, warning_id_autoclear_deprecated);
		if (numwords==3)
		{
			if ((unsigned int)snespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_autoclean_in_freespace);
			const char * labeltest = word[2];
			string testlabel = labeltest;
			int num=(int)labelval(&labeltest);
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.data());
			unsigned char targetid=(unsigned char)(num>>24);
			if (pass==1) freespaceleak[targetid]=false;
			num&=0xFFFFFF;
			if (strlen(par)>3 && !stricmp(par+3, ".l")) par[3]=0;
			if (!stricmp(par, "JSL") || !stricmp(par, "JML"))
			{
				int orgpos=read3(snespos+1);
				int ratsloc=freespaceorgpos[targetid];
				int firstbyte = ((!stricmp(par, "JSL")) ? 0x22 : 0x5C);
				int pcpos=snestopc(snespos);
				if (/*pass==1 && */pcpos>=0 && pcpos<romlen_r && romdata_r[pcpos]==firstbyte)
				{
					ratsloc=ratsstart(orgpos)+8;
					freespaceorglen[targetid]=read2(ratsloc-4)+1;
					if (!freespacestatic[targetid] && pass==1) removerats(orgpos, freespacebyte[targetid]);
				}
				else if (ratsloc<0) ratsloc=0;
				write1((unsigned int)firstbyte);
				write3((unsigned int)num);
				if (pass==2)
				{
					int start=ratsstart(num);
					if (start>=num || start<0)
					{
						asar_throw_error(2, error_type_block, error_id_autoclean_label_at_freespace_end);
					}
				}
				//freespaceorglen[targetid]=read2(ratsloc-4)+1;
				freespaceorgpos[targetid]=ratsloc;
			}
			else if (!stricmp(par, "dl"))
			{
				int orgpos=read3(snespos);
				int ratsloc=freespaceorgpos[targetid];
				int start=ratsstart(num);
				if (pass==1 && num>=0)
				{
					ratsloc=ratsstart(orgpos)+8;
					if (!freespacestatic[targetid]) removerats(orgpos, freespacebyte[targetid]);
				}
				else if (!ratsloc) ratsloc=0;
				if ((start==num || start<0) && pass==2)
					asar_throw_error(2, error_type_block, error_id_autoclean_label_at_freespace_end);
				write3((unsigned int)num);
				freespaceorgpos[targetid]=ratsloc;
				freespaceorglen[targetid]=read2(ratsloc-4)+1;
			}
			else asar_throw_error(0, error_type_block, error_id_broken_autoclean);
		}
		else if (pass==0) removerats((int)getnum(word[1]), 0x00);
	}
	else if (is0("pushpc"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		pushpc[pushpcnum].arch=arch;
		pushpc[pushpcnum].snespos=snespos;
		pushpc[pushpcnum].snesstart=startpos;
		pushpc[pushpcnum].snesposreal=realsnespos;
		pushpc[pushpcnum].snesstartreal=realstartpos;
		pushpc[pushpcnum].freeid=freespaceid;
		pushpc[pushpcnum].freeex=freespaceextra;
		pushpc[pushpcnum].freest=freespacestart;
		pushpcnum++;
		snespos=(int)0xFFFFFFFF;
		startpos= (int)0xFFFFFFFF;
		realsnespos= (int)0xFFFFFFFF;
		realstartpos= (int)0xFFFFFFFF;
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
		freespaceextra=pushpc[pushpcnum].freeex;
		freespacestart=pushpc[pushpcnum].freest;
	}
	else if (is0("pushbase"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
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
	else if (is1("namespace") || is2("namespace"))
	{
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
				namespace_list.append(S tmpstr);
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
			ns += S"_";
		}
	}
	else if (is1("warnpc"))
	{
		unsigned int maxpos=getnum(par);
		if ((unsigned int)snespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_warnpc_in_freespace);
		if ((unsigned int)maxpos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_warnpc_broken_param);
		if ((unsigned int)snespos > maxpos) asar_throw_error(0, error_type_block, error_id_warnpc_failed, hex6((unsigned int)snespos).data(), hex6((unsigned int)maxpos).data());
		if (warnxkas && (unsigned int)snespos == maxpos) asar_throw_warning(0, warning_id_xkas_warnpc_relaxed);
		if (emulatexkas && (unsigned int)snespos==maxpos) asar_throw_error(0, error_type_block, error_id_warnpc_failed_equal, hex6((unsigned int)snespos).data(), hex6((unsigned int)maxpos).data());
	}
	else if (is1("rep"))
	{
		int rep = (int)getnum64(par);
		if (foundlabel) asar_throw_error(0, error_type_block, error_id_rep_label);
		if (rep<=1)
		{
			if (emulatexkas)
			{
				if (rep==0) rep=1;
				if (rep<0) rep=0;
				repeatnext=rep;
				return;
			}
			asar_throw_warning(0, warning_id_xkas_style_conditional);
		}
		repeatnext=rep;
	}
#ifdef SANDBOX
	else if (is("incsrc") || is("incbin") || is("table"))
	{
		asar_throw_error(0, error_type_block, error_id_command_disabled);
	}
#endif
	else if (is1("incsrc"))
	{
		checkbankcross();
		string name;
		if (warnxkas && (strchr(thisfilename, '/') || strchr(thisfilename, '\\')))
			asar_throw_warning(0, warning_id_xkas_incsrc_relative);
		if (strchr(par, '\\'))
		{
			if (emulatexkas)
			{
				for (int i=0;par[i];i++)
				{
					if (par[i]=='\\') par[i]='/';//let's just hope nobody finds I could just enable this for everything.
				}
			}
#ifdef _WIN32
			else asar_throw_warning(0, warning_id_cross_platform_path);
#endif
		}
		if (emulatexkas) name= safedequote(par);
		else name=S safedequote(par);
		assemblefile(name, false);
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
			if (strchr(lengths, '"')) asar_throw_error(0, error_type_block, error_id_broken_incbin);
			if(*lengths=='(') {
				char* tmp = strqpchr(lengths, '-');
				if(!tmp || (*(tmp-1)!=')')) asar_throw_error(0, error_type_block, error_id_broken_incbin);
				start = (int)getnum64(string(lengths+1, tmp-1-lengths-1));
				lengths = tmp;
			} else {
				start=(int)strtoul(lengths, &lengths, 16);
			}
			if (*lengths!='-') asar_throw_error(0, error_type_block, error_id_broken_incbin);
			lengths++;
			if(*lengths=='(') {
				char* tmp = strchr(lengths, '\0');
				if(*(tmp-1)!=')') asar_throw_error(0, error_type_block, error_id_broken_incbin);
				end = (int)getnum64(string(lengths+1, tmp-1-lengths-1));
				// no need to check end-of-string here
			} else {
				end=(int)strtoul(lengths, &lengths, 16);
				if (*lengths) asar_throw_error(0, error_type_block, error_id_broken_incbin);
			}
		}
		string name;
		if (warnxkas && (strchr(thisfilename, '/') || strchr(thisfilename, '\\')))
			asar_throw_warning(0, warning_id_xkas_incsrc_relative);
		if (strchr(par, '\\'))
		{
			if (emulatexkas)
			{
				for (int i=0;par[i];i++)
				{
					if (par[i]=='\\') par[i]='/';//let's just hope nobody finds I could just enable this for everything.
				}
			}
#ifdef _WIN32
			else asar_throw_warning(0, warning_id_cross_platform_path);
#endif
		}
		if (emulatexkas) name= safedequote(par);
		else name=S safedequote(par);
		char * data;//I couldn't find a way to get this into an autoptr
		if (!readfile(name, thisfilename, &data, &len)) asar_throw_error(0, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
		autoptr<char*> datacopy=data;
		if (!end) end=len;
		if(start < 0) asar_throw_error(0, error_type_block, error_id_file_offset_out_of_bounds, dec(start).data(), name.data());
		if (end < start || end > len || end < 0) asar_throw_error(0, error_type_block, error_id_file_offset_out_of_bounds, dec(end).data(), name.data());
		if (numwords==4)
		{
			if (!confirmname(word[3]))
			{
				int pos=(int)getnum(word[3]);
				if (foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
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
					freespacepos[foundfreespaceid]=pctosnes(pos)|(/*fastrom?0x800000:*/0x000000)|(foundfreespaceid <<24);
					setlabel(word[3], freespacepos[foundfreespaceid]);
					writeromdata_bytes(pos, 0xFF, end-start);
				}
				if (pass==1)
				{
					getfreespaceid();//nothing to do here, but we want to tick the counter
				}
				if (pass==2)
				{
					int foundfreespaceid =getfreespaceid();
					if (freespaceleak[foundfreespaceid]) asar_throw_warning(2, warning_id_freespace_leaked);
					writeromdata(snestopc(freespacepos[foundfreespaceid]&0xFFFFFF), data+start, end-start);
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
			int alignment = getnum64(word[2]);
			if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			int offset = 0;
			if(numwords==5)
			{
				offset = getnum64(word[4]);
				if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			}
			if(alignment > 0x800000) asar_throw_error(0, error_type_block, error_id_alignment_too_big);
			if(alignment < 1) asar_throw_error(0, error_type_block, error_id_alignment_too_small);
			if(alignment & (alignment-1)) asar_throw_error(0, error_type_block, error_id_invalid_alignment);
			// i just guessed this formula but it seems to work
			amount = (alignment - ((snespos - offset) & (alignment-1))) & (alignment-1);
		}
		else
		{
			amount = (int)getnum64(par);
			if (foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
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
		tablestack.append(table);
	}
	else if (is0("pulltable"))
	{
		if (tablestack.count <= 0) asar_throw_error(0, error_type_block, error_id_pulltable_without_table);
		table=tablestack[tablestack.count-1];
		tablestack.remove(tablestack.count-1);
	}
	else if (is1("table"))
	{
		bool fliporder=false;
		if(0);
		else if (striend(par, ",ltr")) { itrim(par, "", ",ltr"); }
		else if (striend(par, ",rtl")) { itrim(par, "", ",rtl"); fliporder=true; }
		string name=S safedequote(par);
		autoptr<char*> tablecontents=readfile(name, thisfilename);
		if (!tablecontents) asar_throw_error(0, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
		autoptr<char**> tablelines=split(tablecontents, "\n");
		for (int i=0;i<256;i++) table.table[i]=(unsigned int)(((numopcodes+read2(0x00FFDE)+i)*0x26594131)|0x40028020);
			//garbage value so people will notice they're doing it wrong (for bonus points: figure out what 0x26594131 is)
		for (int i=0;tablelines[i];i++)
		{
			string tableline=tablelines[i];
			if (!*tableline) continue;
			if (strlen(tableline) < 4 || strlen(tableline) & 1 || strlen(tableline) > 10) asar_throw_error(0, error_type_block, error_id_invalid_table_file);
			if (!fliporder)
			{
				if (tableline[3]=='x' || tableline[3]=='X') asar_throw_error(0, error_type_block, error_id_invalid_table_file);
				char * end;
				table.table[(unsigned char)tableline[0]]=(unsigned int)strtol(tableline.data()+2, &end, 16);
				if (*end) asar_throw_error(0, error_type_block, error_id_invalid_table_file);
			}
			else
			{
				if (tableline[1]=='x' || tableline[1]=='X') asar_throw_error(0, error_type_block, error_id_invalid_table_file);
				char * eq;
				unsigned int val=(unsigned int)strtol(tableline, &eq, 16);
				if (eq[0]!='=' || eq[2]) asar_throw_error(0, error_type_block, error_id_invalid_table_file);
				table.table[(unsigned char)eq[1]]=val;
			}
		}
	}
	else if (is3("function"))
	{
		//if (!pass)
		//{
			if (stricmp(word[2], "=")) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
			if (!confirmqpar(word[1])) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
			string line=word[1];
			clean(line);
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
		//}
	}
	else if (is1("print"))
	{
		if (!confirmqpar(par)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
		string out;
		autoptr<char**> pars=qpsplit(par, ",");
		for (int i=0;pars[i];i++)
		{
			if(0);
			else if (pars[i][0]=='"') out+= safedequote(pars[i]);
			else if (!stricmp(pars[i], "bytes")) out+=dec(bytes);
			else if (!stricmp(pars[i], "freespaceuse")) out+=dec(freespaceuse);
			else if (!stricmp(pars[i], "pc")) out+=hex6((unsigned int)(snespos&0xFFFFFF));
			else if (!strncasecmp(pars[i], "bin(", strlen("bin(")) ||
			         !strncasecmp(pars[i], "dec(", strlen("dec(")) ||
			         !strncasecmp(pars[i], "hex(", strlen("hex(")) ||
			         !strncasecmp(pars[i], "double(", strlen("double(")))
			{
				char * arg1pos = strchr(pars[i], '(')+1;
				char * endpos = strchr(arg1pos, '\0');
				while (*endpos==' ' || *endpos=='\0') endpos--;
				if (*endpos != ')') asar_throw_error(0, error_type_block, error_id_invalid_print_function_syntax);
				string paramstr = string(arg1pos, (int)(endpos-arg1pos));

				int numargs;
				autoptr<char**> params = qpsplit(paramstr.temp_raw(), ",", &numargs);
				if (numargs > 2) asar_throw_error(0, error_type_block, error_id_wrong_num_parameters);
				int precision = 0;
				bool hasprec = numargs == 2;
				if (hasprec)
				{
					precision = getnum64(params[1]);
					if(precision < 0) precision = 0;
					if(precision > 64) precision = 64;
				}
				*(arg1pos-1) = '\0'; // allows more convenient comparsion functions
				if(!stricmp(pars[i], "bin"))
				{
					// sadly printf doesn't have binary, so let's roll our own
					int64_t value = getnum64(params[0]);
					char buffer[65];
					if(value < 0) {
						out += '-';
						value = -value;
						// decrement precision because we've output one char already
						precision -= 1;
						if(precision<0) precision=0;
					}
					for(int j = 0; j < 64; j++) {
						buffer[63-j] = '0' + ((value & (1ull<<j)) >> j);
					}
					buffer[64] = 0;
					int startidx = 0;
					while(startidx < 64-precision && buffer[startidx] == '0') startidx++;
					if(startidx == 64) startidx--; // always want to print at least one digit
					out += buffer+startidx;
				}
				else if(!stricmp(pars[i], "dec"))
				{
					int64_t value = getnum64(params[0]);
					char buffer[65];
					snprintf(buffer, 65, "%0*" PRId64, precision, value);
					out += buffer;
				}
				else if(!stricmp(pars[i], "hex"))
				{
					int64_t value = getnum64(params[0]);
					char buffer[65];
					snprintf(buffer, 65, "%0*" PRIX64, precision, value);
					out += buffer;
				}
				else if(!stricmp(pars[i], "double"))
				{
					if(!hasprec) precision=5;
					out += ftostrvar(getnumdouble(params[0]), precision);
				}
			}
			else asar_throw_error(2, error_type_block, error_id_unknown_variable);
		}
		if (pass!=2) return;
		print(out);
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
		if ((unsigned int)realsnespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_pad_in_freespace);
		int num=(int)getnum(par);
		if ((unsigned int)num & 0xFF000000) asar_throw_error(0, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex6((unsigned int)num).data());
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
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		if (!stricmp(par, "65816")) { arch=arch_65816; return; }
		if (!stricmp(par, "spc700")) { arch=arch_spc700; return; }
		if (!stricmp(par, "spc700-inline")) { arch=arch_spc700_inline; return; }
		if (!stricmp(par, "spc700-raw")) {
			arch=arch_spc700;
			mapper=norom;
			mapper_set = false;
			if(!force_checksum_fix)
				checksum_fix_enabled = false;
			return;
		}
		if (!stricmp(par, "superfx")) { arch=arch_superfx; return; }
	}
	else if (is2("math"))
	{
		bool val = false;
		if(0);
		else if (!stricmp(word[2], "on")) val=true;
		else if (!stricmp(word[2], "off")) val=false;
		else asar_throw_error(0, error_type_block, error_id_invalid_math);
		if(0);
		else if (!stricmp(word[1], "pri")) math_pri=val;
		else if (!stricmp(word[1], "round")) math_round=val;
		else asar_throw_error(0, error_type_block, error_id_invalid_math);
	}
	else if (is2("warn"))
	{
		bool val = false;
		if(0);
		else if (!stricmp(word[2], "on")) val=true;
		else if (!stricmp(word[2], "off")) val=false;
		else asar_throw_error(0, error_type_block, error_id_invalid_warn);
		if(0);
		else if (!stricmp(word[1], "xkas")) {
			asar_throw_warning(0, warning_id_xkas_deprecated);
			warnxkas=val;
		}
		else asar_throw_error(0, error_type_block, error_id_invalid_warn);
	}
	else if (is0("fastrom"))
	{
		//removed due to causing more trouble than it's worth.
		//if (emulatexkas) warn0("Convert the patch to native Asar format instead of making an Asar-only xkas patch.");
		//if (mapper==lorom || mapper==hirom) fastrom=true;
		//else error(0, "Can't use fastrom in this mapper.");
	}
	else if (is0("{") || is0("}")) {}
	else
	{
		if (thisline == lastspecialline)
		{
			asar_throw_warning(0, warning_id_unrecognized_special_command);
		}
		else
		{
			asar_throw_error(1, error_type_block, error_id_unknown_command);
		}
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
			autoptr<char**> pars=qpsplit(par, ",", &len);
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
	else if (is0("header"))
	{
		//headers are detected elsewhere; ignoring for familiarity
	}
	else return false;
	
	if(!mapper_set){
		mapper_set = true;
	}else if(previous_mapper != mapper){
		asar_throw_warning(1, warning_id_mapper_already_set);
	}
	return true;
}
