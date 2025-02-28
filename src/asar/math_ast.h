#include <memory>
#include <variant>
#include <vector>
#include "libstr.h"
#include "asar.h" // todo assembleblock.h sux
#include "assembleblock.h"
#include "errors.h"
using std::unique_ptr;

enum class math_val_type {
	floating,
	integer,
	string,
	// a resolved label with a name and value. used for datasize() and whatnot
	identifier,
};

inline int64_t float_to_int(double f) {
	// TODO: throw error on overflow?
	return (int64_t)f;
}

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

	// TODO: make all these conversions print the current type aswell instead of just expected type
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
				return float_to_int(m_numeric_val.double_);
			case math_val_type::integer:
				return m_numeric_val.int_;
			case math_val_type::identifier:
				if(!labels.exists(m_string_val))
					// i'm not sure if it's even possible to reach here, anything
					// that constructs identifiers should already check for existence...
					asar_throw_error(2, error_type_block, error_id_label_not_found, m_string_val.data());
				return labels.find(m_string_val).pos;
			case math_val_type::string:
				asar_throw_error(2, error_type_block, error_id_expected_number);
		}
	}
	const string& get_str() const {
		if(m_type == math_val_type::string) return m_string_val;
		asar_throw_error(2, error_type_block, error_id_expected_string);
	}
	const string& get_identifier() const {
		if(m_type == math_val_type::identifier) return m_string_val;
		asar_throw_error(2, error_type_block, error_id_expected_ident);
	}

	bool get_bool() const {
		switch(m_type) {
			case math_val_type::floating:
				return get_double() != 0.0;
			case math_val_type::integer:
			case math_val_type::identifier:
				return get_integer() != 0;
			case math_val_type::string:
				return get_str().length() != 0;
		}
	}
};

// any info that's necessary during evaluation
class math_eval_context {
public:
	// TODO would it be faster to make this a reference? does that avoid any significant copies?
	std::vector<math_val> userfunc_params;
};

class math_ast_node {
public:
	virtual math_val evaluate(const math_eval_context&) const = 0;
	// 0 - no label, 1 - static label, 3 - nonstatic label.
	// (maybe tracking forwardlabel too would be good?)
	virtual int has_label() const = 0;
	// how many bytes long should the result of this expression be?
	virtual int get_len(bool could_be_bank_ex) const = 0;
	virtual ~math_ast_node() = default;
};

using owned_node = unique_ptr<math_ast_node>;

enum class math_binop_type {
	pow,          // **
	mul,          // *
	div,          // /
	mod,          // %
	add,          // +
	sub,          // -
	shift_left,   // <<
	shift_right,  // >>
	bit_and,      // &
	bit_or,       // |
	bit_xor,      // ^
	logical_and,  // &&
	logical_or,   // ||
	comp_ge,      // >=
	comp_le,      // <=
	comp_gt,      // >
	comp_lt,      // <
	comp_eq,      // ==
	comp_ne,      // !=
};

template<typename T>
T evaluate_binop_arithmetic(T lhs, T rhs, math_binop_type type) {
	switch(type) {
		case math_binop_type::mul: return lhs * rhs;
		case math_binop_type::add: return lhs + rhs;
		case math_binop_type::sub: return lhs - rhs;
		default:
			asar_throw_error(2, error_type_block, error_id_internal_error, "evaluate_binop_arithmetic with bad type");
	}
}
template<typename T>
bool evaluate_binop_compare(T lhs, T rhs, math_binop_type type) {
	switch(type) {
		case math_binop_type::comp_ge: return lhs >= rhs;
		case math_binop_type::comp_le: return lhs <= rhs;
		case math_binop_type::comp_gt: return lhs > rhs;
		case math_binop_type::comp_lt: return lhs < rhs;
		case math_binop_type::comp_eq: return lhs == rhs;
		case math_binop_type::comp_ne: return lhs != rhs;
		default:
			asar_throw_error(2, error_type_block, error_id_internal_error, "evaluate_binop_compare with bad type");
	}
}

