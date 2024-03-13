// "because satanism is best defeated by summoning a bigger satan"
//   ~Alcaro, 2019 (discussing Asar)
#include "addr2line.h"
#include "std-includes.h"
#include "libsmw.h"
#include "libstr.h"
#include "assocarr.h"
#include "autoarray.h"
#include "asar.h"
#include "virtualfile.h"
#include "warnings.h"
#include "platform/file-helpers.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include <cstdint>

// randomdude999: remember to also update the .rc files (in res/windows/) when changing this.
// Couldn't find a way to automate this without shoving the version somewhere in the CMake files
const int asarver_maj=1;
const int asarver_min=9;
const int asarver_bug=1;
const bool asarver_beta=false;
bool default_math_pri=false;
bool default_math_round_off=false;
extern bool suppress_all_warnings;

#ifdef _I_RELEASE
extern char blockbetareleases[(!asarver_beta)?1:-1];
#endif
#ifdef _I_DEBUG
extern char blockreleasedebug[(asarver_beta)?1:-1];
#endif

unsigned const char * romdata_r;
int romlen_r;

int pass;

int optimizeforbank=-1;
int optimize_dp = optimize_dp_flag::NONE;
bool set_optimize_dp = false;
int dp_base = 0;
int optimize_address = optimize_address_flag::DEFAULT;
bool set_optimize_address = false;

string thisfilename;
int thisline;
const char * thisblock;

string callerfilename;
int callerline=-1;

bool errored=false;
bool ignoretitleerrors=false;

volatile int recursioncount=0;

virtual_filesystem* filesystem = nullptr;

AddressToLineMapping addressToLineMapping;

int get_version_int()
{
	return asarver_maj * 10000 + asarver_min * 100 + asarver_bug;
}

bool setmapper()
{
	int maxscore=-99999;
	mapper_t bestmap=lorom;
	mapper_t maps[]={lorom, hirom, exlorom, exhirom};
	for (size_t mapid=0;mapid<sizeof(maps)/sizeof(maps[0]);mapid++)
	{
		mapper=maps[mapid];
		int score=0;
		int highbits=0;
		bool foundnull=false;
		for (int i=0;i<21;i++)
		{
			unsigned char c=romdata[snestopc(0x00FFC0+i)];
			if (foundnull && c) score-=4;//according to some documents, NUL terminated names are possible - but they shouldn't appear in the middle of the name
			if (c>=128) highbits++;
			else if (is_upper(c)) score+=3;
			else if (c==' ') score+=2;
			else if (is_digit(c)) score+=1;
			else if (is_lower(c)) score+=1;
			else if (c=='-') score+=1;
			else if (!c) foundnull=true;
			else score-=3;
		}
		if (highbits>0 && highbits<=14) score-=21;//high bits set on some, but not all, bytes = unlikely to be a ROM
		if ((romdata[snestopc(0x00FFDE)]^romdata[snestopc(0x00FFDC)])!=0xFF ||
				(romdata[snestopc(0x00FFDF)]^romdata[snestopc(0x00FFDD)])!=0xFF) score=-99999;//checksum doesn't match up to 0xFFFF? Not a ROM.
		//too lazy to check the real checksum
		if (score>maxscore)
		{
			maxscore=score;
			bestmap=mapper;
		}
	}
	mapper=bestmap;

	//detect oddball mappers
	int mapperbyte=romdata[snestopc(0x00FFD5)];
	int romtypebyte=romdata[snestopc(0x00FFD6)];
	if (mapper==lorom)
	{
		if (mapperbyte==0x23 && (romtypebyte==0x32 || romtypebyte==0x34 || romtypebyte==0x35)) mapper=sa1rom;
	}
	return (maxscore>=0);
}

string getdecor()
{
	string e;
	if (thisfilename)
	{
		e+=STR thisfilename;
		if (thisline!=-1) e+=STR ":"+dec(thisline+1);
		if (callerfilename) e+=STR" (called from "+callerfilename+":"+dec(callerline+1)+")";
		e+=": ";
	}
	return e;
}

