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
#include "macro.h"
#include "asar_math.h"
#include "warnings.h"
#include <math.h>
#include <functional>
#include <algorithm>

bool math_pri=true;
bool math_round=false;
extern bool suppress_all_warnings;

static const char * str;
//save before calling eval if needed after
static const char * current_user_function_name;

static double getnumcore();
static double getnum();
static double eval(int depth);

//label (bool foundLabel) (bool forwardLabel)
//userfunction

bool foundlabel;
bool foundlabel_static;
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
	string combinedname = filesystem->create_absolute_path(dir(thisfilename), fname);

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
		asar_throw_error(2, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), fname.data());
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
       if(pass && !structs.exists(name)) asar_throw_error(2, error_type_block, error_id_struct_not_found, name);
       else if(!structs.exists(name)) return 0;
       return structs.find(name).struct_size;
}

static int object_size(const char *name)
{
       if(pass && !structs.exists(name)) asar_throw_error(2, error_type_block, error_id_struct_not_found, name);
       else if(!structs.exists(name)) return 0;
       return structs.find(name).object_size;
}

static int data_size(const char *name)
{
	unsigned int label;
	unsigned int next_label = 0xFFFFFF;
	if(!labels.exists(name)) asar_throw_error(2, error_type_block, error_id_label_not_found, name);
	foundlabel = true;
	snes_label label_data = labels.find(name);
	foundlabel_static &= label_data.is_static;
	label = label_data.pos & 0xFFFFFF;
	labels.each([&next_label, label](const char *key, snes_label current_label){
		current_label.pos &= 0xFFFFFF;
		if(label < current_label.pos && current_label.pos < next_label){
			next_label = current_label.pos;
		}
	});
	if(next_label == 0xFFFFFF) asar_throw_warning(2, warning_id_datasize_last_label, name);
	if(next_label-label > 0xFFFF) asar_throw_warning(2, warning_id_datasize_exceeds_size, name);
	return next_label-label;
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
			return string(safedequote(tempname.temp_raw()));
		}
		// RPG Hacker: AFAIK, this is never actually triggered, since unmatched quotes are already detected earlier,
		// but since it does no harm here, I'll keep it in, just to be safe
		else asar_throw_error(2, error_type_block, error_id_string_literal_not_terminated);
	}//make this error a better one later
	
	asar_throw_error(2, error_type_block, error_id_string_literal_not_terminated);
	return ""; //never actually called, but I don't feel like figuring out __attribute__ ((noreturn)) on MSVC
}

//only returns alphanumeric (and _) starting with alpha or _
string get_symbol_argument()
{
	while (*str==' ') str++;	//is this proper?  Dunno yet.
	const char * strpos = str;
	// hack: for backwards compat, allow strings as symbols
	if(*str=='"') {
		asar_throw_warning(2, warning_id_feature_deprecated, "quoted symbolic arguments", "Remove the quotations");
		string arg = get_string_argument();
		int i = 0;
		if(is_alpha(arg[i]) || arg[i] == '_') i++;
		while(is_alnum(arg[i]) || arg[i] == '_' || arg[i] == '.') i++;
		if(arg[i] != '\0') asar_throw_error(2, error_type_block, error_id_invalid_label_name);
                return arg;
	}
	if(is_alpha(*str) || *str == '_') str++;
	while (is_alnum(*str) || *str == '_' || *str == '.') str++;
	if(strpos == str){
		//error nothing was read, this is a placeholder error
		asar_throw_error(2, error_type_block, error_id_string_literal_not_terminated);
	}
	
	string symbol = string(strpos, (int)(str - strpos));
	while (*str==' ') str++;	//eat spaces
	return symbol;
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
	asar_throw_error(2, error_type_block, error_id_require_parameter);
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

double asar_bank(double a)
{
	return (int)a >> 16;
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

double asar_realbase_wrapper()
{
	//need a better way to do this to make sure it is useful...
	//foundlabel=true;  //Going to consider this an implicit label because we don't really have a better system
	return realsnespos;
}

double asar_pc_wrapper()
{
	// while this really should set foundlabel, not doing it right now for symmetry with realbase.
	// we should rework the way foundlabel works anyways.
	//foundlabel = true;
	return snespos;
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
		if (addr<0) asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex6((unsigned int)target) + " in read function").data());
		else if (addr+count>romlen_r) asar_throw_error(2, error_type_block, error_id_snes_address_out_of_bounds, (hex6(target) + " in read function").data());
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
		if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(2, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
		if (offset < 0 || offset + count > fhandle->filesize) asar_throw_error(2, error_type_block, error_id_file_offset_out_of_bounds, dec(offset).data(), name.data());
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
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) asar_throw_error(2, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
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
	string symbol = get_symbol_argument();
	if(symbol == "..."){
		if(!inmacro) asar_throw_error(2, error_type_block, error_id_vararg_sizeof_nomacro);
		if(numvarargs == -1) asar_throw_error(2, error_type_block, error_id_macro_not_varadic);
		return numvarargs;
	}
	return (double)struct_size(symbol);
}

static double asar_objectsize_wrapper()
{
	return (double)object_size(get_symbol_argument());
}

static double asar_datasize_wrapper()
{
	return (double)data_size(get_symbol_argument());
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
	bool is_symbolic = true;
	int parlevel=0;
	int i = 0;
	while(parlevel > 0 || (str[i] != ',' && str[i] != ')'))
	{
		is_symbolic &= is_ualnum(str[i]);
		if(str[i] == '(') parlevel++;
		else if(str[i] == ')') parlevel--;
		i++;
	}
	result += string(str, i);
	str += i;
	
	if(!is_symbolic)
	{
		const char *oldstr=str;
		str = (const char *)result;
		result = ftostr(eval(0));
		str = oldstr;
	}
	return result;
}

assocarr<double (*)()> builtin_functions =
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

	{"ceil", asar_unary_wrapper<ceil>},
	{"floor", asar_unary_wrapper<floor>},

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
	{"realbase", asar_realbase_wrapper},
	{"pc", asar_pc_wrapper},

	{"max", asar_binary_wrapper<asar_max>},
	{"min", asar_binary_wrapper<asar_min>},
	{"clamp", asar_clamp},
	
	{"safediv", asar_safediv},
	
	{"select", asar_select},
	{"bank", asar_unary_wrapper<asar_bank>},
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
	{"datasize", asar_datasize_wrapper},
	
	{"stringsequal", asar_stringsequal},
	{"stringsequalnocase", asar_stringsequalnocase}
};

