#include "asar.h"
#include "assembleblock.h"
#include "math_ast.h"
#include "platform/file-helpers.h"
#include "macro.h"

namespace {
struct cachedfile {
	string filename;
	virtual_file_handle filehandle;
	size_t filesize;
	bool used;
};

#define numcachedfiles 16

cachedfile cachedfiles[numcachedfiles];
int cachedfileindex = 0;

// Opens a file, trying to open it from cache first

cachedfile * opencachedfile(string fname, bool should_error)
{
	cachedfile * cachedfilehandle = nullptr;

	const char* current_file = get_current_file_name();

	// RPG Hacker: Only using a combined path here because that should
	// hopefully result in a unique string for every file, whereas
	// fname could be a relative path, which isn't guaranteed to be unique.
	// Note that this does not affect how we open the file - this is
	// handled by the filesystem and uses our include paths etc.
	string combinedname = filesystem->create_absolute_path(dir(current_file), fname);

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
			cachedfilehandle->filehandle = filesystem->open_file(fname, current_file);
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
// closecachedfiles is declared at the end of the file outside namespace{}



void assert_argc(const std::vector<math_val>& args, int expected_args) {
	if(args.size() != expected_args) {
		asar_throw_error(2, error_type_block, error_id_argument_count, expected_args, (int)args.size());
	}
}

// these templates could be implemented in a fully generic way too (without
// specializing per argument count), but that looks a little too cryptic to me.
template<math_val (*F)()>
math_val fixed_arity(const std::vector<math_val>& args) {
	assert_argc(args, 0);
    return F();
}
template<math_val (*F)(math_val)>
math_val fixed_arity(const std::vector<math_val>& args) {
	assert_argc(args, 1);
    return F(args[0]);
}
template<math_val (*F)(math_val,math_val)>
math_val fixed_arity(const std::vector<math_val>& args) {
	assert_argc(args, 2);
    return F(args[0], args[1]);
}
template<math_val (*F)(math_val,math_val,math_val)>
math_val fixed_arity(const std::vector<math_val>& args) {
	assert_argc(args, 3);
    return F(args[0], args[1], args[2]);
}

template<double (*F)(double)>
math_val fn_unary_real(const std::vector<math_val>& args) {
	assert_argc(args, 1);
	double val = F(args[0].get_double());
	if (val != val) asar_throw_error(2, error_type_block, error_id_nan);
	return val;
};

template<double (*F)(double)>
math_val fn_rounding(const std::vector<math_val>& args) {
	assert_argc(args, 1);
	double val = F(args[0].get_double());
	// TODO where should we check this? math_val(double) constructor maybe?
	if (val != val) asar_throw_error(2, error_type_block, error_id_nan);
	return float_to_int(val);
};

template<math_binop_type OP>
math_val math_binop_function(const std::vector<math_val>& args) {
	assert_argc(args, 2);
	return evaluate_binop(args[0], args[1], OP);
}

template<bool (*F)(bool, bool)>
math_val math_bool_binop_function(const std::vector<math_val>& args) {
	assert_argc(args, 2);
	return (int64_t)F(args[0].get_bool(), args[1].get_bool());
}

bool fn_and(bool a, bool b)  { return a && b; }
bool fn_or(bool a, bool b)   { return a || b; }
bool fn_nand(bool a, bool b) { return !(a && b); }
bool fn_nor(bool a, bool b)  { return !(a || b); }
bool fn_xor(bool a, bool b)  { return a ^ b; }

math_val fn_select(math_val cond, math_val a, math_val b) {
	return cond.get_bool() ? a : b;
}

template<int (*F)(int)>
math_val fn_snes_pc(math_val arg) {
	return (int64_t)F(arg.get_integer());
}

template<int& variable>
math_val fn_pc_realbase() {
	return (int64_t)variable;
}
int haslabel_always() {
	return 3;
}
template<int& variable>
int getlen_pc_realbase(const std::vector<owned_node>& args, bool could_be_bank_ex) {
	// todo : should this use freespaceid even with base active???
	return getlenforlabel(variable, freespaceid, true);
}

math_val fn_bank(math_val arg) {
	return (arg.get_integer() >> 16);
}
int getlen_bank(const std::vector<owned_node>& args, bool could_be_bank_ex) {
	return 1;
}

math_val fn_safediv(math_val a, math_val b, math_val default_) {
	if(b.get_double() == 0.0) return default_;
	return evaluate_binop(a, b, math_binop_type::div);
}

math_val fn_not(math_val arg) {
	return (int64_t)!arg.get_bool();
}

template<int (*F)(const char*, const char*)>
math_val fn_str_eq(math_val va, math_val vb) {
	const string& a = va.get_str();
	const string& b = vb.get_str();
	bool result = F(a.data(), b.data()) == 0;
	return (int64_t)result;
}

math_val fn_char(math_val str_, math_val ind_) {
	const string& s = str_.get_str();
	int64_t ind = ind_.get_integer();
	if(ind < 0 || ind >= s.length())
		asar_throw_error(2, error_type_block, error_id_oob, (int)ind, s.length());
	return (int64_t)(unsigned char)s[ind];
}
math_val fn_strlen(math_val val) {
	return (int64_t)val.get_str().length();
}

template<int count>
math_val fn_read(const std::vector<math_val>& args) {
	if(args.size() < 1 || args.size() > 2) {
		// TODO expected amount should be string to show the range
		asar_throw_error(2, error_type_block, error_id_argument_count, 2, (int)args.size());
	}
	int64_t target = args[0].get_integer();
	int addr = snestopc(target);

	if(args.size() == 2) {
		math_val default_val = args[1];
		if(addr < 0) return default_val;
		if(addr + count > romlen_r) return default_val;
	} else {
		if (addr < 0)
			asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, (hex((unsigned int)target, 6) + " in read function").data());
		else if (addr + count > romlen_r)
			asar_throw_error(2, error_type_block, error_id_snes_address_out_of_bounds, (hex(target, 6) + " in read function").data());
	}

	int64_t value = 0;
	for(int i = 0; i < count; i++)
	{
		value |= romdata_r[addr+i] << (8 * i);
	}
	return value;
}
template<int count>
math_val fn_canread(const std::vector<math_val>& args) {
	int64_t length = count;
	int64_t addr;
	if(count == 0) {
		assert_argc(args, 2);
		length = args[0].get_integer();
		addr = args[1].get_integer();
	} else {
		assert_argc(args, 1);
		addr = args[0].get_integer();
	}
	int addr_pc = snestopc(addr);
	if (addr_pc<0 || addr_pc+length-1>=romlen_r) return (int64_t)0;
	else return (int64_t)1;
}

template<int count>
math_val fn_readfile(const std::vector<math_val>& args) {
	if(args.size() < 2 || args.size() > 3) {
		// TODO expected amount should be string to show the range
		asar_throw_error(2, error_type_block, error_id_argument_count, 3, (int)args.size());
	}
	string fname = args[0].get_str();
	int64_t offset = args[1].get_integer();
	bool should_error = args.size() == 2;
	cachedfile * fhandle = opencachedfile(fname, should_error);

	if(!should_error) {
		math_val default_val = args[2];
		if(fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return default_val;
		if(offset < 0) return default_val;
		if(offset + count > fhandle->filesize) return default_val;
	} else {
		if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) 
			asar_throw_error(2, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), fname.data());
		if (offset < 0 || offset + count > fhandle->filesize)
			asar_throw_error(2, error_type_block, error_id_file_offset_out_of_bounds, dec(offset).data(), fname.data());
	}

