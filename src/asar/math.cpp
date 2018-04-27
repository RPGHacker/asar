//Don't try using this in your own project, it's got a lot of Asar-specific tweaks. Use mathlib.cpp instead.
#include "platform/file-helpers.h"
#include "std-includes.h"
#include "autoarray.h"
#include "assocarr.h"
#include "libstr.h"
#include "libsmw.h"
#include "asar.h"
#include "virtualfile.hpp"

bool math_pri=true;
bool math_round=false;

extern bool emulatexkas;
extern assocarr<snes_struct> structs;
extern assocarr<string> defines;

//int bp(const char * str)
//{
//	throw str;
//	return 0;
//}
//#define error(str) bp(str)
#define error(str) throw str
static const char * str;

static double getnumcore();
static double getnum();
static double eval(int depth);

bool confirmname(const char * name);

//label (bool foundLabel) (bool forwardLabel)
//userfunction

bool foundlabel;
bool forwardlabel;


struct funcdat {
	autoptr<char*> name;
	int numargs;
	autoptr<char*> argbuf;//this one isn't used, it's just to free it up
	autoptr<char**> arguments;
	autoptr<char*> content;
};
autoarray<funcdat> userfunc;
int numuserfunc=0;

const char * createuserfunc(const char * name, const char * arguments, const char * content)
{
	if (!confirmqpar(content)) return "Mismatched parentheses";
	for (int i=0;i<numuserfunc;i++)
	{
		if (!strcmp(name, userfunc[i].name))
		{
			return "Duplicate function name";
		}
	}
	funcdat& thisone=userfunc[numuserfunc];
	thisone.name=strdup(name);
	thisone.argbuf=strdup(arguments);
	thisone.arguments=qsplit(thisone.argbuf, ",", &(thisone.numargs));
	thisone.content=strdup(content);
	for (int i=0;thisone.arguments[i];i++)
	{
		if (!confirmname(thisone.arguments[i]))
		{
			userfunc.remove(numuserfunc);
			return "Invalid argument name";
		}
	}
	numuserfunc++;
	return NULL;
}

struct cachedfile {
	string filename;
	virtual_file_handle filehandle;
	size_t filesize;
	bool used;
};

#define numcachedfiles 16

static cachedfile cachedfiles[numcachedfiles];
static int cachedfileindex = 0;

const char * safedequote(char * str);
#define dequote safedequote
extern string thisfilename;

// Opens a file, trying to open it from cache first

extern virtual_filesystem* filesystem;

cachedfile * opencachedfile(string fname, bool should_error)
{
	cachedfile * cachedfilehandle = nullptr;

	// RPG Hacker: Only using a combined path here because that should
	// hopefully result in a unique string for every file, whereas
	// fname could be a relative path, which isn't guaranteed to be unique.
	// Note that this does not affect how we open the file - this is
	// handled by the filesystem and uses our include paths etc.
	string combinedname = S filesystem->create_absolute_path(dir(thisfilename), fname);

	for (int i = 0; i < numcachedfiles; i++)
	{
		if (cachedfiles[i].used && cachedfiles[i].filename == combinedname)
		{
			cachedfilehandle = &cachedfiles[i];
			break;
		}
	}

	if (cachedfilehandle == nullptr)
	{

		if (cachedfiles[cachedfileindex].used)
		{
			filesystem->close_file(cachedfiles[cachedfileindex].filehandle);
			cachedfiles[cachedfileindex].filehandle = INVALID_VIRTUAL_FILE_HANDLE;
			cachedfiles[cachedfileindex].used = false;
		}

		cachedfilehandle = &cachedfiles[cachedfileindex];
	}

	if (cachedfilehandle != nullptr)
	{
		if (!cachedfilehandle->used)
		{
			cachedfilehandle->filehandle = filesystem->open_file(fname, thisfilename);
			if (cachedfilehandle->filehandle != INVALID_VIRTUAL_FILE_HANDLE)
			{
				cachedfilehandle->used = true;
				cachedfilehandle->filename = combinedname;
				cachedfilehandle->filesize = filesystem->get_file_size(cachedfilehandle->filehandle);
				cachedfileindex++;
				// randomdude999: when we run out of cached files, just start overwriting ones from the start
				if (cachedfileindex >= numcachedfiles) cachedfileindex = 0;
			}
		}
	}

	if ((cachedfilehandle == nullptr || cachedfilehandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) && should_error)
	{
		error("Failed to open file.");
	}

	return cachedfilehandle;
}