asar_error_id vfile_error_to_error_id(virtual_file_error vfile_error)
{
	switch (vfile_error)
	{
	case vfe_doesnt_exist:
		return error_id_file_not_found;
	case vfe_access_denied:
		return error_id_failed_to_open_file_access_denied;
	case vfe_not_regular_file:
		return error_id_failed_to_open_not_regular_file;
	case vfe_unknown:
	case vfe_none:
	case vfe_num_errors:
		return error_id_failed_to_open_file;
	}

	return error_id_failed_to_open_file;
}

virtual_file_error asar_get_last_io_error()
{
	if (filesystem != nullptr)
	{
		return filesystem->get_last_error();
	}

	return vfe_unknown;
}

static bool freespaced;
static int getlenforlabel(int insnespos, int thislabel, bool exists)
{
	if (warnxkas && (((unsigned int)(thislabel^insnespos)&0xFFFF0000)==0))
		asar_throw_warning(1, warning_id_xkas_label_access);
	unsigned int bank = thislabel>>16;
	unsigned int word = thislabel&0xFFFF;
	unsigned int relaxed_bank;
	if(optimizeforbank >= 0) {
		relaxed_bank = optimizeforbank;
	} else {
		if((insnespos & 0xff000000) == 0) {
			relaxed_bank = insnespos >> 16;
		} else {
			if(freespace_is_freecode) relaxed_bank = 0;
			else relaxed_bank = 0x40;
		}
	}
	if (!exists)
	{
		if (!freespaced) freespaceextra++;
		freespaced=true;
		return 2;
	}
	else if((optimize_dp == optimize_dp_flag::RAM) && bank == 0x7E && (word-dp_base < 0x100))
	{
		return 1;
	}
	else if(optimize_dp == optimize_dp_flag::ALWAYS && (bank == 0x7E || !(bank & 0x40)) && (word-dp_base < 0x100))
	{
		return 1;
	}
	else if (
		// if we should optimize ram accesses...
		(optimize_address == optimize_address_flag::RAM || optimize_address == optimize_address_flag::MIRRORS)
		// and we're in a bank with ram mirrors... (optimizeforbank=0x7E is checked later)
		&& !(relaxed_bank & 0x40)
		// and the label is in low RAM
		&& bank == 0x7E && word < 0x2000)
	{
		return 2;
	}
	else if (
		// if we should optimize mirrors...
		optimize_address == optimize_address_flag::MIRRORS
		// we're in a bank with ram mirrors...
		&& !(relaxed_bank & 0x40)
		// and the label is in a mirrored section
		&& !(bank & 0x40) && word < 0x8000)
	{
		return 2;
	}
	else if (optimizeforbank>=0)
	{
		// if optimizing for a specific bank:
		// if the label is in freespace, never optimize
		if ((unsigned int)thislabel&0xFF000000) return 3;
		else if (bank==(unsigned int)optimizeforbank) return 2;
		else return 3;
	}
	else if ((unsigned int)(thislabel|insnespos)&0xFF000000)
	{
		// optimize only if the label is in the same freespace
		if ((unsigned int)(thislabel^insnespos)&0xFF000000) return 3;
		else return 2;
	}
	else if ((thislabel^insnespos)&0xFF0000){ return 3; }
	else { return 2;}
}

bool is_hex_constant(const char* str){
	if (*str=='$')
	{
		str++;
		while(is_xdigit(*str)) {
			str++;
		}
		if(*str=='\0'){
			return true;
		}
	}
	return false;
}

int getlen(const char * orgstr, bool optimizebankextraction)
{
	const char * str=orgstr;
	freespaced=false;

	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);

	if (posnegname.length() > 0)
	{
		if (*posneglabel != '\0') goto notposneglabel;

		if (!pass) return 2;
		snes_label label_data;
		// RPG Hacker: Umm... what kind of magic constant is this?
		label_data.pos = 31415926;
		bool found = labelval(posnegname, &label_data);
		return getlenforlabel(snespos, (int)label_data.pos, found);
	}
