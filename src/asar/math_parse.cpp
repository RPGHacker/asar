#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "table.h"
#include "unicode.h"
#include <cmath>

#include "math_ast.h"


bool foundlabel;
// WARNING: this flag is only correctly set in pass 0, as forward labels are
// always non-static but we don't know when we hit forward-labels past pass 0
bool foundlabel_static;
// only set in pass 0
bool forwardlabel;


std::unordered_map<string, math_user_function> user_functions;

// we don't need this struct to be exported
namespace {
// data necessary for parsing an expression, which might be an user function declaration
struct parse_context {
	const char*& str;
	// this map is empty unless declaring a function,
	// in which case it maps argument name to argument index
	const std::unordered_map<string, size_t> function_arg_names;
	// these are methods to allow easier access to `str`
	owned_node parse_atom();
	owned_node parse_unops();
	owned_node parse_binops(int depth = 0);
	owned_node parse();
};
}


static const long hextable[] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

// "atom" = literal, parenthesized expression, or label reference
owned_node parse_context::parse_atom() {
	if(*str == '$') {
		if (!is_xdigit(*++str)) asar_throw_error(2, error_type_block, error_id_invalid_hex_value);
		const char* start = str;
		int64_t ret = 0; // todo error on overflow?
		while (hextable[0 + *str] >= 0) {
			ret = (ret << 4) | hextable[0 + *str++];
		}
		int len = str - start;
		int len_bytes = (len+1)/2;
		return std::make_unique<math_ast_literal>(ret, len_bytes);
	}
	if (is_ualpha(*str) || *str=='.' || *str=='?') {
		const char * start=str;
		while (is_ualnum(*str) || *str == '.') str++;
		int len=(int)(str-start);
		while (*str==' ') str++;
		if (*str=='(') {
			str++;
			string func_name;
			func_name.assign(start, len);
			std::vector<owned_node> arguments;
			while(*str != ')') {
				while (*str==' ') str++;
				arguments.emplace_back(parse_binops());
				// is "invalid number" good here?
				if(*str != ',' && *str != ')') asar_throw_error(2, error_type_block, error_id_invalid_number);
				if(*str == ',') str++;
			}
			str++;
			return std::make_unique<math_ast_function_call>(std::move(arguments), std::move(func_name));
		} else {
			string name_part(start, len);
			if(name_part == "...") {
				// a tiny bit ugly, but whatever
				return std::make_unique<math_ast_literal>(math_val::make_identifier(name_part));
			}
			if(!function_arg_names.empty()) {
				auto it = function_arg_names.find(name_part);
				if(it != function_arg_names.end()) {
					return std::make_unique<math_ast_function_argument>(it->second);
				}
			}
			string name = labelname(&start);
			str = start;
			if(*str == '[') {
				// struct array indexing
				str++;
				auto index = parse_binops();
				if(*str != ']') asar_throw_error(2, error_type_block, error_id_invalid_label_missing_closer);
				str++;
				string subname = name;
				if(*str == '.') {
					// this part used to be in labelname... not sure where it really belongs....
					while (is_ualnum(*str) || *str == '.') {
						subname += *(str++);
					}
				}
				// when doing base[index].sub:
				// result = (base_addr + index*object_size(base)) + sub_offset
				// = sub_addr + index*object_size(base)
				// so we build a math node that represents this calculation
				auto node_sub = std::make_unique<math_ast_label>(subname);
				auto node_base = std::make_unique<math_ast_label>(name);
				std::vector<owned_node> arg_list;
				arg_list.emplace_back(std::move(node_base));
				auto node_objsize = std::make_unique<math_ast_function_call>(std::move(arg_list), "objectsize");
				auto node_mul = std::make_unique<math_ast_binop>(std::move(node_objsize), std::move(index), math_binop_type::mul);
				auto node_add = std::make_unique<math_ast_binop>(std::move(node_mul), std::move(node_sub), math_binop_type::add);
				return node_add;
			} else {
				return std::make_unique<math_ast_label>(name);
			}
		}
	}
	if(*str == '(') {
		str++;
		auto res = parse_binops();
		if(*str != ')') asar_throw_error(2, error_type_block, error_id_mismatched_parentheses);
		str++;
		return res;
	}
	if(*str == '%') {
		if (str[1] != '0' && str[1] != '1') asar_throw_error(2, error_type_block, error_id_invalid_binary_value);
		const char* start = str+1;
		uint64_t res = strtoull(str+1, const_cast<char**>(&str), 2);
		int len = str - start;
		return std::make_unique<math_ast_literal>((int64_t)res, (len+7)/8);
	}
	if (*str=='\'') {
		if (!str[1]) asar_throw_error(2, error_type_block, error_id_invalid_character);
		int orig_val;
		str++;
		str += utf8_val(&orig_val, str);
		if (orig_val == -1) asar_throw_error(0, error_type_block, error_id_invalid_utf8);
		if (*str != '\'') asar_throw_error(2, error_type_block, error_id_invalid_character);
		int64_t rval=thetable.get_val(orig_val);
		if (rval == -1)
		{
			// RPG Hacker: Should be fine to not check return value of codepoint_to_utf8() here, because
			// our error cases above already made sure that orig_val contains valid data at this point.
			string u8_str;
			codepoint_to_utf8(&u8_str, orig_val);
			asar_throw_error(2, error_type_block, error_id_undefined_char, u8_str.data());
		}
		str++;
		return std::make_unique<math_ast_literal>(rval, 1);
	}
	if (is_digit(*str)) {
		const char* end = str;
		bool is_float = false;
		while (is_digit(*end) || *end == '.') {
			if(*end == '.') is_float = true;
			end++;
		}
		string number;
		number.assign(str, (int)(end - str));
		str = end;
		if(is_float) {
			double res = std::atof(number);
			return std::make_unique<math_ast_literal>(res);
		} else {
			int64_t res = strtoll(number, nullptr, 10);
			int len = (res >= 0x10000) ? 3 : (res >= 0x100) ? 2 : 1; 
			return std::make_unique<math_ast_literal>(res, len);
		}
	}
	if(*str == '"') {
		const char * strpos = str + 1;
		while (*str!='"' && *str!='\0') str++;
		str = strchr(str + 1, '"'); // TODO don't we have string escapes????
		string tempname(strpos , (int)(str - strpos));
		str++;
		while (*str==' ') str++;	//eat space
		return std::make_unique<math_ast_literal>(tempname);
	}
	asar_throw_error(2, error_type_block, error_id_invalid_number);
}