static math_val evaluate_binop(math_val lhs, math_val rhs, math_binop_type type) {
	// todo: do this a bit smarter (bit_ops shouldn't cast int->float->int)
	if(lhs.m_type == math_val_type::floating) rhs = math_val(rhs.get_double());
	else if(rhs.m_type == math_val_type::floating) lhs = math_val(lhs.get_double());
	switch(type) {
		case math_binop_type::pow: return math_val(pow(lhs.get_double(), rhs.get_double()));
		case math_binop_type::div:
			if(rhs.get_double() == 0.0)
				asar_throw_error(2, error_type_block, error_id_division_by_zero);
			return math_val(lhs.get_double() / rhs.get_double());
		case math_binop_type::mod:
			if(rhs.get_double() == 0.0)
				asar_throw_error(2, error_type_block, error_id_division_by_zero);
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
				if(rhs_v < 0)
					asar_throw_error(2, error_type_block, error_id_negative_shift);
				return math_val(lhs.get_integer() << (uint64_t)rhs_v);
			}
		case math_binop_type::shift_right:
			{
				int64_t rhs_v = rhs.get_integer();
				if(rhs_v < 0)
					asar_throw_error(2, error_type_block, error_id_negative_shift);
				return math_val(lhs.get_integer() >> (uint64_t)rhs_v);
			}

		case math_binop_type::bit_and: return math_val(lhs.get_integer() & rhs.get_integer());
		case math_binop_type::bit_or:  return math_val(lhs.get_integer() | rhs.get_integer());
		case math_binop_type::bit_xor: return math_val(lhs.get_integer() ^ rhs.get_integer());

		case math_binop_type::logical_and:
		case math_binop_type::logical_or:
			asar_throw_error(2, error_type_block, error_id_internal_error, "evaluate_binop() on logical ops loses short-circuiting");

		case math_binop_type::mul:
		case math_binop_type::add:
		case math_binop_type::sub:
			// TODO error on string (also TODO support string +)
			if(lhs.m_type == math_val_type::floating)
				return math_val(evaluate_binop_arithmetic<double>(lhs.get_double(), rhs.get_double(), type));
			else
				return math_val(evaluate_binop_arithmetic<int64_t>(lhs.get_integer(), rhs.get_integer(), type));

		case math_binop_type::comp_ge:
		case math_binop_type::comp_le:
		case math_binop_type::comp_gt:
		case math_binop_type::comp_lt:
		case math_binop_type::comp_eq:
		case math_binop_type::comp_ne:
			if(lhs.m_type == math_val_type::floating)
				return math_val((int64_t)evaluate_binop_compare<double>(lhs.get_double(), rhs.get_double(), type));
			else
				return math_val((int64_t)evaluate_binop_compare<int64_t>(lhs.get_integer(), rhs.get_integer(), type));
	}
}

class math_ast_binop : public math_ast_node {
public:
	owned_node m_left, m_right;
	math_binop_type m_type;
	math_ast_binop(owned_node left_in, owned_node right_in, math_binop_type type_in)
		: m_left(std::move(left_in)), m_right(std::move(right_in)), m_type(type_in) {}


	math_val evaluate(const math_eval_context& ctx) const {
		math_val lhs = m_left->evaluate(ctx);

		// handle short-circuiting for || and &&
		if(m_type == math_binop_type::logical_or) {
			if(lhs.get_bool() == true) return math_val((int64_t)true);
			math_val rhs = m_right->evaluate(ctx);
			return math_val((int64_t)rhs.get_bool());
		}
		if(m_type == math_binop_type::logical_and) {
			if(lhs.get_bool() == false) return math_val((int64_t)false);
			math_val rhs = m_right->evaluate(ctx);
			return math_val((int64_t)rhs.get_bool());
		}

		math_val rhs = m_right->evaluate(ctx);
		return evaluate_binop(lhs, rhs, m_type);
	}

	int has_label() const {
		return m_left->has_label() | m_right->has_label();
	}

	int get_len(bool could_be_bank_ex) const;
};

enum class math_unop_type {
	neg,
	bit_not,
	bank_extract,
};