notposneglabel:
	int len=0;
	while (*str)
	{
		int thislen=0;
		bool maybebankextraction=(str==orgstr);
		if (*str=='$')
		{
			str++;
			int i;
			for (i=0;is_xdigit(str[i]);i++);
			//if (i&1) warn(S dec(i)+"-digit hex value");//blocked in getnum instead
			thislen=(i+1)/2;
			str+=i;
		}
		else if (*str=='%')
		{
			str++;
			int i;
			for (i=0;str[i]=='0' || str[i]=='1';i++);
			//if (i&7) warn(S dec(i)+"-digit binary value");
			thislen=(i+7)/8;
			str+=i;
		}
		else if (str[0]=='\'' && str[2]=='\'')
		{
			thislen=1;
			str+=3;
		}
		else if (is_digit(*str))
		{
			int val=strtol(str, const_cast<char**>(&str), 10);
			if (val>=0) thislen=1;
			if (val>=256) thislen=2;
			if (val>=65536) thislen=3;
			if (val>=16777216) thislen=4;
		}
		else if (is_alpha(*str) || *str=='_' || *str=='.' || *str=='?')
		{
			snes_label thislabel;
			bool exists=labelval(&str, &thislabel);
			thislen=getlenforlabel(snespos, (int)thislabel.pos, exists);
		}
		else str++;
		if (optimizebankextraction && maybebankextraction &&
				(!strcmp(str, ">>16") || !strcmp(str, "/65536") || !strcmp(str, "/$10000")))
					return 1;
		if (thislen>len) len=thislen;
	}
	if (len>3) return 3;
	return len;
}

struct strcompare {
	bool operator() (const char * lhs, const char * rhs) const
	{
		return strcmp(lhs, rhs)<0;
	}
};

struct stricompare {
	bool operator() (const char * lhs, const char * rhs) const
	{
		return stricmp(lhs, rhs)<0;
	}
};

struct sourcefile {
	char** contents;
	int numlines;
};

static assocarr<sourcefile> filecontents;
assocarr<string> defines;
// needs to be separate because defines is reset between parsing arguments and patching
assocarr<string> clidefines;
assocarr<string> builtindefines;

bool validatedefinename(const char * name)
{
	if (!name[0]) return false;
	for (int i = 0;name[i];i++)
	{
		if (!is_ualnum(name[i])) return false;
	}

	return true;
}

void resolvedefines(string& out, const char * start)
{
	recurseblock rec;
	const char * here=start;
	while (*here)
	{
		if (*here=='"' && emulatexkas)
		{
			asar_throw_warning(0, warning_id_feature_deprecated, "xkas define quotes", "removing the quotes generally does what you want");
			out+=*here++;
			while (*here && *here!='"') out+=*here++;
			out+=*here++;
		}
		else if (here[0] == '\\' && here[1] == '\\')
		{
			// allow using \\ as escape sequence
			out += "\\";
			here += 2;
		}
		else if (here[0] == '\\' && here[1] == '!')
		{
			// allow using \! to escape !
			out+="!";
			here += 2;
		}
		else if (*here=='!')
		{
			bool first=(here==start || (here>=start+4 && here[-1]==' ' && here[-2]==':' && here[-3]==' '));//check if it's the start of a block
			string defname;
			here++;
			if (*here=='{')
			{
				here++;
				string unprocessedname;
				int braces=1;
				while (true)
				{
					if (*here=='{') braces++;
					if (*here=='}') braces--;
					if (!*here) asar_throw_error(0, error_type_line, error_id_mismatched_braces);
					if (!braces) break;
					unprocessedname+=*here++;
				}
				here++;
				resolvedefines(defname, unprocessedname);
				if (!validatedefinename(defname)) asar_throw_error(0, error_type_line, error_id_invalid_define_name);
			}
			else
			{
				while (is_ualnum(*here)) defname+=*here++;
			}
			if (warnxkas && here[0]=='(' && here[1]==')')
				asar_throw_warning(0, warning_id_xkas_eat_parentheses);
			//if (emulatexkas && here[0]=='(' && here[1]==')') here+=2;

			if (first)
			{
				enum {
					null,
					append,
					expand,
					domath,
					setifnotset,
				} mode;
				if(0);
				else if (stribegin(here,  " = ")) { here+=3; mode=null; }
				else if (stribegin(here, " += ")) { here+=4; mode=append; }
				else if (stribegin(here, " := ")) { here+=4; mode=expand; }
				else if (stribegin(here, " #= ")) { here+=4; mode=domath; }
				else if (stribegin(here, " ?= ")) { here+=4; mode=setifnotset; }
				else goto notdefineset;
				if (emulatexkas && mode != null) asar_throw_warning(0, warning_id_convert_to_asar);
				//else if (stribegin(here, " equ ")) here+=5;
				string val;
				if (*here=='"')
				{
					here++;
					while (true)
					{
						if (*here=='"')
						{
							if (!here[1] || here[1]==' ') break;
							else if (here[1]=='"') here++;
							else asar_throw_error(0, error_type_line, error_id_broken_define_declaration);
						}
						val+=*here++;
					}
					here++;
				}
				else
				{
					while (*here && *here!=' ') val+=*here++;
				}
				//if (strqchr(val.data(), ';')) *strqchr(val.data(), ';')=0;
				if (*here && !stribegin(here, " : ")) asar_throw_error(0, error_type_line, error_id_broken_define_declaration);
				clean(val);

				// RPG Hacker: throw an error if we're trying to overwrite built-in defines.
				if (builtindefines.exists(defname))
				{
					asar_throw_error(0, error_type_line, error_id_overriding_builtin_define, defname.data());
				}

				switch (mode)
				{
					case null:
					{
						defines.create(defname) = val;
						break;
					}
					case append:
					{
						if (!defines.exists(defname)) asar_throw_error(0, error_type_line, error_id_define_not_found, defname.data());
						string oldval = defines.find(defname);
						val=oldval+val;
						defines.create(defname) = val;
						break;
					}
					case expand:
					{
						string newval;
						resolvedefines(newval, val);
						defines.create(defname) = newval;
						break;
					}
					case domath:
					{
						string newval;
						resolvedefines(newval, val);
						double num= getnumdouble(newval);
						if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_line, error_id_define_label_math);
						defines.create(defname) = ftostr(num);
						break;
					}
					case setifnotset:
					{
						if (!defines.exists(defname)) defines.create(defname) = val;
						break;
					}
				}
			}
			else
			{
			notdefineset:
				if (!defname) out+="!";
				else
				{
					if (!defines.exists(defname)) asar_throw_error(0, error_type_line, error_id_define_not_found, defname.data());
					else {
						string thisone = defines.find(defname);
						resolvedefines(out, thisone);
					}
				}
			}
		}
		else out+=*here++;
	}
}

