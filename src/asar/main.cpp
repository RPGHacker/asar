#include "std-includes.h"
#include "libsmw.h"
#include "libstr.h"
#include "assocarr.h"
#include "autoarray.h"
#include "asar.h"
#include "virtualfile.hpp"
#include "platform/file-helpers.h"

// randomdude999: remember to also update the .rc files (in res/windows/) when changing this.
// Couldn't find a way to automate this without shoving the version somewhere in the CMake files
extern const int asarver_maj=1;
extern const int asarver_min=6;
extern const int asarver_bug=0;
extern const bool asarver_beta=false;
extern bool specifiedasarver;

#ifdef _I_RELEASE
extern char blockbetareleases[(!asarver_beta)?1:-1];
#endif
#ifdef _I_DEBUG
extern char blockreleasedebug[(asarver_beta)?1:-1];
#endif

unsigned const char * romdata_r;
int romlen_r;

double math(const char * mystr, const char ** e);
void initmathcore();

int pass;

int optimizeforbank=-1;

string thisfilename;
int thisline;
int lastspecialline=-1;
const char * thisblock;

const char * callerfilename=NULL;
int callerline=-1;

bool errored=false;
bool ignoretitleerrors=false;

volatile int recursioncount=0;

virtual_filesystem* filesystem = nullptr;

unsigned int labelval(const char ** rawname, bool update);
unsigned int labelval(char ** rawname, bool update);
unsigned int labelval(string name, bool define);
bool labelval(const char ** rawname, unsigned int * rval, bool update);
bool labelval(char ** rawname, unsigned int * rval, bool update);
bool labelval(string name, unsigned int * rval, bool define);

void startmacro(const char * line);
void tomacro(const char * line);
void endmacro(bool insert);
void callmacro(const char * data);

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
			else if (isupper(c)) score+=3;
			else if (c==' ') score+=2;
			else if (isdigit(c)) score+=1;
			else if (islower(c)) score+=1;
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
		e+=S thisfilename;
		if (thisline!=-1) e+=S ":"+dec(thisline+1);
		if (callerfilename) e+=S" (called from "+callerfilename+":"+dec(callerline+1)+")";
		e+=": ";
	}
	return e;
}

extern int freespaceextra;
extern int freespaceid;
extern int freespacepos[256];

void checkbankcross();

extern bool warnxkas;
extern bool emulatexkas;

static bool freespaced;
static int getlenforlabel(int insnespos, int thislabel, bool exists)
{
	if (warnxkas && (((thislabel^insnespos)&0xFFFF0000)==0))
		warn1("xkas compatibility warning: label access is always 24bit in emulation mode, but may be 16bit in native mode");
	if (!exists)
	{
		if (!freespaced) freespaceextra++;
		freespaced=true;
		return 2;
	}
	else if (optimizeforbank>=0)
	{
		if (thislabel&0xFF000000) return 3;
		else if ((thislabel>>16)==optimizeforbank) return 2;
		else return 3;
	}
	else if((thislabel >> 16) == 0x7E && (thislabel & 0xFFFF) < 0x100)
	{
		return 1;
	}
	else if ((thislabel >> 16) == 0x7E && (thislabel & 0xFFFF) < 0x2000)
	{
		return 2;
	}
	else if ((thislabel|insnespos)&0xFF000000)
	{
		if ((thislabel^insnespos)&0xFF000000) return 3;
		else return 2;
	}
	else if ((thislabel^insnespos)&0xFF0000) return 3;
	else return 2;
}


string posneglabelname(const char ** input, bool define);