void closecachedfiles()
{
	for (int i = 0; i < numcachedfiles; i++)
	{
		if (cachedfiles[i].used)
		{
			if (cachedfiles[i].filehandle != INVALID_VIRTUAL_FILE_HANDLE)
			{
				filesystem->close_file(cachedfiles[i].filehandle);
				cachedfiles[i].filehandle = INVALID_VIRTUAL_FILE_HANDLE;
			}

			cachedfiles[i].used = false;
		}
	}

	cachedfileindex = 0;
}


// Some helpers so that we can pass variable parameter types to functions

enum funcparamtype {
	Type_String,
	Type_Double
};

struct funcparam {
	funcparamtype type;
	union valueunion {
		const char * stringvalue;
		double longdoublevalue;

		valueunion() {}
		~valueunion() {}
	} value;

	funcparam()
	{
		type = Type_Double;
		value.longdoublevalue = 0.0;
	}

	funcparam(const char * in)
	{
		type = Type_String;
		value.stringvalue = in;
	}

	funcparam(double in)
	{
		type = Type_Double;
		value.longdoublevalue = in;
	}
};


char ** funcargnames;
funcparam * funcargvals;
int numuserfuncargs;


int snestopc_pick(int addr);

#define validateparam(inparam, paramindex, expectedtype)     \
	if (inparam.type != expectedtype)            \
	{                                             \
		error("Wrong type for argument " #paramindex ", expected " #expectedtype ".");   \
	}

static double validaddr(const funcparam& in, const funcparam& len)
{
	validateparam(in, 0, Type_Double);
	validateparam(len, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0 || addr+len.value.longdoublevalue-1>romlen_r) return 0;
	else return 1;
}

static double read1(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) error("read1(): Address doesn't map to ROM.");
	else if (addr+1>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     ;
}

static double read2(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) error("read2(): Address doesn't map to ROM.");
	else if (addr+2>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]    |
			(romdata_r[addr+1]<< 8);
}

static double read3(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) error("read3(): Address doesn't map to ROM.");
	else if (addr+3>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
}

static double read4(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) error("read4(): Address doesn't map to ROM.");
	else if (addr+4>romlen_r) error("Address out of bounds.");
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16)|
			(romdata_r[addr+3]<<24);
}

static double read1s(const funcparam& in, const funcparam& def)
{
	validateparam(in, 0, Type_Double);
	validateparam(def, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) return def.value.longdoublevalue;
	else if (addr+0>romlen_r) return def.value.longdoublevalue;
	else return
			 romdata_r[addr  ]     ;
}

static double read2s(const funcparam& in, const funcparam& def)
{
	validateparam(in, 0, Type_Double);
	validateparam(def, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) return def.value.longdoublevalue;
	else if (addr+1>romlen_r) return def.value.longdoublevalue;
	else return
			 romdata_r[addr  ]    |
			(romdata_r[addr+1]<< 8);
}

static double read3s(const funcparam& in, const funcparam& def)
{
	validateparam(in, 0, Type_Double);
	validateparam(def, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) return def.value.longdoublevalue;
	else if (addr+2>romlen_r) return def.value.longdoublevalue;
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
}

static double read4s(const funcparam& in, const funcparam& def)
{
	validateparam(in, 0, Type_Double);
	validateparam(def, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) return def.value.longdoublevalue;
	else if (addr+3>romlen_r) return def.value.longdoublevalue;
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16)|
			(romdata_r[addr+3]<<24);
}

static double readfilefunc(const funcparam& fname, const funcparam& offset, long numbytes)
{
	validateparam(fname, 0, Type_String);
	validateparam(offset, 1, Type_Double);
	if (numbytes <=0 || numbytes > 4) error("Can only read chunks of 1 to 4 bytes.");
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, true);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) error("Failed to open file.");
	if ((long)offset.value.longdoublevalue < 0 || (size_t)offset.value.longdoublevalue + (size_t)numbytes > fhandle->filesize) error("File read offset out of bounds.");
	unsigned char readdata[4] = { 0, 0, 0, 0 };
	filesystem->read_file(fhandle->filehandle, readdata, (size_t)offset.value.longdoublevalue, (size_t)numbytes);
	return
		 readdata[0]       |
		(readdata[1] << 8) |
		(readdata[2] << 16)|
		(readdata[3] << 24);
}