	unsigned char data[4] = { 0, 0, 0, 0 };
	filesystem->read_file(fhandle->filehandle, data, offset, count);

	int64_t value = 0;
	for(size_t i = 0; i < count; i++)
	{
		value |= data[i] << (8 * i);
	}

	return value;
}

template<int count>
math_val fn_canreadfile(const std::vector<math_val>& args) {
	string fname;
	int64_t length = count;
	int64_t offset;
	if(count == 0) {
		assert_argc(args, 3);
		fname = args[0].get_str();
		offset = args[1].get_integer();
		length = args[2].get_integer();
	} else {
		assert_argc(args, 2);
		fname = args[0].get_str();
		offset = args[1].get_integer();
	}
	
	cachedfile * fhandle = opencachedfile(fname, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) return (int64_t)0;
	if (offset < 0 || offset + length > fhandle->filesize) return (int64_t)0;
	return (int64_t)1;
}

math_val fn_min(math_val a, math_val b) {
	math_val cond = evaluate_binop(a, b, math_binop_type::comp_lt);
	return cond.get_bool() ? a : b;
}
math_val fn_max(math_val a, math_val b) {
	math_val cond = evaluate_binop(a, b, math_binop_type::comp_gt);
	return cond.get_bool() ? a : b;
}
math_val fn_clamp(math_val val, math_val lo, math_val hi) {
	// compute temp = min(hi, val)
	math_val temp = fn_min(hi, val);
	return fn_max(lo, temp);
}

