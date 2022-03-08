#include <memory>
#include <vector>
#include "assocarr.h"
using std::unique_ptr;

enum class math_val_type {
	floating,
	integer,
	string,
	identifier,
};

class math_val {
public:
	math_val_type m_type;
	union {
		double double_;
		int64_t int_;
	} m_numeric_val;
	// slightly hacky but this is used for both labelname in case of
	// m_type=identifier, and string value in case of m_type=string
	string m_string_val;

	math_val() {
		m_type = math_val_type::integer;
		m_numeric_val.int_ = 0;
	}
	math_val(double v) {
		m_type = math_val_type::floating;
		m_numeric_val.double_ = v;
	}
	math_val(int64_t v) {
		m_type = math_val_type::integer;
		m_numeric_val.int_ = v;
	}
	math_val(string v) {
		m_type = math_val_type::string;
		m_string_val = v;
	}

	double get_double() const {
		switch(m_type) {
			case math_val_type::floating:
				return m_numeric_val.double_;
			case math_val_type::integer:
				return (double)m_numeric_val.int_;
			case math_val_type::identifier:
				return (double)get_integer();
			case math_val_type::string:
				asar_throw_error(2, error_type_block, error_id_expected_number);
		}
	}

	int64_t get_integer() const {
		switch(m_type) {
			case math_val_type::floating:
				return (int64_t)m_numeric_val.double_;
			case math_val_type::integer:
				return m_numeric_val.int_;
			case math_val_type::identifier:
				// the entire label value code here i guess
				// structs??? i think those can always be parsed to statics and thus
				// directly to integer values though
				// or not???
				// we can parse it to an expression involving simple labels
				// maybe???
				return labelval(m_string_val).pos;
			case math_val_type::string:
				asar_throw_error(2, error_type_block, error_id_expected_number);
		}
	}
	const string& get_str() const {
		if(m_type == math_val_type::string) return m_string_val;
		asar_throw_error(2, error_type_block, error_id_expected_string);
	}
};

// any info that's necessary during evaluation
class math_eval_context {
public:
	std::vector<math_val> userfunc_params;
};

class math_ast_node {
public:
	virtual math_val evaluate(const math_eval_context&) = 0;
	// 0 - no label, 1 - static label, 3 - nonstatic label.
	// actually i think doing 0 - static label, 1 - backward label, 3 - forward
	// label would be better
	virtual int has_label() = 0;
	virtual ~math_ast_node() = default;
};

enum class math_binop_type {
	pow,
	mul,
	div,
	mod,
	add,
	sub,
	shift_left,
	shift_right,
	bit_and,
	bit_or,
	bit_xor,
	comp_ge,
	comp_le,
	comp_gt,
	comp_lt,
	comp_eq,
	comp_ne,
};

template<typename T> T eval_arith(T lhs, T rhs, math_binop_type type) {
	switch(type) {
		case math_binop_type::mul: return lhs * rhs;
		case math_binop_type::add: return lhs + rhs;
		case math_binop_type::sub: return lhs - rhs;
		case math_binop_type::comp_ge: return lhs >= rhs;
		case math_binop_type::comp_le: return lhs <= rhs;
		case math_binop_type::comp_gt: return lhs > rhs;
		case math_binop_type::comp_lt: return lhs < rhs;
		case math_binop_type::comp_eq: return lhs == rhs;
		case math_binop_type::comp_ne: return lhs != rhs;
		default:
			// this should never happen
			__builtin_trap();
	}
}

class math_ast_binop : public math_ast_node {
public:
	unique_ptr<math_ast_node> m_left, m_right;
	math_binop_type m_type;
	math_ast_binop(math_ast_node* left_in, math_ast_node* right_in, math_binop_type type_in)
		: m_left(left_in), m_right(right_in), m_type(type_in) {}


	math_val evaluate(const math_eval_context& ctx) {
		math_val lhs = m_left->evaluate(ctx);
		math_val rhs = m_right->evaluate(ctx);
		if(lhs.m_type == math_val_type::floating) rhs = math_val(rhs.get_double());
		else if(rhs.m_type == math_val_type::floating) lhs = math_val(lhs.get_double());
		switch(m_type) {
			case math_binop_type::pow: return math_val(pow(lhs.get_double(), rhs.get_double()));
			case math_binop_type::div:
				if(rhs.get_double() == 0.0) asar_throw_error(2, error_type_block, error_id_division_by_zero);
				return math_val(lhs.get_double() / rhs.get_double());
			case math_binop_type::mod:
				if(rhs.get_double() == 0.0) asar_throw_error(2, error_type_block, error_id_division_by_zero);
				// TODO: negative semantics
				if(lhs.m_type == math_val_type::floating) {
					return math_val(fmod(lhs.get_double(), rhs.get_double()));
				} else {
					return math_val(lhs.get_integer() % rhs.get_integer());
				}
				break;
			
			case math_binop_type::shift_left:
				{
					int64_t rhs_v = rhs.get_integer();
					if(rhs_v < 0) asar_throw_error(2, error_type_block, error_id_negative_shift);
					return math_val(lhs.get_integer() << (uint64_t)rhs_v);
				}
			case math_binop_type::shift_right:
				{
					int64_t rhs_v = rhs.get_integer();
					if(rhs_v < 0) asar_throw_error(2, error_type_block, error_id_negative_shift);
					return math_val(lhs.get_integer() >> (uint64_t)rhs_v);
				}

			case math_binop_type::bit_and: return math_val(lhs.get_integer() & rhs.get_integer());
			case math_binop_type::bit_or:  return math_val(lhs.get_integer() | rhs.get_integer());
			case math_binop_type::bit_xor: return math_val(lhs.get_integer() ^ rhs.get_integer());

			default:
				if(lhs.m_type == math_val_type::floating)
					return math_val(eval_arith<double>(lhs.get_double(), rhs.get_double(), m_type));
				else
					return math_val(eval_arith<int64_t>(lhs.get_integer(), rhs.get_integer(), m_type));
		}
	}

