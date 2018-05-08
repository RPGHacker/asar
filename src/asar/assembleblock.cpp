#include "asar.h"
#include "libstr.h"
#include "libsmw.h"
#include "assocarr.h"
#include "autoarray.h"
#include "warnings.h"
#include "platform/file-helpers.h"

int arch=arch_65816;

extern int pass;

int snespos;
int realsnespos;
int startpos;
int realstartpos;

bool emulatexkas;
bool specifiedasarver = false;

extern int optimizeforbank;
extern bool ignoretitleerrors;

int old_snespos;
int old_startpos;
int old_optimizeforbank;
int struct_base;
string struct_name;
string struct_parent;
bool in_struct = false;
bool in_sub_struct = false;

assocarr<snes_struct> structs;

static bool movinglabelspossible = false;

bool disable_bank_cross_errors = false;

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

int fixsnespos(int inaddr, int step)
{
	// RPG Hacker: No idea how reliable this is.
	// Might not work with some of the more exotic mappers.
	return pctosnes(snestopc(inaddr) + step);
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
			asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex6((unsigned int)realsnespos).str);
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

void asinit_65816();
bool asblock_65816(char** word, int numwords);
void asend_65816();
void asinit_spc700();
bool asblock_spc700(char** word, int numwords);
void asend_spc700();
void asinit_superfx();
bool asblock_superfx(char** word, int numwords);
void asend_superfx();

bool asblock_pick(char** word, int numwords)
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
#define dequote safedequote

extern char romtitle[30];
extern bool stdlib;

extern bool foundlabel;
extern bool forwardlabel;
int getlen(const char * str);
unsigned int getnum(const char * str);

void assemblefile(const char * filename, bool toplevel);
extern string thisfilename;
extern int thisline;
extern int lastspecialline;

void createuserfunc(const char * name, const char * arguments, const char * content);

bool commandlineflag(const char * cmd, const char * val);

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
	c=(char)tolower(c);
	if (c=='b') return 1;
	if (c=='w') return 2;
	if (c=='l') return 3;
	if (c=='d') return 4;
	asar_throw_error(0, error_type_block, error_id_invalid_opcode_length);
	return -1;
}

assocarr<unsigned int> labels;
autoarray<int> poslabels;
autoarray<int> neglabels;

autoarray<int>* macroposlabels;
autoarray<int>* macroneglabels;

autoarray<string> sublabels;
autoarray<string>* macrosublabels;

// randomdude999: ns is still the string to prefix to all labels, it's calculated whenever namespace_list is changed
string ns;
autoarray<string> namespace_list;

//bool fastrom=false;

autoarray<string> includeonce;

void startmacro(const char * line);
void tomacro(const char * line);
void endmacro(bool insert);
void callmacro(const char * data);

extern int reallycalledmacros;
extern int calledmacros;
extern int macrorecursion;
extern int incsrcdepth;

extern int repeatnext;