class math_ast_unop : public math_ast_node {
public:
	owned_node m_arg;
	math_unop_type m_type;
	math_ast_unop(owned_node arg_in, math_unop_type type_in)
		: m_arg(std::move(arg_in)), m_type(type_in) {}
	math_val evaluate(const math_eval_context& ctx) const {
		math_val arg = m_arg->evaluate(ctx);
		switch(m_type) {
			case math_unop_type::neg:
				if(arg.m_type == math_val_type::floating) return math_val(-arg.get_double());
				else return math_val(-arg.get_integer());
			case math_unop_type::bit_not: return math_val(~arg.get_integer());
			case math_unop_type::bank_extract: return math_val(arg.get_integer() >> 16);
		}
	}
	int has_label() const {
		return m_arg->has_label();
	}
	int get_len(bool could_be_bank_ex) const {
		if(could_be_bank_ex && m_type == math_unop_type::bank_extract)
			return 1;
		return m_arg->get_len(false);
	}
};

class math_ast_literal : public math_ast_node {
	math_val m_value;
	int m_len;
public:
	math_ast_literal(math_val value, int len=0) : m_value(value), m_len(len) {}
	math_val evaluate(const math_eval_context& ctx) const { return m_value; }
	int has_label() const { return 0; }
	int get_len(bool could_be_bank_ex) const { return m_len; }
	friend int math_ast_binop::get_len(bool) const;
};

