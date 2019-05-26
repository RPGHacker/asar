//Don't try using this in your own project, it's got a lot of Asar-specific tweaks. Use mathlib.cpp instead.
#include "platform/file-helpers.h"
#include "std-includes.h"
#include "autoarray.h"
#include "assocarr.h"
#include "libstr.h"
#include "libsmw.h"
#include "asar.h"
#include "virtualfile.h"
#include "assembleblock.h"
#include "asar_math.h"
#include <math.h>

bool math_pri=true;
bool math_round=false;

static const char * str;

static double getnumcore();
static double getnum();
static double eval(int depth);

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
static autoarray<funcdat> userfunc;
static int numuserfunc=0;

void createuserfunc(const char * name, const char * arguments, const char * content)
{
	if (!confirmqpar(content)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
	for (int i=0;i<numuserfunc;i++)
	{
		if (!strcmp(name, userfunc[i].name))
		{
			asar_throw_error(0, error_type_block, error_id_function_redefined, name);
		}
	}
	funcdat& thisone=userfunc[numuserfunc];
	thisone.name= duplicate_string(name);
	thisone.argbuf= duplicate_string(arguments);
	thisone.arguments=qsplit(thisone.argbuf, ",", &(thisone.numargs));
	thisone.content= duplicate_string(content);
	for (int i=0;thisone.arguments[i];i++)
	{
		if (!confirmname(thisone.arguments[i]))
		{
			userfunc.remove(numuserfunc);
			asar_throw_error(0, error_type_block, error_id_invalid_param_name);
		}
	}
	numuserfunc++;
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

// Opens a file, trying to open it from cache first

static cachedfile * opencachedfile(string fname, bool should_error)
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
		asar_throw_error(1, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), fname.str);
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


static char ** funcargnames;
static funcparam * funcargvals;
static int numuserfuncargs;

#define validateparam(inparam, paramindex, expectedtype)     \
	if (inparam.type != expectedtype)            \
	{                                             \
		asar_throw_error(1, error_type_block, error_id_math_invalid_type, #paramindex, #expectedtype);   \
	}

static double validaddr(const funcparam& in, const funcparam& len)
{
	validateparam(in, 0, Type_Double);
	validateparam(len, 1, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0 || addr+len.value.longdoublevalue-1>=romlen_r) return 0;
	else return 1;
}

static double read1(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) asar_throw_error(1, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)in.value.longdoublevalue) + " in read1()").str);
	else if (addr+1>romlen_r) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, (hex6((unsigned int)in.value.longdoublevalue) + " in read1()").str);
	else return
			 romdata_r[addr  ]     ;
	return 0.0;
}

static double read2(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) asar_throw_error(1, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)in.value.longdoublevalue) + " in read2()").str);
	else if (addr+2>romlen_r) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, (hex6((unsigned int)in.value.longdoublevalue) + " in read2()").str);
	else return
			 romdata_r[addr  ]    |
			(romdata_r[addr+1]<< 8);
	return 0.0;
}

static double read3(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) asar_throw_error(1, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)in.value.longdoublevalue) + " in read3()").str);
	else if (addr+3>romlen_r) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, (hex6((unsigned int)in.value.longdoublevalue) + " in read3()").str);
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16);
	return 0.0;
}

static double read4(const funcparam& in)
{
	validateparam(in, 0, Type_Double);
	int addr=snestopc_pick((int)in.value.longdoublevalue);
	if (addr<0) asar_throw_error(1, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)in.value.longdoublevalue) + " in read4()").str);
	else if (addr+4>romlen_r) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, (hex6((unsigned int)in.value.longdoublevalue) + " in read4()").str);
	else return
			 romdata_r[addr  ]     |
			(romdata_r[addr+1]<< 8)|
			(romdata_r[addr+2]<<16)|
			(romdata_r[addr+3]<<24);
	return 0.0;
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
	if (numbytes <= 0 || numbytes > 4) asar_throw_error(1, error_type_block, error_id_readfile_1_to_4_bytes);
	cachedfile * fhandle = opencachedfile(fname.value.stringvalue, true);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(1, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), fname.value.stringvalue);
	if ((long)offset.value.longdoublevalue < 0 || (size_t)offset.value.longdoublevalue + (size_t)numbytes > fhandle->filesize) asar_throw_error(1, error_type_block, error_id_file_offset_out_of_bounds, dec((int)offset.value.longdoublevalue).str, fname.value.stringvalue);
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
	if (numbytes <= 0 || numbytes > 4) asar_throw_error(1, error_type_block, error_id_readfile_1_to_4_bytes);
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
	if ((long)numbytes.value.longdoublevalue <= 0) asar_throw_error(1, error_type_block, error_id_canreadfile_0_bytes);
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
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(1, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), fname.value.stringvalue);
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
	if(!structs.exists(name)) asar_throw_error(1, error_type_block, error_id_struct_not_found, name);
	structure = structs.find(name);
	return structure.struct_size;
}