int repeatnext=1;

bool moreonline;
bool moreonlinecond;
int fakeendif;
bool asarverallowed;
bool istoplevel;

void assembleline(const char * fname, int linenum, const char * line)
{
	recurseblock rec;
	bool moreonlinetmp=moreonline;
	// randomdude999: redundant, assemblefile already converted the path to absolute
	//string absolutepath = filesystem->create_absolute_path("", fname);
	string absolutepath = fname;
	thisfilename = absolutepath;
	thisline=linenum;
	thisblock= nullptr;
	single_line_for_tracker = 1;
	try
	{
		string tmp;
		if(inmacro && numif == numtrue) tmp = replace_macro_args(line);
		else tmp = line;
		clean(tmp);
		string out;
		if (numif==numtrue) resolvedefines(out, tmp);
		else out=tmp;
		// recheck quotes - defines can mess those up sometimes
		if (!confirmquotes(out)) asar_throw_error(0, error_type_line, error_id_mismatched_quotes);
		out.qreplace(": :", ":  :", true);
//puts(out);
		autoptr<char**> blocks=qsplit(out.temp_raw(), " : ");
		moreonline=true;
		moreonlinecond=true;
		fakeendif=0;
		for (int block=0;moreonline;block++)
		{
			moreonline=(blocks[block+1] != nullptr);
			int repeatthis=repeatnext;
			repeatnext=1;
			for (int i=0;i<repeatthis;i++)
			{
				try
				{
					string stripped_block = blocks[block];
					strip_both(stripped_block, ' ', true);
					
					thisline=linenum;//do not optimize, this one is recursive
					thisblock = stripped_block.data();
					bool isspecialline = false;
					if (thisblock[0] == '@')
					{
						asar_throw_warning(0, warning_id_feature_deprecated, "prefixing Asar commands with @ or ;@", "remove the @ or ;@ prefix");

						isspecialline = true;
						thisblock++;
						while (is_space(*thisblock))
						{
							thisblock++;
						}
					}
					assembleblock(thisblock, isspecialline);
					checkbankcross();
				}
				catch (errblock&) {}
				if (blocks[block][0]!='\0' && blocks[block][0]!='@') asarverallowed=false;
			}
			if(single_line_for_tracker == 1) single_line_for_tracker = 0;
		}
		if(fakeendif)
		{
			thisline = linenum;
			thisblock = blocks[0];
			asar_throw_warning(0, warning_id_feature_deprecated, "inline if statements without endif", "Add an \" : endif\" at the end of the line");
			if (numif==numtrue) numtrue--;
			numif--;
		}
	}
	catch (errline&) {}
	moreonline=moreonlinetmp;
}

