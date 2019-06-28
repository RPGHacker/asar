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
#include <functional>
#include <algorithm>

bool math_pri=true;
bool math_round=false;

static const char * str;
//save before calling eval if needed after
static const char * current_user_function_name;

static double getnumcore();
static double getnum();
static double eval(int depth);

//label (bool foundLabel) (bool forwardLabel)
//userfunction

bool foundlabel;
bool forwardlabel;

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


//only returns alphanumeric (and _) starting with alpha or _
string get_symbol_argument()
{
	while (*str==' ') str++;	//is this proper?  Dunno yet.
	const char * strpos = str;
	if(isalpha(*str) || *str == '_') str++;
	while (isalnum(*str) || *str == '_' || *str == '.') str++;
	if(strpos == str){
		//error nothing was read, this is a placeholder error
		asar_throw_error(1, error_type_block, error_id_string_literal_not_terminated);
	}
	
	string symbol = string(strpos, (int)(str - strpos));
	while (*str==' ') str++;	//eat spaces
	return symbol;
}

string get_string_argument()
{
	while (*str==' ') str++;
	if (*str=='"')
	{
		const char * strpos = str;
		str++;
		while (*str!='"' && *str!='\0' && *str!='\n') str++;
		if (*str == '"')
		{
			string tempname(strpos, (int)(str - strpos + 1));
			str++;
			while (*str==' ') str++;	//eat space
			return string(safedequote(&tempname[0]));
		}
		// RPG Hacker: AFAIK, this is never actually triggered, since unmatched quotes are already detected earlier,
		// but since it does no harm here, I'll keep it in, just to be safe
		else asar_throw_error(1, error_type_block, error_id_string_literal_not_terminated);
	}//make this error a better one later
	
	asar_throw_error(1, error_type_block, error_id_string_literal_not_terminated);
	return ""; //never actually called, but I don't feel like figuring out __attribute__ ((noreturn)) on MSVC
}

double get_double_argument()
{
	while (*str==' ') str++;
	double result = eval(0);
	while (*str==' ') str++; //eat spaces
	return result;
}

//will eat the comma
bool has_next_parameter()
{
	if (*str==',')
	{
		str++;
		return true;
	}
	return false;
}

void require_next_parameter()
{
	if (*str==',')
	{
		str++;
		return;
	}
	asar_throw_error(1, error_type_block, error_id_require_parameter);
}

template <typename F> double asar_unary_wrapper()
{
	return F()(get_double_argument());
}

template <double (*F)(double)> double asar_unary_wrapper()
{
	return F(get_double_argument());
}

//possibly restrict type T in the future....
//first a case for functors
template <typename F> double asar_binary_wrapper()
{
	double first = get_double_argument();
	require_next_parameter();
	return F()(first, get_double_argument());
}
//this could be DRY with if constexpr....oh well
template <double (*F)(double, double)> double asar_binary_wrapper()
{
	double first = get_double_argument();
	require_next_parameter();
	return F(first, get_double_argument());
}

double asar_logical_nand(double a, double b)
{
	return !(a && b);
}


double asar_logical_nor(double a, double b)
{
	return !(a || b);
}


double asar_logical_xor(double a, double b)
{
	return !!a ^ !!b;
}

double asar_max(double a, double b)
{
	return a > b ? a : b;
}

double asar_min(double a, double b)
{
	return a < b ? a : b;
}

double asar_clamp()
{
	double value = get_double_argument();
	require_next_parameter();
	double low = get_double_argument();
	require_next_parameter();
	double high = get_double_argument();
	
	return asar_max(low, asar_min(high, value));
}

double asar_safediv()
{
	double dividend = get_double_argument();
	require_next_parameter();
	double divisor = get_double_argument();
	require_next_parameter();
	double default_value = get_double_argument();
	
	return divisor == 0.0 ? default_value : dividend / divisor;
}