owned_node parse_context::parse_unops() {
	while(*str == ' ') str++;
	// optimize for the common case
	// TODO how much of an optimization is this really?
	if(*str == '$') return parse_atom();
	
	if(*str == '-') {
		str++;
		return std::make_unique<math_ast_unop>(parse_unops(), math_unop_type::neg);
	} else if(*str == '~') {
		str++;
		return std::make_unique<math_ast_unop>(parse_unops(), math_unop_type::bit_not);
	} else if(*str == '<' && str[1] == ':') {
		str += 2;
		return std::make_unique<math_ast_unop>(parse_unops(), math_unop_type::bank_extract);
	} else if(*str == '+') {
		str++;
		return parse_unops();
	}
	else return parse_atom();
}

owned_node parse_context::parse_binops(int depth) {
	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);
	if (posnegname.length() > 0 &&
		(*posneglabel == '\0' || *posneglabel == ')')) {
		str = posneglabel;
		return std::make_unique<math_ast_label>(posnegname);
	}

	recurseblock rec;

	owned_node left = parse_unops();
	owned_node right;
	while(*str == ' ') str++;
	while (*str && *str != ')' && *str != ','&& *str != ']') {
		while(*str == ' ') str++;
		// TODO can we make this macro a bit nicer???
#define oper(name, thisdepth, contents)      \
			if (!strncmp(str, name, strlen(name)))       \
			{                                            \
				if (depth<=thisdepth)                \
				{                                    \
					str += strlen(name);           \
					right = parse_binops(thisdepth+1);     \
					left = std::make_unique<math_ast_binop>(std::move(left), std::move(right), contents); \
					continue; \
				}                                    \
				else return left;                    \
			}
		oper("**", 6, math_binop_type::pow);
		oper("*", 5, math_binop_type::mul);
		oper("/", 5, math_binop_type::div);
		oper("%", 5, math_binop_type::mod);
		oper("+", 4, math_binop_type::add);
		oper("-", 4, math_binop_type::sub);
		oper("<<", 3, math_binop_type::shift_left);
		oper(">>", 3, math_binop_type::shift_right);

		//these two needed checked early to avoid bitwise from eating a operator
		oper("&&", 0, math_binop_type::logical_and);
		oper("||", 0, math_binop_type::logical_or);
		oper("&", 2, math_binop_type::bit_and);
		oper("|", 2,math_binop_type::bit_or);
		oper("^", 2, math_binop_type::bit_xor);

		oper(">=", 1, math_binop_type::comp_ge);
		oper("<=", 1, math_binop_type::comp_le);
		oper(">", 1, math_binop_type::comp_gt);
		oper("<", 1, math_binop_type::comp_lt);
		oper("==", 1, math_binop_type::comp_eq);
		oper("!=", 1, math_binop_type::comp_ne);
		asar_throw_error(2, error_type_block, error_id_unknown_operator);
#undef oper
	}
	return left;
}