static double readfilefuncs(const funcparam& fname, const funcparam& offset, const funcparam& def, long numbytes)
{
	validateparam(fname, 0, Type_String);
	validateparam(offset, 1, Type_Double);
	validateparam(def, 2, Type_Double);
	if (numbytes <= 0 || numbytes > 4) error("Can only read chunks of 1 to 4 bytes.");
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return def.value.longdoublevalue;
	if ((long)offset.value.longdoublevalue < 0 || (size_t)offset.value.longdoublevalue + (size_t)numbytes > fhandle->filesize) return def.value.longdoublevalue;
	unsigned char readdata[4] = { 0, 0, 0, 0 };
	filesystem->read_file(fhandle->filehandle, readdata, (size_t)offset.value.longdoublevalue, (size_t)numbytes);
	return
		readdata[0] |
		(readdata[1] << 8) |
		(readdata[2] << 16) |
		(readdata[3] << 24);
}

static double canreadfilefunc(const funcparam& fname, const funcparam& offset, const funcparam& numbytes)
{
	validateparam(fname, 0, Type_String);
	validateparam(offset, 1, Type_Double);
	validateparam(numbytes, 2, Type_Double);
	if ((long)numbytes.value.longdoublevalue <= 0) error("Number of bytes to check must be > 0.");
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return 0;
	if ((long)offset.value.longdoublevalue < 0 || (size_t)offset.value.longdoublevalue + (size_t)numbytes.value.longdoublevalue > fhandle->filesize) return 0;
	return 1;
}

// returns 0 if the file is OK, 1 if the file doesn't exist, 2 if it couldn't be opened for some other reason
static double getfilestatus(const funcparam& fname)
{
	validateparam(fname, 0, Type_String);
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE)
	{
		if (filesystem->get_last_error() == vfe_doesnt_exist)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
	return 0;
}

// Returns the size of the specified file.
static double filesizefunc(const funcparam& fname)
{
	validateparam(fname, 0, Type_String);
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return -1;
	return (double)fhandle->filesize;
}

// Checks whether the specified define is defined.
static double isdefinedfunc(const funcparam& definename)
{
	validateparam(definename, 0, Type_String);
	return defines.exists(definename.value.stringvalue);
}

// RPG Hacker: What exactly makes this function overly complicated, you ask?
// Well, it converts a double to a string and then back to a double.
// It was the quickest reliable solution I could find, though, so there's that.
static double overlycomplicatedround(const funcparam& number, const funcparam& decimalplaces)
{
	validateparam(number, 0, Type_Double);
	validateparam(decimalplaces, 1, Type_Double);

	// Hue hue hue... ass!
	// OK, sorry, I apologize.
	string asstring = ftostrvar(number.value.longdoublevalue, (int)decimalplaces.value.longdoublevalue);

	// Some hacky shenanigans with variables going on here
	const char * strbackup = str;
	str = asstring;
	double asdouble = (double)getnum();
	str = strbackup;

	return asdouble;
}

static int struct_size(const char *name)
{
	snes_struct structure;
	if(!structs.exists(name)) error("Struct not found.");
	structure = structs.find(name);
	return structure.struct_size;
}

static int object_size(const char *name)
{
	snes_struct structure;
	if(!structs.exists(name)) error("Struct not found.");
	structure = structs.find(name);
	return structure.object_size;
}


extern chartabledata table;

// RPG Hacker: Kind of a hack, but whatever, it's the simplest solution
static char errorstringbuffer[256];

#define maxint(a, b) ((unsigned int)a > (unsigned int)b ? (unsigned int)a : (unsigned int)b)

// Check if the argument is simply a parent function parameter being passed through.
// If so return the index of the parameter, otherwise return -1.
// Update source if it is a parameter being pass through.
int check_passthough_argument(const char** source)
{
	const char * current=str;
	while (isalnum(*current) || *current == '_' || *current == '.') current++;
	
	size_t len=(size_t)(current-*source);
	for (int i=0;i<numuserfuncargs;i++)
	{
		if (!strncmp(*source, funcargnames[i], (size_t)maxint(len, strlen(funcargnames[i]))))
		{
			const char* pos= *source+len;
			while (*pos==' ') pos++;
			if(*pos == ',' || *pos == ')')
			{
				*source = pos;
				return i;
			}
			break;				
		}
	}
	return -1;
}