double asar_select()
{
	double selector = get_double_argument();
	require_next_parameter();
	double a = get_double_argument();
	require_next_parameter();
	double b = get_double_argument();
	
	return selector == 0.0 ? b : a;
}

double asar_snestopc_wrapper()
{
	return snestopc(get_double_argument());
}

double asar_pctosnes_wrapper()
{
	return pctosnes(get_double_argument());
}

template <int count> double asar_read()
{
	int target = get_double_argument();
	int addr=snestopc_pick(target);
	if(has_next_parameter())
	{
		double default_value = get_double_argument();
		if (addr<0) return default_value;
		else if (addr+count>romlen_r) return default_value;		
	}
	else
	{
		if (addr<0) asar_throw_error(1, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)target) + " in read function").str);
		else if (addr+count>romlen_r) asar_throw_error(1, error_type_block, error_id_snes_address_out_of_bounds, (hex6(target) + " in read function").str);
	}
	
	unsigned int value = 0;
	for(int i = 0; i < count; i++)
	{
		value |= romdata_r[addr+i] << (8 * i);
	}
	return value;
}

template <int count> double asar_canread()
{
	int length = count;
	if(!length)
	{
		length = get_double_argument();
	}
	int addr=snestopc_pick(get_double_argument());
	if (addr<0 || addr+length-1>=romlen_r) return 0;
	else return 1;
}

template <size_t count> double asar_readfile()
{
	static_assert(count && count <= 4, "invalid count"); //1-4 inclusive
	
	string name = get_string_argument();
	require_next_parameter();
	size_t offset = get_double_argument();
	bool should_error = !has_next_parameter();
	cachedfile * fhandle = opencachedfile(name, should_error);
	if(!should_error)
	{
		double default_value = get_double_argument();
		if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return default_value;
		if (offset < 0 || offset + count > fhandle->filesize) return default_value;
	}
	else
	{		
		if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(1, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name);
		if (offset < 0 || offset + count > fhandle->filesize) asar_throw_error(1, error_type_block, error_id_file_offset_out_of_bounds, dec(offset).str, name);
	}
	
	unsigned char data[4] = { 0, 0, 0, 0 };
	filesystem->read_file(fhandle->filehandle, data, offset, count);
	
	unsigned int value = 0;
	for(size_t i = 0; i < count; i++)
	{
		value |= data[i] << (8 * i);
	}
	
	return value;
}