int incsrcdepth=0;

// Returns true if a file is protected via
// an "includeonce".
bool file_included_once(const char* file)
{
	for (int i = 0; i < includeonce.count; ++i)
	{
		if (includeonce[i] == file)
		{
			return true;
		}
	}

	return false;
}

void assemblefile(const char * filename, bool toplevel)
{
	incsrcdepth++;
	string absolutepath = filesystem->create_absolute_path(thisfilename, filename);

	if (file_included_once(absolutepath))
	{
		return;
	}

	string prevthisfilename = thisfilename;
	thisfilename = absolutepath;
	int prevline = thisline;
	thisline=-1;
	const char* prevthisblock = thisblock;
	thisblock= nullptr;
	sourcefile file;
	file.contents = nullptr;
	file.numlines = 0;
	int startif=numif;
	if (!filecontents.exists(absolutepath))
	{
		char * temp= readfile(absolutepath, "");
		if (!temp)
		{
			// RPG Hacker: This is so that we hopefully always end up with a proper decor
			// and get better error messages.
			thisfilename = prevthisfilename;
			thisline = prevline;
			thisblock = prevthisblock;

			asar_throw_error(0, error_type_null, vfile_error_to_error_id(asar_get_last_io_error()), filename);

			return;
		}
		file.contents =split(temp, "\n");
		for (int i=0;file.contents[i];i++)
		{
			file.numlines++;
			char * line= file.contents[i];
			char * comment=line;
			comment = strqchr(comment, ';');
			while (comment != nullptr)
			{
				const char* comment_end = comment + strlen(comment);
				if (comment_end - comment >= 2
					&& comment[1] == '[' && comment[2] == '['
					&& (comment_end[-1] != ']' || comment_end[-2] != ']'))
				{
					asar_throw_warning(0, warning_id_feature_deprecated, "comments starting with ;[[", "\";[[\" marks the start of a block comments in Asar 2.0 - either remove the \"[[\", or make sure the commented line ends on \"]]\"");
				}

				if (comment[1]!='@')
				{
					comment[0]='\0';
				}
				else
				{
					comment[0] = ' ';
				}
				comment = strqchr(comment, ';');
			}
			while (strqchr(line, '\t')) *strqchr(line, '\t')=' ';
			if (!confirmquotes(line)) { thisline = i; thisblock = line; asar_throw_error(0, error_type_null, error_id_mismatched_quotes); line[0] = '\0'; }
			itrim(line, " ", " ", true);	//todo make use strip
		}
		for(int i=0;file.contents[i];i++)
		{
			char* line = file.contents[i];
			for (int j=1;strqrchr(line, ',') && !strqrchr(line, ',')[1] && file.contents[i+j];j++)
			{
				// not using strcat because the source and dest overlap here
				char* otherline = file.contents[i+j];
				char* line_end = line + strlen(line);
				while(*otherline) *line_end++ = *otherline++;
				*line_end = '\0';
				static char nullstr[]="";
				file.contents[i+j]=nullstr;
			}
		}
		filecontents.create(absolutepath) = file;
	} else { // filecontents.exists(absolutepath)
		file = filecontents.find(absolutepath);
	}
	bool in_macro_def=false;
	asarverallowed=true;
	for (int i=0;file.contents[i] && i<file.numlines;i++)
	{
		try
		{
			thisfilename= absolutepath;
			thisline=i;
			thisblock= nullptr;
			istoplevel=toplevel;
			if (stribegin(file.contents[i], "macro ") && numif==numtrue)
			{
				// RPG Hacker: Commented out for Asar 1.81 backwards-compatibility.
				// (From Asar 2.0 onwards, nested macro definitions will be well-defined).
				if (in_macro_def /*|| inmacro*/) asar_throw_error(0, error_type_line, error_id_nested_macro_definition);
				in_macro_def=true;
				if (!pass) startmacro(file.contents[i]+6);
			}
			else if (!stricmp(file.contents[i], "endmacro") && numif==numtrue)
			{
				if (!in_macro_def) asar_throw_error(0, error_type_line, error_id_misplaced_endmacro);
				in_macro_def=false;
				if (!pass) endmacro(true);
			}
			else if (in_macro_def)
			{
				if (!pass) tomacro(file.contents[i]);
			}
			else
			{
				int prevnumif = numif;
				string connectedline;
				int skiplines = getconnectedlines<char**>(file.contents, i, connectedline);
				assembleline(absolutepath, i, connectedline);
				thisfilename = absolutepath;
				i += skiplines;
				if ((numif != prevnumif || single_line_for_tracker == 3) && (whilestatus[numif].iswhile || whilestatus[numif].is_for) && whilestatus[numif].cond)
					i = whilestatus[numif].startline - 1;
			}
		}
		catch (errline&) {}
	}
	thisline++;
	thisblock= nullptr;
	if (in_macro_def)
	{
		asar_throw_error(0, error_type_null, error_id_unclosed_macro);
		if (!pass) endmacro(false);
	}
	if (repeatnext!=1)
	{
		repeatnext=1;
		asar_throw_error(0, error_type_null, error_id_rep_at_file_end);
	}
	if (numif!=startif)
	{
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_null, error_id_unclosed_if);
	}
	incsrcdepth--;
}