static double getnumcore()
{
	if (*str=='(')
	{
		str++;
		double rval=eval(0);
		if (*str!=')') error("Mismatched parentheses.");
		str++;
		return rval;
	}
	if (*str=='$')
	{
		if (!isxdigit(str[1])) error("Invalid hex value.");
		if (tolower(str[2])=='x') return -42;//let str get an invalid value so it'll throw an invalid operator later on
		return strtoul(str+1, (char**)&str, 16);
	}
	if (*str=='%')
	{
		if (str[1]!='0' && str[1]!='1') error("Invalid binary value.");
		return strtoul(str+1, (char**)&str, 2);
	}
	if (*str=='\'')
	{
		if (!str[1] || str[2]!='\'') error("Invalid character.");
		unsigned int rval=table.table[(unsigned char)str[1]];
		str+=3;
		return rval;
	}
	if (isdigit(*str))
	{
		return strtod(str, (char**)&str);
	}
	if (isalpha(*str) || *str=='_' || *str=='.' || *str=='?')
	{
		const char * start=str;
		while (isalnum(*str) || *str == '_' || *str == '.') str++;
		int len=(int)(str-start);
		while (*str==' ') str++;
		if (*str=='(')
		{
			str++;
			while (*str==' ') str++;

			if (!strncasecmp(start, "sizeof", (size_t)len)) {
				string label;
				while (*str == ' ') str++;
				while (isalnum(*str) || *str == '.') label += *(str++);
				//printf("%s\n", (const char*)label);
				if (*(str++) != ')') error("Malformed sizeof call.");
				return struct_size(label);
			}
			if (!strncasecmp(start, "objectsize", (size_t)len)) {
				string label;
				while (*str == ' ') str++;
				while (isalnum(*str) || *str == '.') label += *(str++);
				//printf("%s\n", (const char*)label);
				if (*(str++) != ')') error("Malformed objectsize call.");
				return object_size(label);
			}

			// RPG Hacker: This is only here to assure that all strings are still
			// alive in memory when we call our functions further down
			autoarray<string> stringparams;
			int numstrings = 0;
			autoarray<funcparam> params;
			int numparams=0;
			if (*str!=')')
			{
				while (true)
				{
					while (*str==' ') str++;
					int pass_arg = check_passthough_argument(&str);
					if (pass_arg != -1)
					{
						params[numparams++] = funcargvals[pass_arg];
					}
					else if (*str=='"')
					{
						const char * strpos = str;
						str++;
						while (*str!='"' && *str!='\0' && *str!='\n') str++;
						if (*str == '"')
						{
							params[numparams].type = Type_String;
							string tempname(strpos, (int)(str - strpos + 1));
							stringparams[numstrings] = string(dequote(&tempname[0]));
							params[numparams++].value.stringvalue = stringparams[numstrings];
							numstrings++;
							str++;
						}
						// RPG Hacker: AFAIK, this is never actually triggered, since unmatched quotes are already detected earlier,
						// but since it does no harm here, I'll keep it in, just to be safe
						else error("String literal not terminated.");
					}
					else
					{
						params[numparams].type = Type_Double;
						params[numparams++].value.longdoublevalue = eval(0);
					}
					while (*str==' ') str++;
					if (*str==',')
					{
						str++;
						continue;
					}
					if (*str==')')
					{
						str++;
						break;
					}
					error("Malformed function call.");
				}
			}
			double rval;
			for (int i=0;i<numuserfunc;i++)
			{
				if ((int)strlen(userfunc[i].name)==len && !strncmp(start, userfunc[i].name, (size_t)len))
				{
					if (userfunc[i].numargs!=numparams) error("Wrong number of parameters to function.");
					char ** oldfuncargnames=funcargnames;
					funcparam * oldfuncargvals=funcargvals;
					const char * oldstr=str;
					int oldnumuserfuncargs=numuserfuncargs;
					funcargnames=userfunc[i].arguments;
					funcargvals=params;
					str=userfunc[i].content;
					numuserfuncargs=numparams;
					rval=eval(0);
					funcargnames=oldfuncargnames;
					funcargvals=oldfuncargvals;
					str=oldstr;
					numuserfuncargs=oldnumuserfuncargs;
					return rval;
				}
			}
			if (*str=='_') str++;

			// RPG Hacker: Originally, these macros used "len" in place of "maxint(len, strlen(name))"
			// This caused Asar to recognize "canread()" as "canread1()", for example, so I changed it to this
#define func(name, numpar, code, hasfurtheroverloads)                                   \
					if (!strncasecmp(start, name, maxint(len, strlen(name))))                      \
					{                                                        \
						if (numparams==numpar) return (code);                  \
						else if (!hasfurtheroverloads) error("Wrong number of parameters to function."); \
					}
#define wrappedfunc1(name, inparam, code, hasfurtheroverloads)                             \
					if (!strncasecmp(start, name, maxint(len, strlen(name))))                      \
					{                                                        \
						if (numparams==1)                                \
						{                                                     \
							validateparam(inparam, 0, Type_Double);      \
							return (code);                                    \
						}                                                      \
						else if (!hasfurtheroverloads) error("Wrong number of parameters to function."); \
					}
#define wrappedfunc2(name, inparam1, inparam2, code, hasfurtheroverloads)                             \
					if (!strncasecmp(start, name, maxint(len, strlen(name))))                      \
					{                                                        \
						if (numparams==2)                                \
						{                                                     \
							validateparam(inparam1, 0, Type_Double);      \
							validateparam(inparam2, 0, Type_Double);      \
							return (code);                                    \
						}                                                      \
						else if (!hasfurtheroverloads) error("Wrong number of parameters to function."); \
					}
#define wrappedfunc3(name, inparam1, inparam2, inparam3, code, hasfurtheroverloads)                             \
					if (!strncasecmp(start, name, maxint(len, strlen(name))))                      \
					{                                                        \
						if (numparams==3)                                \
						{                                                     \
							validateparam(inparam1, 0, Type_Double);      \
							validateparam(inparam2, 0, Type_Double);      \
							validateparam(inparam3, 0, Type_Double);      \
							return (code);                                    \
						}                                                      \
						else if (!hasfurtheroverloads) error("Wrong number of parameters to function."); \
					}
			wrappedfunc1("sqrt", params[0], sqrt((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("sin", params[0], sin((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("cos", params[0], cos((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("tan", params[0], tan((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("asin", params[0], asin((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("acos", params[0], acos((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("atan", params[0], atan((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("arcsin", params[0], asin((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("arccos", params[0], acos((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("arctan", params[0], atan((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("log", params[0], log((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("log10", params[0], log10((double)params[0].value.longdoublevalue), false);
			wrappedfunc1("log2", params[0], log((double)params[0].value.longdoublevalue)/log(2.0), false);

			func("read1", 1, read1(params[0]), true);
			func("read2", 1, read2(params[0]), true);
			func("read3", 1, read3(params[0]), true);
			func("read4", 1, read4(params[0]), true);
			func("read1", 2, read1s(params[0], params[1]), false);
			func("read2", 2, read2s(params[0], params[1]), false);
			func("read3", 2, read3s(params[0], params[1]), false);
			func("read4", 2, read4s(params[0], params[1]), false);
			func("canread1", 1, validaddr(params[0], funcparam(1.0)), false);
			func("canread2", 1, validaddr(params[0], funcparam(2.0)), false);
			func("canread3", 1, validaddr(params[0], funcparam(3.0)), false);
			func("canread4", 1, validaddr(params[0], funcparam(4.0)), false);
			func("canread", 2, validaddr(params[0], params[1]), false);
			func("readfile1", 2, readfilefunc(params[0], params[1], 1), true);
			func("readfile2", 2, readfilefunc(params[0], params[1], 2), true);
			func("readfile3", 2, readfilefunc(params[0], params[1], 3), true);
			func("readfile4", 2, readfilefunc(params[0], params[1], 4), true);
			func("readfile1", 3, readfilefuncs(params[0], params[1], params[2], 1), false);
			func("readfile2", 3, readfilefuncs(params[0], params[1], params[2], 2), false);
			func("readfile3", 3, readfilefuncs(params[0], params[1], params[2], 3), false);
			func("readfile4", 3, readfilefuncs(params[0], params[1], params[2], 4), false);
			func("canreadfile1", 2, canreadfilefunc(params[0], params[1], funcparam(1.0)), false);
			func("canreadfile2", 2, canreadfilefunc(params[0], params[1], funcparam(2.0)), false);
			func("canreadfile3", 2, canreadfilefunc(params[0], params[1], funcparam(3.0)), false);
			func("canreadfile4", 2, canreadfilefunc(params[0], params[1], funcparam(4.0)), false);
			func("canreadfile", 3, canreadfilefunc(params[0], params[1], params[2]), false);
			func("filesize", 1, filesizefunc(params[0]), false);
			func("getfilestatus", 1, getfilestatus(params[0]), false);

			func("defined", 1, isdefinedfunc(params[0]), false);

			wrappedfunc1("snestopc", params[0], snestopc((int)params[0].value.longdoublevalue), false);
			wrappedfunc1("pctosnes", params[0], pctosnes((int)params[0].value.longdoublevalue), false);

			wrappedfunc2("max", params[0], params[1], (params[0].value.longdoublevalue > params[1].value.longdoublevalue ? params[0].value.longdoublevalue : params[1].value.longdoublevalue), false);
			wrappedfunc2("min", params[0], params[1], (params[0].value.longdoublevalue < params[1].value.longdoublevalue ? params[0].value.longdoublevalue : params[1].value.longdoublevalue), false);
			wrappedfunc3("clamp", params[0], params[1], params[2], (params[0].value.longdoublevalue < params[1].value.longdoublevalue ? params[1].value.longdoublevalue : (params[0].value.longdoublevalue > params[2].value.longdoublevalue ? params[2].value.longdoublevalue : params[0].value.longdoublevalue)), false);

			wrappedfunc3("safediv", params[0], params[1], params[2], (params[1].value.longdoublevalue == 0.0 ? params[2].value.longdoublevalue : params[0].value.longdoublevalue / params[1].value.longdoublevalue), false);

			wrappedfunc3("select", params[0], params[1], params[2], (params[0].value.longdoublevalue == 0.0 ? params[2].value.longdoublevalue : params[1].value.longdoublevalue), false);
			wrappedfunc1("not", params[0], (params[0].value.longdoublevalue == 0.0 ? 1.0 : 0.0), false);
			wrappedfunc2("equal", params[0], params[1], (params[0].value.longdoublevalue == params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			wrappedfunc2("notequal", params[0], params[1], (params[0].value.longdoublevalue != params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			wrappedfunc2("less", params[0], params[1], (params[0].value.longdoublevalue < params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			wrappedfunc2("lessequal", params[0], params[1], (params[0].value.longdoublevalue <= params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			wrappedfunc2("greater", params[0], params[1], (params[0].value.longdoublevalue > params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			wrappedfunc2("greaterequal", params[0], params[1], (params[0].value.longdoublevalue >= params[1].value.longdoublevalue ? 1.0 : 0.0), false);
			
			wrappedfunc2("and", params[0], params[1], ((params[0].value.longdoublevalue != 0 && params[1].value.longdoublevalue != 0) ? 1.0 : 0.0), false);
			wrappedfunc2("or", params[0], params[1], ((params[0].value.longdoublevalue != 0 || params[1].value.longdoublevalue != 0) ? 1.0 : 0.0), false);
			wrappedfunc2("nand", params[0], params[1], (!(params[0].value.longdoublevalue != 0 && params[1].value.longdoublevalue != 0) ? 1.0 : 0.0), false);
			wrappedfunc2("nor", params[0], params[1], (!(params[0].value.longdoublevalue != 0 || params[1].value.longdoublevalue != 0) ? 1.0 : 0.0), false);
			wrappedfunc2("xor", params[0], params[1], (((params[0].value.longdoublevalue != 0 && params[1].value.longdoublevalue == 0) || (params[0].value.longdoublevalue == 0 && params[1].value.longdoublevalue != 0)) ? 1.0 : 0.0), false);
			
			func("round", 2, overlycomplicatedround(params[0], params[1]), false);
#undef func
#undef wrappedfunc1
#undef wrappedfunc2
#undef wrappedfunc3
			error("Unknown function.");
		}
		else
		{
			for (int i=0;i<numuserfuncargs;i++)
			{
				if (!strncmp(start, funcargnames[i], (size_t)maxint(len, strlen(funcargnames[i]))))
				{
					if (funcargvals[i].type == Type_Double)
						return funcargvals[i].value.longdoublevalue;
					else
					{
						string tmp("Wrong type for argument ");
						tmp += string(i);
						tmp += ", expected Type_Double.";
						strcpy(errorstringbuffer, tmp);
						error(errorstringbuffer);
					}
				}
			}
			foundlabel=true;

			const char *old_start = start;
			int i=(int)labelval(&start);
			bool scope_passed = false;
			bool subscript_passed = false;
			while (str < start)
			{
				if (*str == '.') scope_passed = true;
				if (*(str++) == '[')
				{
					if (subscript_passed)
					{
						error("Multiple subscript operators is invalid");
						break;
					}
					subscript_passed = true;
					if (scope_passed)
					{
						error("Invalid array subscript after first scope resolution.");
						break;
					}
					string struct_name = substr(old_start, (int)(str - old_start - 1));
					i += (int)(eval(0) * object_size(struct_name));
				}
			}

			str=start;
			if (i==-1) forwardlabel=true;
			return (int)i&0xFFFFFF;
		}
	}
	error("Invalid number.");
}

static double sanitize(double val)
{
	if (val!=val) error("NaN encountered.");
	if (math_round) return trunc(val); // originally used int cast, but that broke numbers > $8000_0000
	return val;
}

static double getnum()
{
	while (*str==' ') str++;
#define prefix(name, func) if (!strncasecmp(str, name, strlen(name))) { str+=strlen(name); double val=getnum(); return sanitize(func); }
	prefix("-", -val);
	prefix("~", ~(int)val);
	prefix("+", val);
	if (emulatexkas) prefix("#", val);
#undef prefix
	return sanitize(getnumcore());
}


string posneglabelname(const char ** input, bool define);

static double eval(int depth)
{
	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);

	if (posnegname.truelen() > 0)
	{
		if (*posneglabel != '\0' && *posneglabel != ')') goto notposneglabel;

		str = posneglabel;

		foundlabel=true;
		if (*(posneglabel-1) == '+') forwardlabel=true;
		return labelval(posnegname) & 0xFFFFFF;
	}
notposneglabel:
	recurseblock rec;
	double left=getnum();
	double right;
	while (*str==' ') str++;
	while (*str && *str != ')' && *str != ','&& *str != ']')
	{
		while (*str==' ') str++;
		if (math_round) left=(int)left;
#define oper(name, thisdepth, contents)      \
			if (!strncmp(str, name, strlen(name))) \
			{                                      \
				if (math_pri)                        \
				{                                    \
					if (depth<=thisdepth)              \
					{                                  \
						str+=strlen(name);               \
						right=eval(thisdepth+1);         \
					}                                  \
					else return left;                  \
				}                                    \
				else                                 \
				{                                    \
					str+=strlen(name);                 \
					right=getnum();                    \
				}                                    \
				left=sanitize(contents);             \
				continue;                            \
			}
		oper("**", 4, pow((double)left, (double)right));
		oper("*", 3, left*right);
		oper("/", 3, right?left/right:error("Division by zero."));
		oper("%", 3, right?fmod((double)left, (double)right):error("Modulos by zero."));
		oper("+", 2, left+right);
		oper("-", 2, left-right);
		oper("<<", 1, (unsigned int)left<<(unsigned int)right);
		oper(">>", 1, (unsigned int)left>>(unsigned int)right);
		oper("&", 0, (unsigned int)left&(unsigned int)right);
		oper("|", 0, (unsigned int)left|(unsigned int)right);
		oper("^", 0, (unsigned int)left^(unsigned int)right);
		error("Unknown operator.");
#undef oper
	}
	return left;
}

//static autoptr<char*> freeme;
double math(const char * s, const char ** e)
{
	//free(freeme);
	//freeme=NULL;
	foundlabel=false;
	forwardlabel=false;
	try
	{
		str=s;
		double rval=eval(0);
		if (*str)
		{
			if (*str==',') error("Invalid input.");
			else error("Mismatched parentheses.");
		}
		*e=NULL;
		return rval;
	}
	catch (const char * error)
	{
		*e=error;
		return 0;
	}
	//catch (string& error)
	//{
	//	freeme=strdup(error);
	//	*e=error;
	//	return 0;
	//}
}

void initmathcore()
{
	//not needed
}

void deinitmathcore()
{
	userfunc.reset();
	numuserfunc = 0;
}