class math_ast_label : public math_ast_node {
	string m_labelname;
	// current namespace when this label was referenced
	string m_cur_ns;
public:
	// this should be the output of labelname() already
	math_ast_label(string labelname)
		: m_labelname(labelname)
		// this is initialized with the global ns
		, m_cur_ns(ns) {}
	math_val evaluate(const math_eval_context& ctx) const {
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
	int has_label() const {
		if(m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
			return labels.find(m_cur_ns + m_labelname).is_static ? 1 : 3;
		}
		else if(labels.exists(m_labelname)) {
			return labels.find(m_labelname).is_static ? 1 : 3;
		}
		// otherwise, non-static label
		return 3;
	}

	int get_len(bool could_be_bank_ex) const {
		snes_label label;
		if(m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
			label = labels.find(m_cur_ns + m_labelname);
		}
		else if(labels.exists(m_labelname)) {
			label = labels.find(m_labelname);
		}
		else return 2;
		return getlenforlabel(label, true);
	}
};


class math_builtin_function {
	using call_t = math_val(*)(const std::vector<math_val>& args);
	using haslabel_t = int(*)();
	using getlen_t = int(*)(const std::vector<owned_node>& args, bool could_be_bank_ex);
	static int default_has_label() {
		return 0;
	}
	static int default_get_len(const std::vector<owned_node>& args, bool could_be_bank_ex) {
		int res = 0;
		for(auto& v : args) {
			res = std::max(res, v->get_len(false));
		}
		return res;
	}
	call_t m_call;
	haslabel_t m_haslabel;
	getlen_t m_getlen;
public:
	math_builtin_function(call_t c, haslabel_t l = default_has_label, getlen_t gl = default_get_len)
		: m_call(c), m_haslabel(l), m_getlen(gl) {}
	math_builtin_function(call_t c, getlen_t gl)
		: m_call(c), m_haslabel(default_has_label), m_getlen(gl) {}
	math_val call(const std::vector<math_val>& args) const {
		return m_call(args);
	}
	int has_label() const {
		return m_haslabel();
	}
	int get_len(const std::vector<owned_node>& args, bool could_be_bank_ex) const {
		return m_getlen(args, could_be_bank_ex);
	}
};

class math_user_function {
	int m_arg_count;
	owned_node m_func_body;
public:
	math_user_function(owned_node body, size_t arg_count)
		: m_arg_count(arg_count), m_func_body(std::move(body)) {}
	math_val call(const std::vector<math_val>& args) const {
		math_eval_context new_ctx;
		new_ctx.userfunc_params = args;
		if(args.size() != m_arg_count)
			asar_throw_error(2, error_type_block, error_id_argument_count, m_arg_count, (int)args.size());
		return m_func_body->evaluate(new_ctx);
	}
	int has_label() const {
		return m_func_body->has_label();
	}
	int get_len(const std::vector<owned_node>& args, bool could_be_bank_ex) const {
		// TODO: this doesn't forward could_be_bank_ex to the fn call...
		// supporting that properly would require stringing some context through all get_len calls
		// supporting it less properly (making a special return value of get_len signify bankextract) could be viable tho...
		int len = m_func_body->get_len(false);
		for(auto& arg : args) {
			len = std::max(len, arg->get_len(false));
		}
		return len;
	}
};

class math_function_ref {
public:
	std::variant<const math_builtin_function*, const math_user_function*> inner;
	math_function_ref(const math_builtin_function& fn) : inner(&fn) {}
	math_function_ref(const math_user_function& fn) : inner(&fn) {}
	math_val call(const std::vector<math_val>& args) const {
		return std::visit([&](auto& i) { return i->call(args); }, inner);
	}
	int has_label() const {
		return std::visit([&](auto& i) { return i->has_label(); }, inner);
	}
	int get_len(const std::vector<owned_node>& args, bool could_be_bank_ex) const {
		return std::visit([&](auto& i) { return i->get_len(args, could_be_bank_ex); }, inner);
	}
};

extern std::unordered_map<string, math_user_function> user_functions;
extern const std::unordered_map<string, math_builtin_function> builtin_functions;

class math_ast_function_call : public math_ast_node {
	std::vector<owned_node> m_arguments;
	math_function_ref m_func;
	static math_function_ref lookup_fname(string const& function_name) {
		if(auto it = user_functions.find(function_name); it != user_functions.end()) {
			return it->second;
		} else if(auto it = builtin_functions.find(function_name); it != builtin_functions.end()) {
			return it->second;
		} else {
			asar_throw_error(2, error_type_block, error_id_function_not_found, function_name.data());
		}
	}

public:
	math_ast_function_call(std::vector<owned_node> args, string function_name)
		: m_arguments(std::move(args))
		, m_func(lookup_fname(function_name)) {}
	math_val evaluate(const math_eval_context& ctx) const {
		std::vector<math_val> arg_vals;
		for(auto const& p : m_arguments) {
			arg_vals.push_back(p->evaluate(ctx));
		}
		return m_func.call(arg_vals);
	}
	int has_label() const {
		int out = m_func.has_label();
		for(auto const& p : m_arguments) {
			out |= p->has_label();
		}
		return out;
	}
	int get_len(bool could_be_bank_ex) const {
		return m_func.get_len(m_arguments, could_be_bank_ex);
	}
};

// only for use inside user function definitions
class math_ast_function_argument : public math_ast_node {
	size_t m_arg_idx;
public:
	math_ast_function_argument(size_t arg_idx) : m_arg_idx(arg_idx) {}

	math_val evaluate(const math_eval_context& ctx) const {
		return ctx.userfunc_params[m_arg_idx];
	}

	// if a function is called with a label as an argument, that gets checked by
	// the function call node, not here
	int has_label() const { return 0; }
	// i don't think these should ever have their len gotten?
	int get_len(bool could_be_bank_ex) const { return 0; }
};

inline int math_ast_binop::get_len(bool could_be_bank_ex) const {
	if(could_be_bank_ex) {
		int want_rhs = 0;
		if(m_type == math_binop_type::div) {
			want_rhs = 65536;
		} else if(m_type == math_binop_type::shift_right) {
			want_rhs = 16;
		}
		if(want_rhs) {
			math_ast_node* right_ptr = m_right.get();
			auto right_lit = dynamic_cast<math_ast_literal*>(right_ptr);
			if(right_lit && right_lit->m_value.m_type == math_val_type::integer) {
				int64_t right_val = right_lit->m_value.get_integer();
				if(right_val == want_rhs) return 1;
			}
		}
	}
	return std::max(m_left->get_len(false), m_right->get_len(false));
}