	int has_label() {
		return m_left->has_label() | m_right->has_label();
	}
};

enum class math_unop_type {
	neg,
	bit_not,
	bank_extract,
};

class math_ast_unop : public math_ast_node {
public:
	unique_ptr<math_ast_node> m_arg;
	math_unop_type m_type;
	math_ast_unop(math_ast_node* arg_in, math_unop_type type_in)
		: m_arg(arg_in), m_type(type_in) {}
	math_val evaluate(const math_eval_context& ctx) {
		math_val arg = m_arg->evaluate(ctx);
		switch(m_type) {
			case math_unop_type::neg:
				if(arg.m_type == math_val_type::floating) return math_val(-arg.get_double());
				else return math_val(-arg.get_integer());
			case math_unop_type::bit_not: return math_val(~arg.get_integer());
			case math_unop_type::bank_extract: return math_val(arg.get_integer() >> 16);
		}
	}
	int has_label() {
		return m_arg->has_label();
	}
};

class math_ast_literal : public math_ast_node {
	math_val m_value;
public:
	math_ast_literal(math_val value) : m_value(value) {}
	math_val evaluate(const math_eval_context& ctx) { return m_value; }
	int has_label() { return 0; }
};

// TODO: move this somewhere better - it's needed here though
// (also give it a better name...)
extern string ns;
class math_ast_label : public math_ast_node {
	string m_labelname;
	// current namespace when this label was referenced
	string m_cur_ns;
public:
	// this should be the output of labelname() already
	math_ast_label(string& labelname)
		: m_labelname(labelname), m_cur_ns(ns) {}
	math_val evaluate(const math_eval_context& ctx) {
		if(m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
			math_val v = math_val(m_cur_ns + m_labelname);
			v.m_type = math_val_type::identifier;
			return v;
		}
		else if(labels.exists(m_labelname)) {
			math_val v = math_val(m_labelname);
			v.m_type = math_val_type::identifier;
			return v;
		}
		else {
			// i think in this context we always should throw???
			asar_throw_error(2, error_type_block, error_id_label_not_found, m_labelname.data());
		}
	}
	int has_label() {
		/*if(m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
			return labels.find(m_cur_ns + m_labelname).is_static ? 1 : 3;
		}
		else if(labels.exists(m_labelname)) {
			return labels.find(m_labelname).is_static ? 1 : 3;
		}*/
		// otherwise, non-static label
		return 3;
	}
};

class math_function {
public:
	virtual math_val call(const std::vector<math_val>& args) = 0;
	virtual int has_label() { return 0; }
};


template<double (*F)(double)>
class math_unary_real_function : public math_function {
	math_val call(const std::vector<math_val>& args) {
		if(args.size() != 1)
			asar_throw_error(2, error_type_block, error_id_argument_count, 1, (int)args.size());
		double val = F(args[0].get_double());
		if (val != val) asar_throw_error(2, error_type_block, error_id_nan);
		return math_val(val);
	}
};

std::unordered_map<std::string, unique_ptr<math_function>> builtin_functions_ = {
//	{"sqrt", std::make_unique<math_unary_real_function<sqrt>>()},
};
assocarr<math_function> all_functions;

class math_user_function : public math_function {
public:
	int m_arg_count;
	unique_ptr<math_ast_node> m_func_body;
	math_val call(const std::vector<math_val>& args) {
		math_eval_context new_ctx;
		new_ctx.userfunc_params = args;
		if(args.size() != m_arg_count)
			asar_throw_error(2, error_type_block, error_id_argument_count, m_arg_count, (int)args.size());
		return m_func_body->evaluate(new_ctx);
	}
	int has_label() {
		return m_func_body->has_label();
	}
};

class math_ast_function_call : public math_ast_node {
public:
	std::vector<unique_ptr<math_ast_node>> m_arguments;
	math_function* m_func;
	math_ast_function_call(std::vector<math_ast_node*> args, string function_name) {
		for(math_ast_node* p : args) {
			m_arguments.push_back(unique_ptr<math_ast_node>(p));
		}
		if(all_functions.exists(function_name)) {
			m_func = &all_functions.find(function_name);
		} else {
			asar_throw_error(2, error_type_block, error_id_function_not_found, function_name);
		}
	}
	math_val evaluate(const math_eval_context& ctx) {
		std::vector<math_val> arg_vals;
		for(auto const& p : m_arguments) {
			arg_vals.push_back(p->evaluate(ctx));
		}
		return m_func->call(arg_vals);
	}
	int has_label() {
		int out = m_func->has_label();
		for(auto const& p : m_arguments) {
			out |= p->has_label();
		}
		return out;
	}
};

// only for use inside user function definitions
class math_ast_function_argument : public math_ast_node {
	int m_arg_idx;
	math_ast_function_argument(int arg_idx) : m_arg_idx(arg_idx) {}

	math_val evaluate(const math_eval_context& ctx) {
		return ctx.userfunc_params[m_arg_idx];
	}

	// if a function is called with a label as an argument, that gets checked by
	// the function call node, not here
	int has_label() {
		return 0;
	}
};