bool confirmname(const char * name)
{
	if (!name[0]) return false;
	if (isdigit(name[0])) return false;
	for (int i=0;name[i];i++)
	{
		if (!isalnum(name[i]) && name[i]!='_') return false;
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

string labelname(const char ** rawname, bool define=false)
{
#define rawname (*rawname)
	bool ismacro = (rawname[0] == '?');

	if (ismacro)
	{
		rawname++;
	}

	bool issublabel = false;

	string name;
	int i=-1;

	autoarray<string>* sublabellist = &sublabels;
	if (ismacro)
	{
		sublabellist = macrosublabels;
	}

	if (isdigit(*rawname)) asar_throw_error(0, error_type_block, error_id_invalid_label_name);
	if (*rawname==':')
	{
		rawname++;
		name=":";
	}
	else if (!in_struct && !in_sub_struct)
	{
		for (i=0;(*rawname=='.');i++) rawname++;
		if (!isualnum(*rawname)) asar_throw_error(0, error_type_block, error_id_invalid_label_name);
		if (emulatexkas && i>1) asar_throw_warning(0, warning_id_convert_to_asar);
		if (i)
		{
			if (!(*sublabellist)[i - 1]) asar_throw_error(0, error_type_block, error_id_label_missing_parent);
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
		rawname++;
	}

	if (!isualnum(*rawname)) asar_throw_error(0, error_type_block, error_id_invalid_label_name);

	while (isualnum(*rawname) || *rawname == '.' || *rawname == '[')
	{
		if(!in_struct && !in_sub_struct && *rawname == '[')
		{
			bool invalid = true;
			while (isprint(*rawname))
			{
				if (*(rawname++) == ']')
				{
					invalid = false;
					break;
				}
			}
			if (invalid)
			{
				asar_throw_error(0, error_type_block, error_id_invalid_label_missing_closer);
			}
		}
		else if (*rawname == '{')
		{
			asar_throw_error(0, error_type_block, error_id_array_invalid_inside_structs);
		}

		name+=*(rawname++);
	}
	if (define && i>=0)
	{
		(*sublabellist).reset(i);
		(*sublabellist)[i]=name;
	}
	return name;
#undef rawname
}

string labelname(char ** rawname, bool define=false)
{
	return labelname((const char**)rawname, define);
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
			asar_throw_error(1, error_type_block, error_id_label_not_found, name.str);
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
	labelvalcore((const char**)rawname, &rval, define, true);
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
	return labelvalcore((const char**)rawname, rval, define, false);
}

bool labelval(string name, unsigned int * rval, bool define)
{
	const char * str=name;
	return labelvalcore(&str, rval, define, false);
}

void setlabel(string name, int loc=-1)
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
			asar_throw_error(0, error_type_block, error_id_label_redefined, name.str);
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
autoarray<chartabledata> tablestack;

int freespacepos[256];
int freespacelen[256];
int freespaceidnext;
int freespaceid;
int freespacestart;
int freespaceextra;

bool freespaceleak[256];
string freespacefile[256];
int freespaceline[256];

int freespaceorgpos[256];
int freespaceorglen[256];
bool freespacestatic[256];
unsigned char freespacebyte[256];

void cleartable()
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
autoarray<pushable> pushpc;
int pushpcnum;

autoarray<int> basestack;
int basestacknum;

unsigned char fillbyte[12];
unsigned char padbyte[12];

bool sandbox=false;

bool nested_namespaces = false;

int getfreespaceid()
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

void freespaceend()
{
	checkbankcross();
	if ((snespos&0x7F000000) && (snespos&0x80000000)==0)
	{
		freespacelen[freespaceid]=snespos-freespacestart+freespaceextra;
		snespos=0xFFFFFFFF;
	}
	freespaceextra=0;
}

int savedoptions[16];

int numopcodes;

extern bool math_pri;
extern bool math_round;

bool warnxkas;

extern assocarr<string> defines;
extern assocarr<string> clidefines;
extern assocarr<string> builtindefines;

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

void initstuff()
{
	//int numsavedoptions=0;
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
	snespos=0xFFFFFFFF;
	realsnespos=0xFFFFFFFF;
	startpos=0xFFFFFFFF;
	realstartpos=0xFFFFFFFF;
	//fastrom=false;
	freespaceidnext=1;
	freespaceid=1;
	freespaceextra=0;
	numopcodes=0;

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
}


//void nerf(const string& left, string& right){puts(S left+" = "+right);}

void finishpass()
{
//defines.traverse(nerf);
	if (in_struct || in_sub_struct) asar_throw_error(pass, error_type_null, error_id_struct_without_endstruct);
	else if (pushpcnum && pass == 0) asar_throw_error(pass, error_type_null, error_id_pushpc_without_pullpc);
	freespaceend();
	if (arch==arch_65816) asend_65816();
	if (arch==arch_spc700) asend_spc700();
	if (arch==arch_spc700_inline) asend_spc700();
	if (arch==arch_superfx) asend_superfx();
}

bool addlabel(const char * label, int pos=-1)
{
	if (!label[0] || label[0]==':') return false;//colons are reserved for special labels

	const char* posneglabel = label;
	string posnegname = posneglabelname(&posneglabel, true);

	if (posnegname.truelen() > 0)
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

autoarray<bool> elsestatus;
int numtrue=0;//if 1 -> increase both
int numif = 0;  //if 0 or inside if 0 -> increase only numif
autoarray<whiletracker> whilestatus;

extern bool asarverallowed;
extern bool istoplevel;

extern bool moreonline;

void print(const char * str);

extern bool checksum;

extern const char * callerfilename;
extern int callerline;
extern int macrorecursion;

int freespaceuse=0;


void push_pc()
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

void pop_pc()
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


void resolvedefines(string& out, const char * start);
bool file_included_once(const char* file);

void assembleblock(const char * block)
{
	string tmp=block;
	int numwords;
	char ** word = qsplit(tmp.str, " ", &numwords);
	string resolved;

#define is(test) (!stricmp(word[0], test))
#define is0(test) (!stricmp(word[0], test) && numwords==1)
#define is1(test) (!stricmp(word[0], test) && numwords==2)
#define is2(test) (!stricmp(word[0], test) && numwords==3)
#define is3(test) (!stricmp(word[0], test) && numwords==4)
#define is4(test) (!stricmp(word[0], test) && numwords==5)
#define is5(test) (!stricmp(word[0], test) && numwords==6)
#define par word[1]

	// RPG Hacker: Hack to fix the bug where defines in elseifs would never get resolved
	// This really seems like the only possible place for the fix
	if (is("elseif") && numtrue+1==numif)
	{
		resolvedefines(resolved, block);
		word = qsplit(resolved.str, " ", &numwords);
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
			callerfilename=NULL;
			callerline=-1;
		}
		return;
	}
	if (is1("undef"))
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
		/*if (*par == '!') def = S dequote(par) + 1;
		else*/ def = S dequote(par);

		if (defines.exists(def))
		{
			defines.remove(def);
		}
		else
		{
			asar_throw_error(0, error_type_block, error_id_define_not_found, def.str);
		}
	}
	else if (is("if") || is("elseif") || is("assert") || is("while"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		const char * errmsg=NULL;
		whiletracker wstatus;		
		wstatus.startline = thisline;
		wstatus.iswhile = false;
		wstatus.cond = false;
		if (is("while")) wstatus.iswhile = true;
		whiletracker& addedwstatus = (whilestatus[numif] = wstatus);
		if (is("assert"))
		{
			char * rawerrmsg=strchr(word[numwords-1], ',');
			if (rawerrmsg)
			{
				*rawerrmsg=0;
				errmsg=dequote(rawerrmsg+1);
			}
		}
		if (numtrue!=numif && !(is("elseif") && numtrue+1==numif))
		{
			if ((is("if") || is("while")) && !moreonline) numif++;
			return;
		}
		if ((is("if") || is("while")) && !moreonline) numif++;
		bool cond;

		char ** nextword=word+1;
		char * condstr=NULL;
		while (true)
		{
			if (!nextword[0]) asar_throw_error(0, error_type_block, error_id_broken_conditional, word[0]);
			bool thiscond = false;
			if (!nextword[1] || !strcmp(nextword[1], "&&") || !strcmp(nextword[1], "||"))
			{
				unsigned int val = getnum(nextword[0]);
				if (foundlabel) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
				thiscond = (val > 0);

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
				unsigned int par1=getnum(nextword[0]);
				if (foundlabel) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
				unsigned int par2=getnum(nextword[2]);
				if (foundlabel) asar_throw_error(1, error_type_block, error_id_label_in_conditional, word[0]);
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
				if (!strcmp(condstr, "&&") && thiscond) continue;
				if (!strcmp(condstr, "||") && !thiscond) continue;
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
		else if (!cond)
		{
			if (errmsg) asar_throw_error(1, error_type_block, error_id_assertion_failed, (string(": ") + errmsg).str);
			else asar_throw_error(1, error_type_block, error_id_assertion_failed, ".");
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
	else if (asblock_pick(word, numwords)) { numopcodes++; }
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
		asar_throw_error(0, error_type_block, error_id_error_command, (string(": ") + dequote(par)).str);
	}
	else if (is1("warn"))
	{
		asar_throw_warning(2, warning_id_warn_command, (string(": ") + dequote(par)).str);
	}
	else if (is1("expecttitle"))
	{
		// RPG Hacker: Removed trimming for now - I think requiring an exact match is probably
		// better here (not sure, though, it's worth discussing)
		string expected_title = dequote(par);
		// randomdude999: this also removes leading spaces because itrim gets stuck in an endless
		// loop when multi is true and one argument is empty
		//expected_title = itrim(expected_title.str, " ", " ", true); // remove trailing spaces
		// in hirom the rom needs to be an entire bank for it to have a title, other modes only need 0x8000 bytes
		if (romlen < ((mapper==hirom || mapper==exhirom) ? 0x10000 : 0x8000)) // too short
		{
			if (!ignoretitleerrors) // title errors shouldn't be ignored
				asar_throw_error(0, error_type_block, error_id_rom_too_short, expected_title.str);
			else // title errors should be ignored, throw a warning anyways
				asar_throw_warning(0, warning_id_rom_too_short, expected_title.str);
		}
		else {
			string actual_title;
			string actual_display_title;
			for (int i=0;i<21;i++)
			{
				unsigned char c = romdata[snestopc(0x00FFC0+i)];
				actual_title += (char)c;
				// the replacements are from interface-cli.cpp
				if (c==7) c=14;
				if (c==8) c=27;
				if (c==9) c=26;
				if (c=='\r') c=17;
				if (c=='\n') c=25;
				if (c=='\0') c=155;
				actual_display_title += (char)c;
			}
			//actual_display_title = itrim(actual_display_title.str, " ", " ", true); // remove trailing spaces
			//actual_title = itrim(actual_title.str, " ", " ", true); // remove trailing spaces
			if (strncmp(expected_title, actual_title, 21) != 0)
			{
				if (!ignoretitleerrors) // title errors shouldn't be ignored
					asar_throw_error(0, error_type_block, error_id_rom_title_incorrect, expected_title.str, actual_display_title.str);
				else // title errors should be ignored, throw a warning anyways
					asar_throw_warning(0, warning_id_rom_title_incorrect, expected_title.str, actual_display_title.str);
			}
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
			else if (isdigit(par[i])) dig++;
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
		checksum=false;
		sublabels[0]=":xkasdefault:";
	}
	else if (is0("include") || is1("includefrom"))
	{
		if (!asarverallowed) asar_throw_error(0, error_type_block, error_id_start_of_file);
		if (istoplevel)
		{
			if (par) asar_throw_error(pass, error_type_fatal, error_id_cant_be_main_file, (string("The main file is '") + S par + S "'.").str);
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
				for (unsigned char * str=(unsigned char*)dequote(pars[i]);*str;str++)
				{
					if (len==1) write1(table.table[*str]);
					if (len==2) write2(table.table[*str]);
					if (len==3) write3(table.table[*str]);
					if (len==4) write4(table.table[*str]);
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
		int num=(int)getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_org_label_forward);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).str);
		if ((mapper==lorom || mapper==exlorom) && (num&0x408000)==0x400000) asar_throw_warning(0, warning_id_set_middle_byte);
		//if (fastrom) num|=0x800000;
		snespos=num;
		realsnespos=num;
		startpos=num;
		realstartpos=num;
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

		in_struct = numwords == 2 || numwords == 3;
		in_sub_struct = numwords == 4;

		if (numwords == 3)
		{
			int base = (int)getnum(word[2]);
			if (base&~0xFFFFFF) ret_error_params(error_id_snes_address_out_of_bounds, hex6((unsigned int)base).str);
			snespos = base;
			startpos = base;
		}
		else if (numwords == 4)
		{
			if (strcasecmp(word[2], "extends")) ret_error(error_id_missing_extends);
			if (!confirmname(word[3])) ret_error(error_id_struct_invalid_parent_name);
			struct_parent = word[3];

			if (!structs.exists(struct_parent)) ret_error_params(error_id_struct_not_found, struct_parent.str);
			snes_struct structure = structs.find(struct_parent);

			snespos = structure.base_end;
			startpos = structure.base_end;
		}

		push_pc();

		optimizeforbank = -1;

		struct_name = word[1];
		struct_base = snespos;
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
		int num=(int)getnum(par);
		if (forwardlabel) asar_throw_error(0, error_type_block, error_id_base_label_invalid);
		if (num&~0xFFFFFF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).str);
		snespos=num;
		startpos=num;
		optimizeforbank=-1;
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
		int num=(int)getnum(par);
		//if (forwardlabel) error(0, "bank Label is not valid");
		//if (foundlabel) num>>=16;
		if (num&~0x0000FF) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).str);
		optimizeforbank=num;
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
		autoptr<char**> pars=split(parstr.str, ",");
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
		if (snespos < 0 && mapper == sa1rom) asar_throw_error(pass, error_type_fatal, error_id_no_freespace_in_mapped_banks, dec(freespacelen[freespaceid]).str);
		if (snespos < 0) asar_throw_error(pass, error_type_fatal, error_id_no_freespace, dec(freespacelen[freespaceid]).str);
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
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.str);
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
			if (snespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_autoclean_in_freespace);
			const char * labeltest = word[2];
			string testlabel = labeltest;
			int num=(int)labelval(&labeltest);
			if (*labeltest) asar_throw_error(0, error_type_block, error_id_label_not_found, testlabel.str);
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
		snespos=0xFFFFFFFF;
		startpos=0xFFFFFFFF;
		realsnespos=0xFFFFFFFF;
		realstartpos=0xFFFFFFFF;
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
				const char * tmpstr=dequote(par);
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
		int maxpos=(int)getnum(par);
		if (snespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_warnpc_in_freespace);
		if (maxpos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_warnpc_broken_param);
		if (snespos > maxpos) asar_throw_error(0, error_type_block, error_id_warnpc_failed, hex6((unsigned int)snespos).str, hex6((unsigned int)maxpos).str);
		if (warnxkas && snespos == maxpos) asar_throw_warning(0, warning_id_xkas_warnpc_relaxed);
		if (emulatexkas && snespos==maxpos) asar_throw_error(0, error_type_block, error_id_warnpc_failed_equal, hex6((unsigned int)snespos).str, hex6((unsigned int)maxpos).str);
	}
	else if (is1("rep"))
	{
		int rep=(int)getnum(par);
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
		if (emulatexkas) name=dequote(par);
		else name=S dequote(par);
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
			start=(int)strtoul(lengths, &lengths, 16);
			if (*lengths!='-') asar_throw_error(0, error_type_block, error_id_broken_incbin);
			lengths++;
			end=(int)strtoul(lengths, &lengths, 16);
			if (*lengths) asar_throw_error(0, error_type_block, error_id_broken_incbin);
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
		if (emulatexkas) name=dequote(par);
		else name=S dequote(par);
		char * data;//I couldn't find a way to get this into an autoptr
		if (!readfile(name, thisfilename, &data, &len)) asar_throw_error(0, error_type_block, error_id_file_not_found, name.str);
		autoptr<char*> datacopy=data;
		if (!end) end=len;
		if (end < start || end > len) asar_throw_error(0, error_type_block, error_id_file_offset_out_of_bounds, dec(end).str, name.str);
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
					if (pos < 0) asar_throw_error(0, error_type_block, error_id_no_freespace, dec(end - start).str);
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
	else if (is1("skip"))//nobody ever uses this, but whatever
	{
		int skip=(int)getnum(par);
		if (foundlabel) asar_throw_error(0, error_type_block, error_id_skip_label_invalid);
		step(skip);
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
		string name=S dequote(par);
		autoptr<char*> tablecontents=readfile(name, thisfilename);
		if (!tablecontents) asar_throw_error(0, error_type_block, error_id_file_not_found, name.str);
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
				table.table[(unsigned char)tableline[0]]=(unsigned int)strtol(tableline+2, &end, 16);
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
		if (!pass)
		{
			if (stricmp(word[2], "=")) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
			if (!confirmqpar(word[1])) asar_throw_error(0, error_type_block, error_id_broken_function_declaration);
			string line=word[1];
			clean(line);
			char * startpar=strqchr(line.str, '(');
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
	}
	else if (is1("print"))
	{
		if (!confirmqpar(par)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
		string out;
		autoptr<char**> pars=qpsplit(par, ",");
		for (int i=0;pars[i];i++)
		{
			if(0);
			else if (pars[i][0]=='"') out+=dequote(pars[i]);
			else if (!stricmp(pars[i], "bytes")) out+=dec(bytes);
			else if (!stricmp(pars[i], "freespaceuse")) out+=dec(freespaceuse);
			else if (!stricmp(pars[i], "pc")) out+=hex6((unsigned int)(snespos&0xFFFFFF));
			else if (!strncasecmp(pars[i], "dec(", strlen("dec("))) out+=dec((int)getnum(pars[i]+strlen("dec")));
			else if (!strncasecmp(pars[i], "hex(", strlen("hex("))) out+=hex0(getnum(pars[i]+strlen("hex")));
			else if (!strncasecmp(pars[i], "double(", strlen("double(")))
			{
				char * arg1pos = pars[i] + strlen("double(");
				char * endpos = strchr(arg1pos, '\0');
				while (*endpos == ' ' || *endpos == '\t' || *endpos == '\0') endpos--;
				if (*endpos != ')') asar_throw_error(0, error_type_block, error_id_invalid_print_function_syntax);
				string params = string(arg1pos, (int)(endpos - arg1pos));

				int numargs;
				autoptr<char**> double_pars = qpsplit(params.str, ",", &numargs);
				if (numargs > 2) asar_throw_error(0, error_type_block, error_id_wrong_num_parameters);
				// qpsplit always returns at least 1 part (if the input is empty, it returns an empty string)

				if (numargs == 1)
				{
					out += ftostrvar(getnumdouble(double_pars[0]), 5);
				}
				else // numargs == 2
				{
					out += ftostrvar(getnumdouble(double_pars[0]), (int)getnum(double_pars[1]));
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
		if (realsnespos & 0xFF000000) asar_throw_error(0, error_type_block, error_id_pad_in_freespace);
		int num=(int)getnum(par);
		if (num & 0xFF000000) asar_throw_error(0, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex6((unsigned int)num).str);
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
	else if (is1("fill"))
	{
		int num=(int)getnum(par);
		for (int i=0;i<num;i++) write1(fillbyte[i%12]);
	}
	else if (is1("arch"))
	{
		if (emulatexkas) asar_throw_warning(0, warning_id_convert_to_asar);
		if (!stricmp(par, "65816")) { arch=arch_65816; return; }
		if (!stricmp(par, "spc700")) { arch=arch_spc700; return; }
		if (!stricmp(par, "spc700-inline")) { arch=arch_spc700_inline; return; }
		if (!stricmp(par, "spc700-raw")) { arch=arch_spc700; mapper=norom; checksum=false; return; }
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
		else if (!stricmp(word[1], "bankcross")) {
			disable_bank_cross_errors = !val;
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
		checksum=false;//we don't know where the header is, so don't set the checksum
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
			if (!isdigit(par[0]) || par[1]!=',' ||
					!isdigit(par[2]) || par[3]!=',' ||
					!isdigit(par[4]) || par[5]!=',' ||
					!isdigit(par[6]) || par[7]) asar_throw_error(0, error_type_block, error_id_invalid_mapper);
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
	return true;
}
