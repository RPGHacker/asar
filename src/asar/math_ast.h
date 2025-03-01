#include <memory>
#include <variant>
#include <vector>
#include <unordered_map>
#include "libstr.h"
#include "asar_math.h"
#include "assembleblock.h"

using owned_node = std::unique_ptr<math_ast_node>;

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

math_val evaluate_binop(math_val lhs, math_val rhs, math_binop_type type);

class math_ast_binop : public math_ast_node {
public:
	owned_node m_left, m_right;
	math_binop_type m_type;
	math_ast_binop(owned_node left_in, owned_node right_in, math_binop_type type_in)
	: m_left(std::move(left_in)), m_right(std::move(right_in)), m_type(type_in) {}

	math_val evaluate(const eval_context &ctx) const;
	int has_label() const;
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
	math_val evaluate(const eval_context &ctx) const;
	int has_label() const;
	int get_len(bool could_be_bank_ex) const;
};

class math_ast_literal : public math_ast_node {
	math_val m_value;
	int m_len;
public:
	math_ast_literal(math_val value, int len=0) : m_value(value), m_len(len) {}
	math_val evaluate(const eval_context& ctx) const { return m_value; }
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
	math_val evaluate(const eval_context &ctx) const;
	int has_label() const;
	int get_len(bool could_be_bank_ex) const;
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
	math_val call(const std::vector<math_val> &args) const;
	int has_label() const;
	int get_len(const std::vector<owned_node> &args, bool could_be_bank_ex) const;
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
	static math_function_ref lookup_fname(string const &function_name);

public:
	math_ast_function_call(std::vector<owned_node> args, string function_name)
		: m_arguments(std::move(args))
		, m_func(lookup_fname(function_name)) {}
	math_val evaluate(const eval_context &ctx) const;
	int has_label() const;
	int get_len(bool could_be_bank_ex) const;
};

// only for use inside user function definitions
class math_ast_function_argument : public math_ast_node {
	size_t m_arg_idx;
public:
	math_ast_function_argument(size_t arg_idx) : m_arg_idx(arg_idx) {}

	math_val evaluate(const eval_context& ctx) const {
		return ctx.userfunc_params[m_arg_idx];
	}

	// if a function is called with a label as an argument, that gets checked by
	// the function call node, not here
	int has_label() const { return 0; }
	// i don't think these should ever have their len gotten?
	int get_len(bool could_be_bank_ex) const { return 0; }
};