template <size_t count> double asar_canreadfile()
{
	string name = get_string_argument();
	require_next_parameter();
	size_t offset = get_double_argument();
	size_t length = count;
	if(!count)
	{
		require_next_parameter();
		length = get_double_argument();
	}
	cachedfile * fhandle = opencachedfile(name, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return 0;
	if (offset < 0 || offset + length > fhandle->filesize) return 0;
	return 1;
}

// returns 0 if the file is OK, 1 if the file doesn't exist, 2 if it couldn't be opened for some other reason
static double asar_filestatus()
{
	cachedfile * fhandle = opencachedfile(get_string_argument(), false);
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
static double asar_filesize()
{
	string name = get_string_argument();
	cachedfile * fhandle = opencachedfile(name, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(1, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name);
	return (double)fhandle->filesize;
}

// Checks whether the specified define is defined.
static double asar_isdefined()
{
	return defines.exists(get_string_argument());
}

// RPG Hacker: What exactly makes this function overly complicated, you ask?
// Well, it converts a double to a string and then back to a double.
// It was the quickest reliable solution I could find, though, so there's that.
static double asar_round()
{
	double number = get_double_argument();
	require_next_parameter();

	// Hue hue hue... ass!
	// OK, sorry, I apologize.
	string asstring = ftostrvar(number, get_double_argument());

	// Some hacky shenanigans with variables going on here
	const char * strbackup = str;
	str = asstring;
	double asdouble = (double)getnum();
	str = strbackup;

	return asdouble;
}

static double asar_structsize_wrapper()
{
	return (double)struct_size(get_symbol_argument());
}

static double asar_objectsize_wrapper()
{
	return (double)object_size(get_symbol_argument());
}

static double asar_stringsequal()
{
	string string1 = get_string_argument();
	require_next_parameter();
	return (strcmp(string1, get_string_argument()) == 0 ? 1.0 : 0.0);
}

static double asar_stringsequalnocase()
{
	string string1 = get_string_argument();
	require_next_parameter();
	return (stricmp(string1, get_string_argument()) == 0 ? 1.0 : 0.0);
}

string copy_arg()
{
	if(*str == '"')
	{
		string t = "\"";
		return (t += get_string_argument() + "\"");
	}
	
	string result;
	while(*str != ',' && *str != ')') result += *str++;
	return result;
}

assocarr<double (*)()> functions = 
{
	{"sqrt", asar_unary_wrapper<sqrt>},
	{"sin", asar_unary_wrapper<sin>},
	{"cos", asar_unary_wrapper<cos>},
	{"tan", asar_unary_wrapper<tan>},
	{"asin", asar_unary_wrapper<asin>},
	{"acos", asar_unary_wrapper<acos>},
	{"atan", asar_unary_wrapper<atan>},
	{"arcsin", asar_unary_wrapper<asin>},
	{"arccos", asar_unary_wrapper<acos>},
	{"arctan", asar_unary_wrapper<atan>},
	{"log", asar_unary_wrapper<log>},
	{"log10", asar_unary_wrapper<log10>},
	{"log2", asar_unary_wrapper<log2>},

	{"read1", asar_read<1>},	//This handles the safe and unsafe variant
	{"read2", asar_read<2>},
	{"read3", asar_read<3>},
	{"read4", asar_read<4>},			
	{"canread", asar_canread<0>},
	{"canread1", asar_canread<1>},
	{"canread2", asar_canread<2>},
	{"canread3", asar_canread<3>},
	{"canread4", asar_canread<4>},
	
	{"readfile1", asar_readfile<1>},
	{"readfile2", asar_readfile<2>},
	{"readfile3", asar_readfile<3>},
	{"readfile4", asar_readfile<4>},
	{"canreadfile", asar_canreadfile<0>},
	{"canreadfile1", asar_canreadfile<1>},
	{"canreadfile2", asar_canreadfile<2>},
	{"canreadfile3", asar_canreadfile<3>},
	{"canreadfile4", asar_canreadfile<4>},
	
	{"filesize", asar_filesize},
	{"getfilestatus", asar_filestatus},
	
	{"defined", asar_isdefined},
	
	{"snestopc", asar_snestopc_wrapper},
	{"pctosnes", asar_pctosnes_wrapper},
	
	{"max", asar_binary_wrapper<asar_max>},
	{"min", asar_binary_wrapper<asar_min>},
	{"clamp", asar_clamp},
	
	{"safediv", asar_safediv},
	
	{"select", asar_select},
	{"not", asar_unary_wrapper<std::logical_not<unsigned int>>},
	{"equal", asar_binary_wrapper<std::equal_to<double>>},
	{"notequal", asar_binary_wrapper<std::not_equal_to<double>>},
	{"less", asar_binary_wrapper<std::less<double>>},
	{"lessequal", asar_binary_wrapper<std::less_equal<double>>},
	{"greater", asar_binary_wrapper<std::greater<double>>},
	{"greaterequal", asar_binary_wrapper<std::greater_equal<double>>},
	
	{"and", asar_binary_wrapper<std::logical_and<unsigned int>>},
	{"or", asar_binary_wrapper<std::logical_or<unsigned int>>},
	{"nand", asar_binary_wrapper<asar_logical_nand>},
	{"nor", asar_binary_wrapper<asar_logical_nor>},
	{"xor", asar_binary_wrapper<asar_logical_xor>},
	
	{"round", asar_round},
	
	{"sizeof", asar_structsize_wrapper},
	{"objectsize", asar_objectsize_wrapper},
	
	{"stringsequal", asar_stringsequal},
	{"stringsequalnocase", asar_stringsequalnocase}
};

struct funcdat {
	autoptr<char*> name;
	int numargs;
	autoptr<char*> argbuf;//this one isn't used, it's just to free it up
	autoptr<char**> arguments;
	autoptr<char*> content;
};
static assocarr<funcdat> user_functions;

static double asar_call_user_function()
{
	autoarray<string> args;
	funcdat &user_function = user_functions[current_user_function_name];
	string real_content;
	
	while (*str==' ') str++;
	bool has_next = *str != ')';
	
	for (int i=0;i<user_function.numargs;i++)
	{
		if(!has_next)
		{
			asar_throw_error(0, error_type_block, error_id_expected_parameter, current_user_function_name);
		}
		args[i] = copy_arg();
		has_next = has_next_parameter();
	}
	
	if(has_next)
	{
		asar_throw_error(0, error_type_block, error_id_unexpected_parameter, current_user_function_name);
	}
	
	for(int i=0; user_function.content[i]; i++)
	{
		if(!isalpha(user_function.content[i]) && user_function.content[i] != '_')
		{
			real_content += user_function.content[i];
			continue;
		}
		bool found = false;
		for (int j=0;user_function.arguments[j];j++)
		{
			//this should *always* have a null term or another character after 
			bool potential_arg = stribegin(user_function.content+i, user_function.arguments[j]);
			int next_char = i+strlen(user_function.arguments[j]);
			if(potential_arg && (!isalnum(user_function.content[next_char]) && user_function.content[next_char] != '_'))
			{
				real_content += args[j];
				i = next_char - 1;
				found = true;
			}
		}
		
		if(!found) real_content += user_function.content[i];
	}
	const char * oldstr=str;
	str = (const char *)real_content;
	double result = eval(0);
	str = oldstr;
	return result;
}

void createuserfunc(const char * name, const char * arguments, const char * content)
{
	if (!confirmqpar(content)) asar_throw_error(0, error_type_block, error_id_mismatched_parentheses);
	if(functions.exists(name)) //functions holds both types.
	{		
		asar_throw_error(0, error_type_block, error_id_function_redefined, name);
	}
	funcdat& user_function=user_functions[name];
	user_function.name= duplicate_string(name);
	user_function.argbuf= duplicate_string(arguments);
	user_function.arguments=qsplit(user_function.argbuf, ",", &(user_function.numargs));
	user_function.content= duplicate_string(content);
	for (int i=0;user_function.arguments[i];i++)
	{
		for(int j=0;user_function.arguments[j];j++)
		{
			if(i!=j && !stricmp(user_function.arguments[i], user_function.arguments[j]))
			{
				asar_throw_error(0, error_type_block, error_id_duplicate_param_name, user_function.arguments[i], name);
			}
		}
		if (!confirmname(user_function.arguments[i]))
		{
			user_functions.remove(name);
			asar_throw_error(0, error_type_block, error_id_invalid_param_name);
		}
	}
	
	functions[name] = asar_call_user_function;
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
			double result;
			if (*str!=')')
			{
				while (true)
				{
					while (*str==' ') str++;
					string function_name = lower(string(start, len));
					if(functions.exists(function_name))
					{
						current_user_function_name = function_name;
						result = functions[function_name]();
					}
					else
					{
						str++;
						break;
					}
					
					if (*str==')')
					{
						str++;
						return result;
						break;
					}
					asar_throw_error(1, error_type_block, error_id_malformed_function_call);
				}
			}

			//if (*str=='_') str++; //I don't think this is needed?

			asar_throw_error(1, error_type_block, error_id_function_not_found, start);
		}
		else
		{
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
	user_functions.reset();
}