static int object_size(const char *name)
{
	snes_struct structure;
	if(!structs.exists(name)) asar_throw_error(1, error_type_block, error_id_struct_not_found, name);
	structure = structs.find(name);
	return structure.object_size;
}

static double structsizefunc(const funcparam& structname)
{
	validateparam(structname, 0, Type_String);
	return (double)struct_size(structname.value.stringvalue);
}

static double objectsizefunc(const funcparam& structname)
{
	validateparam(structname, 0, Type_String);
	return (double)object_size(structname.value.stringvalue);
}

static double stringsequalfunc(const funcparam& string1, const funcparam& string2)
{
	validateparam(string1, 0, Type_String);
	validateparam(string1, 1, Type_String);
	return (strcmp(string1.value.stringvalue, string2.value.stringvalue) == 0 ? 1.0 : 0.0);
}

static double stringsequalinsensitivefunc(const funcparam& string1, const funcparam& string2)
{
	validateparam(string1, 0, Type_String);
	validateparam(string1, 1, Type_String);
	return (stricmp(string1.value.stringvalue, string2.value.stringvalue) == 0 ? 1.0 : 0.0);
}


#define maxint(a, b) ((unsigned int)a > (unsigned int)b ? (unsigned int)a : (unsigned int)b)

// Check if the argument is simply a parent function parameter being passed through.
// If so return the index of the parameter, otherwise return -1.
// Update source if it is a parameter being pass through.
static int check_passthough_argument(const char** source)
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
		if (*str != ')') asar_throw_error(1, error_type_block, error_id_mismatched_parentheses);
		str++;
		return rval;
	}
	if (*str=='$')
	{
		if (!isxdigit(str[1])) asar_throw_error(1, error_type_block, error_id_invalid_hex_value);
		if (tolower(str[2])=='x') return -42;//let str get an invalid value so it'll throw an invalid operator later on
		return strtoul(str+1, const_cast<char**>(&str), 16);
	}
	if (*str=='%')
	{
		if (str[1] != '0' && str[1] != '1') asar_throw_error(1, error_type_block, error_id_invalid_binary_value);
		return strtoul(str+1, const_cast<char**>(&str), 2);
	}
	if (*str=='\'')
	{
		if (!str[1] || str[2] != '\'') asar_throw_error(1, error_type_block, error_id_invalid_character);
		unsigned int rval=table.table[(unsigned char)str[1]];
		str+=3;
		return rval;
	}
	if (isdigit(*str))
	{
		const char* end = str;
		while (isdigit(*end) || *end == '.') end++;
		string number;
		number.assign(str, (int)(end - str));
		str = end;
		return atof(number);
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
							stringparams[numstrings] = string(safedequote(&tempname[0]));
							params[numparams++].value.stringvalue = stringparams[numstrings];
							numstrings++;
							str++;
						}
						// RPG Hacker: AFAIK, this is never actually triggered, since unmatched quotes are already detected earlier,
						// but since it does no harm here, I'll keep it in, just to be safe
						else asar_throw_error(1, error_type_block, error_id_string_literal_not_terminated);
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
					asar_throw_error(1, error_type_block, error_id_malformed_function_call);
				}
			}
			double rval;
			for (int i=0;i<numuserfunc;i++)
			{
				if ((int)strlen(userfunc[i].name)==len && !strncmp(start, userfunc[i].name, (size_t)len))
				{
					if (userfunc[i].numargs != numparams) asar_throw_error(1, error_type_block, error_id_wrong_num_parameters);
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
						else if (!hasfurtheroverloads) asar_throw_error(1, error_type_block, error_id_wrong_num_parameters); \
					}
#define wrappedfunc1(name, inparam, code, hasfurtheroverloads)                             \
					if (!strncasecmp(start, name, maxint(len, strlen(name))))                      \
					{                                                        \
						if (numparams==1)                                \
						{                                                     \
							validateparam(inparam, 0, Type_Double);      \
							return (code);                                    \
						}                                                      \
						else if (!hasfurtheroverloads) asar_throw_error(1, error_type_block, error_id_wrong_num_parameters); \
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
						else if (!hasfurtheroverloads) asar_throw_error(1, error_type_block, error_id_wrong_num_parameters); \
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
						else if (!hasfurtheroverloads) asar_throw_error(1, error_type_block, error_id_wrong_num_parameters); \
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

			func("sizeof", 1, structsizefunc(params[0]), false);
			func("objectsize", 1, objectsizefunc(params[0]), false);

			func("stringsequal", 2, stringsequalfunc(params[0], params[1]), false);
			func("stringsequalnocase", 2, stringsequalinsensitivefunc(params[0], params[1]), false);

#undef func
#undef wrappedfunc1
#undef wrappedfunc2
#undef wrappedfunc3
			asar_throw_error(1, error_type_block, error_id_function_not_found, start);
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
						asar_throw_error(1, error_type_block, error_id_math_invalid_type, string(i).str, "Type_Double");
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
						asar_throw_error(1, error_type_block, error_id_multiple_subscript_operators);
						break;
					}
					subscript_passed = true;
					if (scope_passed)
					{
						asar_throw_error(1, error_type_block, error_id_invalid_subscript);
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
	asar_throw_error(1, error_type_block, error_id_invalid_number);
	return 0.0;
}

static double sanitize(double val)
{
	if (val != val) asar_throw_error(1, error_type_block, error_id_nan);
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

uint32_t getnum(const char* instr)
{
	// randomdude999: perform manual bounds-checking and 2's complement,
	// to prevent depending on UB
	double num = math(instr);
	if(num < 0) {
		// manual 2's complement
		if((-num) > (double)UINT32_MAX) {
			// out of bounds, return closest inbounds value
			// (this value is the most negative value possible)
			return ((uint32_t)INT32_MAX)+1;
		}
		return ~((uint32_t)(-num))+1;
	} else {
		if(num > (double)UINT32_MAX) {
			// out of bounds, return closest inbounds value
			return UINT32_MAX;
		}
		return (uint32_t)num;
	}
}

int64_t getnum64(const char* instr)
{
	// randomdude999: perform manual bounds-checking
	// to prevent depending on UB
	double num = math(instr);
	if(num < (double)INT64_MIN) {
		return INT64_MIN;
	} else if(num > (double)INT64_MAX) {
		return INT64_MAX;
	}
	return (int64_t)num;
}

// RPG Hacker: Same function as above, but doesn't truncate our number via int conversion
double getnumdouble(const char * instr)
{
	return math(instr);
}


static double oper_wrapped_throw(asar_error_id errid)
{
	asar_throw_error(1, error_type_block, errid);
	return 0.0;
}

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
		// why was this an int cast
		if (math_round) left=trunc(left);
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
		oper("/", 3, right != 0.0 ? left / right : oper_wrapped_throw(error_id_division_by_zero));
		oper("%", 3, right != 0.0 ? fmod((double)left, (double)right) : oper_wrapped_throw(error_id_modulo_by_zero));
		oper("+", 2, left+right);
		oper("-", 2, left-right);
		oper("<<", 1, (uint64_t)left<<(uint64_t)right);
		oper(">>", 1, (uint64_t)left>>(uint64_t)right);
		oper("&", 0, (uint64_t)left&(uint64_t)right);
		oper("|", 0, (uint64_t)left|(uint64_t)right);
		oper("^", 0, (uint64_t)left^(uint64_t)right);
		asar_throw_error(1, error_type_block, error_id_unknown_operator);
#undef oper
	}
	return left;
}

//static autoptr<char*> freeme;
double math(const char * s)
{
	//free(freeme);
	//freeme=NULL;
	foundlabel=false;
	forwardlabel=false;

	str = s;
	double rval = eval(0);
	if (*str)
	{
		if (*str == ',') asar_throw_error(1, error_type_block, error_id_invalid_input);
		else asar_throw_error(1, error_type_block, error_id_mismatched_parentheses);
	}
	return rval;
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