void parse_std_includes(const char* textfile, autoarray<string>& outarray)
{
	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		char* pos = content;

		while (pos[0] != '\0')
		{
			string stdinclude;

			do 
			{
				if (pos[0] != '\r' && pos[0] != '\n')
				{
					stdinclude += pos[0];
				}
				pos++;
			} while (pos[0] != '\0' && pos[0] != '\n');

			stdinclude = strip_whitespace(stdinclude);

			if (stdinclude != "")
			{
				if (!path_is_absolute(stdinclude))
				{
					stdinclude = dir(textfile) + stdinclude;
				}
				outarray.append(normalize_path(stdinclude));
			}
		}

		free(content);
	}
}

void parse_std_defines(const char* textfile)
{

	// RPG Hacker: add built-in defines.
	// (They're not really standard defines, but I was lazy and this was
	// one convenient place for doing it).
	builtindefines.create("assembler") = "asar";
	builtindefines.create("assembler_ver") = dec(get_version_int());

	if(textfile == nullptr) return;

	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		char* pos = content;
		while (*pos != 0) {
			string define_name;
			string define_val;

			while (*pos != '=' && *pos != '\n') {
				define_name += *pos;
				pos++;
			}
			if (*pos != 0 && *pos != '\n') pos++; // skip =
			while (*pos != 0 && *pos != '\n') {
				define_val += *pos;
				pos++;
			}
			if (*pos != 0)
				pos++; // skip \n
			// clean define_name
			define_name = strip_whitespace(define_name);
			define_name = strip_prefix(define_name, '!', false); // remove leading ! if present

			if (define_name == "")
			{
				if (define_val == "")
				{
					continue;
				}

				asar_throw_error(pass, error_type_null, error_id_stddefines_no_identifier);
			}

			if (!validatedefinename(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_invalid, "stddefines.txt", define_name.data());

			// clean define_val
			const char* defval = define_val.data();
			string cleaned_defval;

			if (*defval == 0) {
				// no value
				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = "";
				continue;
			}

			while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace in beginning
			if (*defval == '"') {
				defval++; // skip opening quote
				while (*defval != '"' && *defval != 0)
					cleaned_defval += *defval++;

				if (*defval == 0) {
					asar_throw_error(pass, error_type_null, error_id_mismatched_quotes);
				}
				defval++; // skip closing quote
				while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace
				if (*defval != 0 && *defval != '\n')
					asar_throw_error(pass, error_type_null, error_id_stddefine_after_closing_quote);

				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = cleaned_defval;
				continue;
			}
			else
			{
				// slightly hacky way to remove trailing whitespace
				const char* defval_end = strchr(defval, '\n'); // slightly hacky way to get end of string or newline
				if (!defval_end) defval_end = strchr(defval, 0);
				defval_end--;
				while (*defval_end == ' ' || *defval_end == '\t') defval_end--;
				cleaned_defval = string(defval, (int)(defval_end - defval + 1));

				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = cleaned_defval;
				continue;
			}

		}
		free(content);
	}
}