owned_node parse_context::parse() {
	auto res = parse_binops();
	if(*str) {
		if(*str == ',') asar_throw_error(2, error_type_block, error_id_invalid_input);
		else asar_throw_error(2, error_type_block, error_id_mismatched_parentheses);
	}
	return res;
}

void createuserfunc(const char * name, const char * arguments, const char * content) {
	if(user_functions.count(name) != 0 || builtin_functions.count(name) != 0) {
		asar_throw_error(0, error_type_block, error_id_function_redefined, name);
	}
	string arguments_buf = arguments;
	// TODO: if we want to be more lenient with spaces in the `function`
	// command, then we need to handle spaces inside `arguments_buf`
	int numargs;
	autoptr<char**> spl = split(arguments_buf.raw(), ',', &numargs);
	size_t arg_count = numargs;
	if(numargs == 1 && spl[0] == string{""}) {
		arg_count = 0;
	}
	std::unordered_map<string, size_t> arg_indices;
	for(size_t i = 0; i < arg_count; i++) {
		string argname = spl[i];
		if(arg_indices.count(argname)) {
			asar_throw_error(0, error_type_block, error_id_duplicate_param_name, argname.data(), name);
		}
		if(!confirmname(argname)) {
			asar_throw_error(0, error_type_block, error_id_invalid_param_name, argname.data());
		}
		arg_indices.emplace(std::move(argname), i);
	}

	parse_context ctx{ content, arg_indices };
	auto parsed = ctx.parse();
	math_user_function userfunc = { std::move(parsed), arg_count };
	user_functions.emplace(name, std::move(userfunc));
}

double math(const char * str)
{
	parse_context parse_ctx { str, {}};
	owned_node parsed = parse_ctx.parse();
	int haslabel = parsed->has_label();
	foundlabel = haslabel > 0;
	foundlabel_static = haslabel < 2;
	forwardlabel=false; // TODO
	math_eval_context ctx;
	math_val rval = parsed->evaluate(ctx);
	return rval.get_double();
}

int64_t getnum(const char* instr)
{
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

int getlen(const char * orgstr, bool optimizebankextraction) {
	parse_context parse_ctx { orgstr, {}};
	owned_node parsed = parse_ctx.parse();
	int letgen = parsed->get_len(optimizebankextraction);
	return letgen;
}

void initmathcore()
{
	user_functions.clear();
}

void deinitmathcore()
{
	//not needed
}