int getlen(const char * orgstr, bool optimizebankextraction)
{
	const char * str=orgstr;
	freespaced=false;

	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);

	if (posnegname.truelen() > 0)
	{
		if (*posneglabel != '\0') goto notposneglabel;

		if (!pass) return 2;
		unsigned int labelpos=31415926;
		bool found = labelval(posnegname, &labelpos);
		return getlenforlabel(snespos, (int)labelpos, found);
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
			for (i=0;isxdigit(str[i]);i++);
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
		else if (isdigit(*str))
		{
			int val=strtol(str, (char**)&str, 10);
			if (val>=0) thislen=1;
			if (val>=256) thislen=2;
			if (val>=65536) thislen=3;
			if (val>=16777216) thislen=4;
		}
		else if (isalpha(*str) || *str=='.' || *str=='?')
		{
			unsigned int thislabel;
			bool exists=labelval(&str, &thislabel);
			thislen=getlenforlabel(snespos, (int)thislabel, exists);
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

extern bool foundlabel;
extern bool forwardlabel;

int getnum(const char * str)
{
	const char * e;
	// RPG Hacker: this was originally an int - changed it into an unsigned int since I found
	// that to yield the more predictable results when converting from a double
	// (e.g.: $FFFFFFFF was originally converted to $80000000, whereas now it remains $FFFFFFFF
	unsigned int num=(unsigned int)math(str, &e);
	if (e)
	{
		error<errblock>(1, e);
	}
	return (int)num;
}

// RPG Hacker: Same function as above, but doesn't truncate our number via int conversion
double getnumdouble(const char * str)
{
	const char * e;
	double num = math(str, &e);
	if (e)
	{
		error<errblock>(1, e);
	}
	return num;
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

assocarr<sourcefile> filecontents;
assocarr<string> defines;
// needs to be separate because defines is reset between parsing arguments and patching
assocarr<string> clidefines;

void assembleblock(const char * block);

void resolvedefines(string& out, const char * start)
{
	recurseblock rec;
	const char * here=start;
	while (*here)
	{
		if (*here=='"' && emulatexkas)
		{
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
					if (!*here) error<errline>(0, "Unmatched braces.");
					if (!braces) break;
					unprocessedname+=*here++;
				}
				here++;
				resolvedefines(defname, unprocessedname);
				bool bad=false;
				if (!defname[0]) bad=true;
				for (int i=0;defname[i];i++)
				{
					if (!isualnum(defname[i])) bad=true;
				}
				if (bad) error<errline>(0, "Invalid define name.");
			}
			else
			{
				while (isualnum(*here)) defname+=*here++;
			}
			if (warnxkas && here[0]=='(' && here[1]==')')
				warn0("xkas compatibility warning: Unlike xkas, Asar does not eat parentheses after defines");
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
				if (emulatexkas && mode!=null) warn0("Convert the patch to native Asar format instead of making an Asar-only xkas patch.");
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
							else error<errline>(0, "Broken define declaration");
						}
						val+=*here++;
					}
					here++;
				}
				else
				{
					while (*here && *here!=' ') val+=*here++;
				}
				//if (strqchr(val.str, ';')) *strqchr(val.str, ';')=0;
				if (*here && !stribegin(here, " : ")) error<errline>(0, "Broken define declaration");
				clean(val);
				switch (mode)
				{
					case null:
					{
						defines.create(defname) = val;
						break;
					}
					case append:
					{
						if (!defines.exists(defname)) error<errnull>(0, "Appending to an undefined define");
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
						if (foundlabel) error<errline>(0, "!Define #= Label is not allowed");
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
					if (!defines.exists(defname)) error<errline>(0, S"Define !"+defname+" not found");
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

bool autocolon=false;

bool moreonline;
bool asarverallowed;
bool istoplevel;

extern int numtrue;
extern int numif;

extern int macrorecursion;

void assembleline(const char * fname, int linenum, const char * line)
{
	recurseblock rec;
	bool moreonlinetmp=moreonline;
	string absolutepath = filesystem->create_absolute_path("", fname);
	thisfilename = absolutepath;
	thisline=linenum;
	thisblock=NULL;
	try
	{
		string tmp=line;
		if (!confirmquotes(tmp)) error<errline>(0, "Mismatched quotes");
		clean(tmp);
		string out;
		if (numif==numtrue) resolvedefines(out, tmp);
		else out=tmp;
		out.qreplace(": :", ":  :", true);
//puts(out);
		autoptr<char**> blocks=qsplit(out.str, " : ");
		moreonline=true;
		for (int block=0;moreonline;block++)
		{
			moreonline=(blocks[block+1] != 0);
			int repeatthis=repeatnext;
			repeatnext=1;
			for (int i=0;i<repeatthis;i++)
			{
				try
				{
					itrim(blocks[block], " ", " ", true);
					thisfilename= absolutepath;
					thisline=linenum;//do not optimize, this one is recursive
					thisblock = blocks[block];
					if (thisblock[0] == '@')
					{
						lastspecialline = thisline;
						thisblock++;
						while (isspace(*thisblock))
						{
							thisblock++;
						}
					}
					assembleblock(thisblock);
				}
				catch (errblock&) {}
				if (blocks[block][0]!='\0' && blocks[block][0]!='@') asarverallowed=false;
			}
		}
	}
	catch (errline&) {}
	moreonline=moreonlinetmp;
}

extern int numif;
extern int numtrue;
extern autoarray<whiletracker> whilestatus;

int incsrcdepth=0;

void assemblefile(const char * filename, bool toplevel)
{
	incsrcdepth++;
	string absolutepath = filesystem->create_absolute_path(thisfilename, filename);
	thisfilename = absolutepath;
	thisline=-1;
	thisblock=NULL;
	sourcefile file;
	file.contents = NULL;
	file.numlines = 0;
	int startif=numif;
	if (!filecontents.exists(absolutepath))
	{
		char * temp= readfile(absolutepath, "");
		if (!temp)
		{
			error<errnull>(0, "Couldn't open file");
			return;
		}
		file.contents =split(temp, "\n");
		for (int i=0;file.contents[i];i++)
		{
			file.numlines++;
			char * line= file.contents[i];
			char * comment=line;
			comment = strqchr(comment, ';');
			while (comment != NULL)
			{
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
			if (!confirmquotes(line)) { thisline=i; thisblock=line; error<errnull>(0, "Mismatched quotes"); line[0]='\0'; }
			itrim(line, " ", " ", true);
			for (int j=1;strqchr(line, ',') && !strqchr(line, ',')[1] && file.contents[i+j];j++)
			{
				strcat(line, file.contents[i+j]);
				static char nullstr[]="";
				file.contents[i+j]=nullstr;
			}
		}
		filecontents.create(absolutepath) = file;
	} else { // filecontents.exists(absolutepath)
		file = filecontents.find(absolutepath);
	}
	bool inmacro=false;
	asarverallowed=true;
	for (int i=0;file.contents[i] && i<file.numlines;i++)
	{
		try
		{
			thisfilename= absolutepath;
			thisline=i;
			thisblock=NULL;
			istoplevel=toplevel;
			if (stribegin(file.contents[i], "macro ") && numif==numtrue)
			{
				if (inmacro) error<errline>(0, "Nested macro definition");
				inmacro=true;
				if (!pass) startmacro(file.contents[i]+6);
			}
			else if (!stricmp(file.contents[i], "endmacro") && numif==numtrue)
			{
				if (!inmacro) error<errline>(0, "Misplaced endmacro");
				inmacro=false;
				if (!pass) endmacro(true);
			}
			else if (inmacro)
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
				if (numif != prevnumif && whilestatus[numif].iswhile && whilestatus[numif].cond)
					i = whilestatus[numif].startline - 1;
			}
		}
		catch (errline&) {}
	}
	thisline++;
	thisblock=NULL;
	checkbankcross();
	if (inmacro)
	{
		error<errnull>(0, "Unclosed macro");
		if (!pass) endmacro(false);
	}
	if (repeatnext!=1)
	{
		repeatnext=1;
		error<errnull>(0, "rep at the end of a file");
	}
	if (numif!=startif)
	{
		numif=startif;
		numtrue=startif;
		error<errnull>(0, "Unclosed if statement");
	}
	incsrcdepth--;
}

void parse_std_includes(const char* textfile, autoarray<string>& outarray)
{
	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		char* pos = content;
		// TODO: Add support for relative paths (relative from the TXT file)
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

			stdinclude = itrim(stdinclude.str, " ", " ", true);
			stdinclude = itrim(stdinclude.str, "\t", "\t", true);

			if (stdinclude != "")
			{
				if (!path_is_absolute(stdinclude))
				{
					stdinclude = dir(textfile) + stdinclude;
				}

				outarray.append(stdinclude);
			}
		}

		free(content);
	}
}

void parse_std_defines(const char* textfile)
{
	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		// TODO
		free(content);
	}
}

bool checksum=true;
extern assocarr<unsigned int> labels;
extern autoarray<writtenblockdata> writtenblocks;

struct macrodata
{
autoarray<string> lines;
int numlines;
int startline;
const char * fname;
const char ** arguments;
int numargs;
};
extern assocarr<macrodata*> macros;
extern assocarr<snes_struct> structs;

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

void closecachedfiles();
void deinitmathcore();

void reseteverything()
{
	string str;
	labels.reset();
	defines.reset();
	clidefines.each(adddefine);
	structs.reset();

	macros.each(clearmacro);
	macros.reset();

	filecontents.each(clearfile);
	filecontents.reset();

	writtenblocks.reset();

	optimizeforbank=-1;

	closecachedfiles();

	deinitmathcore();

	incsrcdepth=0;
	specifiedasarver=false;
	errored = false;
	checksum = true;
	
	lastspecialline = -1;
#undef free
}