assocarr<double (*)()> functions;

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
			asar_throw_error(2, error_type_block, error_id_expected_parameter, current_user_function_name);
		}
		args[i] = copy_arg();
		has_next = has_next_parameter();
	}
	
	if(has_next)
	{
		asar_throw_error(2, error_type_block, error_id_unexpected_parameter, current_user_function_name);
	}
	
	for(int i=0; user_function.content[i]; i++)
	{
		if(!is_alpha(user_function.content[i]) && user_function.content[i] != '_')
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
			if(potential_arg && (!is_alnum(user_function.content[next_char]) && user_function.content[next_char] != '_'))
			{
				real_content += args[j];
				i = next_char - 1;
				found = true;
			}
		}
		
		if(!found){
			for(; is_ualnum(user_function.content[i]); i++){
				real_content += user_function.content[i];
			}
			real_content += user_function.content[i];
		}
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
		//asar_throw_error(0, error_type_block, error_id_function_redefined, name);
		asar_throw_warning(1, warning_id_feature_deprecated, "overwriting a previously defined function", "change the function name");
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
		if (*str != ')') asar_throw_error(2, error_type_block, error_id_mismatched_parentheses);
		str++;
		return rval;
	}
	if (*str=='$')
	{
		if (!is_xdigit(str[1])) asar_throw_error(2, error_type_block, error_id_invalid_hex_value);
		if (to_lower(str[2])=='x') return -42;//let str get an invalid value so it'll throw an invalid operator later on
		return strtoull(str+1, const_cast<char**>(&str), 16);
	}
	if (*str=='%')
	{
		if (str[1] != '0' && str[1] != '1') asar_throw_error(2, error_type_block, error_id_invalid_binary_value);
		return strtoull(str+1, const_cast<char**>(&str), 2);
	}
	if (*str=='\'')
	{
		if (!str[1] || str[2] != '\'') asar_throw_error(2, error_type_block, error_id_invalid_character);
		unsigned int rval=table.table[(unsigned char)str[1]];
		str+=3;
		return rval;
	}
	if (is_digit(*str))
	{
		const char* end = str;
		while (is_digit(*end) || *end == '.') end++;
		string number;
		number.assign(str, (int)(end - str));
		str = end;
		return atof(number);
	}
	if (is_alpha(*str) || *str=='_' || *str=='.' || *str=='?')
	{
		const char * start=str;
		while (is_alnum(*str) || *str == '_' || *str == '.') str++;
		int len=(int)(str-start);
		while (*str==' ') str++;
		if (*str=='(')
		{
			str++;
			// RPG Hacker: This is only here to assure that all strings are still
			// alive in memory when we call our functions further down
			double result;
			while (true)
			{
				while (*str==' ') str++;
				string function_name = string(start, len);
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
				}
				asar_throw_error(2, error_type_block, error_id_malformed_function_call);
			}

			asar_throw_error(2, error_type_block, error_id_function_not_found, start);
		}
		else
		{
			foundlabel=true;

			const char *old_start = start;
			snes_label label_data = labelval(&start);
			int i=(int)label_data.pos;
			foundlabel_static &= label_data.is_static;
			bool scope_passed = false;
			bool subscript_passed = false;
			while (str < start)
			{
				if (*str == '.') scope_passed = true;
				if (*(str++) == '[')
				{
					if (subscript_passed)
					{
						asar_throw_error(2, error_type_block, error_id_multiple_subscript_operators);
						break;
					}
					subscript_passed = true;
					if (scope_passed)
					{
						asar_throw_error(2, error_type_block, error_id_invalid_subscript);
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
	asar_throw_error(2, error_type_block, error_id_invalid_number);
	return 0.0;
}

static double sanitize(double val)
{
	if (val != val) asar_throw_error(2, error_type_block, error_id_nan);
	if (math_round && !default_math_round_off) return trunc(val); // originally used int cast, but that broke numbers > $8000_0000
	return val;
}

static double getnum()
{
	while (*str==' ') str++;
#define prefix(sym, func) if (*str == sym) { str+=1; double val=getnum(); return sanitize(func); }
#define prefix_dep(sym, func) if (*str == sym) { str+=1; asar_throw_warning(2, warning_id_feature_deprecated, "xkas style numbers ", "remove the #"); double val=getnum(); return sanitize(func); }
#define prefix2(sym, sym2, func) if (*str == sym && *(str+1) == sym2) { str+=2; double val=getnum(); return sanitize(func); }
	prefix('-', -val);
	prefix('~', ~(int)val);
	prefix2('<', ':', (int)val>>16);
	prefix('+', val);
	prefix_dep('#' && emulatexkas, val);
#undef prefix
	return sanitize(getnumcore());
}

int64_t getnum(const char* instr)
{
	return getnum64(instr);
	
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
	asar_throw_error(2, error_type_block, errid);
	return 0.0;
}

static double eval(int depth)
{
	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);

	if (posnegname.length() > 0)
	{
		if (*posneglabel != '\0' && *posneglabel != ')') goto notposneglabel;

		str = posneglabel;

		foundlabel=true;
		if (*(posneglabel-1) == '+') forwardlabel=true;
		snes_label label_data = labelval(posnegname);
		foundlabel_static &= label_data.is_static;
		return label_data.pos & 0xFFFFFF;
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
		if (math_round && !default_math_round_off) left=trunc(left);
#define oper(name, thisdepth, contents)      \
			if (!strncmp(str, name, strlen(name))) \
			{                                      \
				if (math_pri || default_math_pri)                        \
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
		oper("<<", 1, right >= 0.0 ? (int64_t)left<<(uint64_t)right : oper_wrapped_throw(error_id_negative_shift));
		oper(">>", 1, right >= 0.0 ? (int64_t)left>>(uint64_t)right : oper_wrapped_throw(error_id_negative_shift));
		oper("&", 0, (int64_t)left&(int64_t)right);
		oper("|", 0, (int64_t)left|(int64_t)right);
		oper("^", 0, (int64_t)left^(int64_t)right);
		asar_throw_error(2, error_type_block, error_id_unknown_operator);
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
	foundlabel_static=true;
	forwardlabel=false;
	double rval;
	
	if(math_pri || default_math_pri)
	{
		str = s;
		rval = eval(0);
	}
	else
	{
		str = s;
		rval = eval(0);

		double pri_rval = NAN;

		suppress_all_warnings = true;
		math_pri = true;
		try
		{
			str = s;
			pri_rval = eval(0);
		}
		catch (errfatal&) {}
		suppress_all_warnings = false;
		math_pri = false;

		if (pri_rval != rval)
		{
			asar_throw_warning(2, warning_id_feature_deprecated, "xkas style left to right math ", "apply order of operations");
		}
	}
	if (*str)
	{
		if (*str == ',') asar_throw_error(2, error_type_block, error_id_invalid_input);
		else asar_throw_error(2, error_type_block, error_id_mismatched_parentheses);
	}
	return rval;
}

void initmathcore()
{
	functions.reset();
	builtin_functions.each([](const char* key, double (*val)()) {
		functions[key] = val;
		functions[STR "_" + key] = val;
	});
	user_functions.reset();
}

void deinitmathcore()
{
	//not needed
}