math_val fn_round(math_val val, math_val precision) {
	// i used to hate the float->str->float approach, but the
	// alternatives do end up quite messy if implemented properly
	string as_str = ftostrvar(val.get_double(), precision.get_integer());
	double res = std::atof(as_str);
	return res;
}
math_val fn_isdefined(math_val defname) {
	return (int64_t)defines.exists(defname.get_str());
}

math_val fn_filesize(math_val fname) {
	string name = fname.get_str();
	cachedfile * fhandle = opencachedfile(name, false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) 
		asar_throw_error(2, error_type_block, vfile_error_to_error_id(asar_get_last_io_error()), name.data());
	return (int64_t)fhandle->filesize;
}

math_val fn_filestatus(math_val fname) {
	cachedfile * fhandle = opencachedfile(fname.get_str(), false);
	if (fhandle == nullptr || fhandle->filehandle == INVALID_VIRTUAL_FILE_HANDLE) {
		if (filesystem->get_last_error() == vfe_doesnt_exist)
			return (int64_t)1;
		else return (int64_t)2;
	}
	return (int64_t)0;
}

math_val fn_sizeof(math_val val) {
	string symbol = val.get_identifier();
	// TODO: do we have a better spot where to parse this...?
	if(symbol == "..."){
		if(!inmacro) asar_throw_error(2, error_type_block, error_id_vararg_sizeof_nomacro);
		if(numvarargs == -1) asar_throw_error(2, error_type_block, error_id_macro_not_varadic, "sizeof(...)");
		return (int64_t)numvarargs;
	}
	if(pass && !structs.exists(symbol)) asar_throw_error(2, error_type_block, error_id_struct_not_found, symbol.data());
	else if(!structs.exists(symbol)) return (int64_t)0;
	return (int64_t)structs.find(symbol).struct_size;
}

math_val fn_objectsize(math_val val) {
	string symbol = val.get_identifier();
	if(pass && !structs.exists(symbol)) asar_throw_error(2, error_type_block, error_id_struct_not_found, symbol.data());
	else if(!structs.exists(symbol)) return (int64_t)0;
	return (int64_t)structs.find(symbol).object_size;
}

math_val fn_datasize(math_val val) {
	string name = val.get_identifier();
	int label;
	if(!labels.exists(name)) asar_throw_error(2, error_type_block, error_id_label_not_found, name.data());
	snes_label label_data = labels.find(name);

	label = label_data.id;
	snes_label selected_label;
	selected_label.id = 0xFFFFFF;
	selected_label.pos = 0xFFFFFF;
	labels.each([&selected_label, label](const char *key, snes_label current_label){
		if(label < current_label.id && current_label.id < selected_label.id){
			selected_label = current_label;
		}
	});
	if(selected_label.id == 0xFFFFFF) asar_throw_warning(2, warning_id_datasize_last_label, name.data());
	if(selected_label.pos-label_data.pos > 0xFFFF) asar_throw_warning(2, warning_id_datasize_exceeds_size, name.data());
	return (int64_t)(selected_label.pos-label_data.pos);
}

} // namespace

void closecachedfiles()
{
	for (int i = 0; i < numcachedfiles; i++)
	{
		if (cachedfiles[i].used)
		{
			if (cachedfiles[i].filehandle != INVALID_VIRTUAL_FILE_HANDLE && filesystem)
			{
				filesystem->close_file(cachedfiles[i].filehandle);
				cachedfiles[i].filehandle = INVALID_VIRTUAL_FILE_HANDLE;
			}

			cachedfiles[i].used = false;
		}
	}

	cachedfileindex = 0;
}