bool checksum_fix_enabled = true;
bool force_checksum_fix = false;

#define cfree(x) free((void*)x)
static void clearmacro(const string & key, macrodata* & macro)
{
	(void)key;
	macro->lines.~autoarray();
	cfree(macro->fname);
	cfree(macro->arguments[0]);
	cfree(macro->arguments);
	cfree(macro);
}

static void clearfile(const string & key, sourcefile& filecontent)
{
	(void)key;
	cfree(*filecontent.contents);
	cfree(filecontent.contents);
}

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

static string symbolfile;

static void printsymbol_wla(const string& key, snes_label& label)
{
	string line = hex2((label.pos & 0xFF0000)>>16)+":"+hex4(label.pos & 0xFFFF)+" "+key+"\n";
	symbolfile += line;
}

static void printsymbol_nocash(const string& key, snes_label& label)
{
	string line = hex8(label.pos & 0xFFFFFF)+" "+key+"\n";
	symbolfile += line;
}

string create_symbols_file(string format, uint32_t romCrc){
	format = lower(format);
	symbolfile = "";
	if(format == "wla")
	{
		symbolfile =  "; wla symbolic information file\n";
		symbolfile += "; generated by asar\n";

		symbolfile += "\n[labels]\n";
		labels.each(printsymbol_wla);

		symbolfile += "\n[source files]\n";
		const autoarray<AddressToLineMapping::FileInfo>& addrToLineFileList = addressToLineMapping.getFileList();
		for (int i = 0; i < addrToLineFileList.count; ++i)
		{
			char addrToFileListStr[256];
			snprintf(addrToFileListStr, 256, "%.4x %.8x %s\n",
				i,
				addrToLineFileList[i].fileCrc,
				addrToLineFileList[i].filename.data()
			);
			symbolfile += addrToFileListStr;
		}

		symbolfile += "\n[rom checksum]\n";
		{
			char romCrcStr[32];
			snprintf(romCrcStr, 32, "%.8x\n",
				romCrc
			);
			symbolfile += romCrcStr;
		}

		symbolfile += "\n[addr-to-line mapping]\n";
		const autoarray<AddressToLineMapping::AddrToLineInfo>& addrToLineInfo = addressToLineMapping.getAddrToLineInfo();
		for (int i = 0; i < addrToLineInfo.count; ++i)
		{
			char addrToLineStr[32];
			snprintf(addrToLineStr, 32, "%.2x:%.4x %.4x:%.8x\n",
				(addrToLineInfo[i].addr & 0xFF0000) >> 16,
				addrToLineInfo[i].addr & 0xFFFF,
				addrToLineInfo[i].fileIdx & 0xFFFF,
				addrToLineInfo[i].line & 0xFFFFFFFF
			);
			symbolfile += addrToLineStr;
		}

	}
	else if (format == "nocash")
	{
		symbolfile = ";no$sns symbolic information file\n";
		symbolfile += ";generated by asar\n";
		symbolfile += "\n";
		labels.each(printsymbol_nocash);
	}
	return symbolfile;
}

void reseteverything()
{
	string str;
	labels.reset();
	defines.reset();
	builtindefines.each(adddefine);
	clidefines.each(adddefine);
	structs.reset();

	macros.each(clearmacro);
	macros.reset();

	filecontents.each(clearfile);
	filecontents.reset();

	writtenblocks.reset();

	optimizeforbank=-1;
	optimize_dp = optimize_dp_flag::NONE;
	dp_base = 0;
	optimize_address = optimize_address_flag::DEFAULT;

	closecachedfiles();

	incsrcdepth=0;
	errored = false;
	checksum_fix_enabled = true;
	force_checksum_fix = false;

	default_math_pri = false;
	default_math_round_off = false;
	suppress_all_warnings = false;
	
#undef free
}