const std::unordered_map<string, math_builtin_function> builtin_functions = {
	{ "sqrt", fn_unary_real<sqrt> },
	{ "sin", fn_unary_real<sin> },
	{ "cos", fn_unary_real<cos> },
	{ "tan", fn_unary_real<tan> },
	{ "asin", fn_unary_real<asin> },
	{ "acos", fn_unary_real<acos> },
	{ "atan", fn_unary_real<atan> },
	{ "arcsin", fn_unary_real<asin> },
	{ "arccos", fn_unary_real<acos> },
	{ "arctan", fn_unary_real<atan> },
	{ "log", fn_unary_real<log> },
	{ "log10", fn_unary_real<log10> },
	{ "log2", fn_unary_real<log2> },

	{ "ceil", fn_rounding<ceil> },
	{ "floor", fn_rounding<floor> },

	{ "read1", fn_read<1> },    //This handles the safe and unsafe variant
	{ "read2", fn_read<2> },
	{ "read3", fn_read<3> },
	{ "read4", fn_read<4> },
	{ "canread",  fn_canread<0> },
	{ "canread1", fn_canread<1> },
	{ "canread2", fn_canread<2> },
	{ "canread3", fn_canread<3> },
	{ "canread4", fn_canread<4> },

	{ "readfile1", fn_readfile<1> },
	{ "readfile2", fn_readfile<2> },
	{ "readfile3", fn_readfile<3> },
	{ "readfile4", fn_readfile<4> },
	{ "canreadfile", fn_canreadfile<0> },
	{ "canreadfile1", fn_canreadfile<1> },
	{ "canreadfile2", fn_canreadfile<2> },
	{ "canreadfile3", fn_canreadfile<3> },
	{ "canreadfile4", fn_canreadfile<4> },

	{ "filesize", fixed_arity<fn_filesize> },
	{ "getfilestatus", fixed_arity<fn_filestatus> },

	{ "defined", fixed_arity<fn_isdefined> },

	{ "snestopc", fixed_arity<fn_snes_pc<snestopc>> },
	{ "pctosnes", fixed_arity<fn_snes_pc<pctosnes>> },
	{ "realbase", { fixed_arity<fn_pc_realbase<realsnespos>>, haslabel_always, getlen_pc_realbase<realsnespos> } },
	{ "pc", { fixed_arity<fn_pc_realbase<snespos>>, haslabel_always, getlen_pc_realbase<snespos> } },

	{ "max", fixed_arity<fn_max> },
	{ "min", fixed_arity<fn_min> },
	{ "clamp", fixed_arity<fn_clamp> },

	{ "safediv", fixed_arity<fn_safediv> },

	{ "select", fixed_arity<fn_select> },
	{ "bank", { fixed_arity<fn_bank>, getlen_bank } },
	{ "not", fixed_arity<fn_not> },
	{ "equal", math_binop_function<math_binop_type::comp_eq> },
	{ "notequal", math_binop_function<math_binop_type::comp_ne> },
	{ "less", math_binop_function<math_binop_type::comp_lt> },
	{ "lessequal", math_binop_function<math_binop_type::comp_le> },
	{ "greater", math_binop_function<math_binop_type::comp_gt> },
	{ "greaterequal", math_binop_function<math_binop_type::comp_ge> },

	{ "and", math_bool_binop_function<fn_and> },
	{ "or", math_bool_binop_function<fn_or> },
	{ "nand", math_bool_binop_function<fn_nand> },
	{ "nor", math_bool_binop_function<fn_nor> },
	{ "xor", math_bool_binop_function<fn_xor> },

	{ "round", fixed_arity<fn_round> },

	{ "sizeof", fixed_arity<fn_sizeof> },
	{ "objectsize", fixed_arity<fn_objectsize> },
	{ "datasize", { fixed_arity<fn_datasize>, haslabel_always } },

	{ "stringsequal", fixed_arity<fn_str_eq<strcmp>> },
	{ "stringsequalnocase", fixed_arity<fn_str_eq<stricmp>> },
	{ "char", fixed_arity<fn_char> },
	{ "stringlength", fixed_arity<fn_strlen> },
};
