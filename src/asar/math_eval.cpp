#include "math_ast.h"
#include "asar.h"
#include "errors.h"

double math_val::get_double() const {
	switch(m_type) {
		case math_val_type::floating:
			return m_numeric_val.double_;
		case math_val_type::integer:
			return (double)m_numeric_val.int_;
		case math_val_type::identifier:
			return (double)get_integer();
		case math_val_type::string:
			throw_err_block(2, err_bad_type, "number", "string");
	}
	// this is actually unreachable because the switch case above is
	// exhaustive, but c++ standard allows storing invalid values inside
	// enums, so everything except clang complains about it...
	throw_err_block(pass, err_internal_error, "math_val invalid type");
}
int64_t math_val::get_integer() const {
	switch(m_type) {
		case math_val_type::floating:
			// TODO: throw error on overflow?
			return (int64_t)m_numeric_val.double_;
		case math_val_type::integer:
			return m_numeric_val.int_;
		case math_val_type::identifier:
			if(!labels.exists(m_string_val)) {
				throw_err_block(pass, err_internal_error, "evaluating nonexistent label");
			}
			return labels.find(m_string_val).pos;
		case math_val_type::string:
			throw_err_block(2, err_bad_type, "number", "string");
	}
	throw_err_block(pass, err_internal_error, "math_val invalid type");
}
const string &math_val::get_str() const {
	if (m_type == math_val_type::string)
		return m_string_val;
	throw_err_block(2, err_bad_type, "string", "number"); // TODO is "number" a good name for identifier/int/float?
}

const string &math_val::get_identifier() const {
	if (m_type == math_val_type::identifier)
		return m_string_val;
	const char* type_name = (m_type == math_val_type::string) ? "string" : "number";
	throw_err_block(2, err_bad_type, "identifier", type_name);
}
bool math_val::get_bool() const {
	switch (m_type) {
		case math_val_type::floating:
			return get_double() != 0.0;
		case math_val_type::integer:
		case math_val_type::identifier:
			return get_integer() != 0;
		case math_val_type::string:
			return get_str().length() != 0;
	}
	throw_err_block(pass, err_internal_error, "math_val invalid type");
}

template<typename T>
bool evaluate_binop_compare(const T& lhs, const T& rhs, math_binop_type type) {
	switch(type) {
		case math_binop_type::comp_ge: return lhs >= rhs;
		case math_binop_type::comp_le: return lhs <= rhs;
		case math_binop_type::comp_gt: return lhs > rhs;
		case math_binop_type::comp_lt: return lhs < rhs;
		default:
			throw_err_block(2, err_internal_error, "evaluate_binop_compare with bad type");
	}
}

static bool evaluate_eq(math_val lhs, math_val rhs) {
	// if only one is string, they can never be equal
	if((lhs.m_type == math_val_type::string) != (rhs.m_type == math_val_type::string)) {
		return false;
	}
	if(lhs.m_type == math_val_type::string) {
		// both strings: compare as strings
		return lhs.get_str() == rhs.get_str();
	} else {
		// both non strings: compare as numbers
		// compare as both types, to ensure rounding errors don't return things like 0x5555555555555555 == 0x5555555555555556+511.0
		// (but omit integer compare if that'd overflow)
		return lhs.get_double() == rhs.get_double() && (fabs(lhs.get_double()) > INT64_MAX || lhs.get_integer() == rhs.get_integer());
	}
}

math_val evaluate_binop(math_val lhs, math_val rhs,
						math_binop_type type) {
	bool has_string = (lhs.m_type == math_val_type::string) || (rhs.m_type == math_val_type::string);
	bool has_float = (lhs.m_type == math_val_type::floating) || (rhs.m_type == math_val_type::floating);
	switch (type) {
		case math_binop_type::pow:
			return math_val(pow(lhs.get_double(), rhs.get_double()));
		case math_binop_type::div:
			if (rhs.get_double() == 0.0)
				throw_err_block(2, err_division_by_zero);
			return math_val(lhs.get_double() / rhs.get_double());
		case math_binop_type::mod:
			if (rhs.get_double() == 0.0)
				throw_err_block(2, err_division_by_zero);
			// TODO: negative semantics
			if (has_float) {
				return math_val(fmod(lhs.get_double(), rhs.get_double()));
			} else {
				return math_val(lhs.get_integer() % rhs.get_integer());
			}
			break;

		case math_binop_type::shift_left: {
			int64_t rhs_v = rhs.get_integer();
			if (rhs_v < 0)
				throw_err_block(2, err_negative_shift);
			return math_val(lhs.get_integer() << (uint64_t)rhs_v);
		}
		case math_binop_type::shift_right: {
			int64_t rhs_v = rhs.get_integer();
			if (rhs_v < 0)
				throw_err_block(2, err_negative_shift);
			return math_val(lhs.get_integer() >> (uint64_t)rhs_v);
		}

		case math_binop_type::bit_and:
			return math_val(lhs.get_integer() & rhs.get_integer());
		case math_binop_type::bit_or:
			return math_val(lhs.get_integer() | rhs.get_integer());
		case math_binop_type::bit_xor:
			return math_val(lhs.get_integer() ^ rhs.get_integer());

		case math_binop_type::logical_and:
		case math_binop_type::logical_or:
			throw_err_block(2, err_internal_error, "evaluate_binop() on logical ops loses short-circuiting");

		case math_binop_type::add:
			if(has_string) return math_val(lhs.get_str() + rhs.get_str());
			else if(has_float) return math_val(lhs.get_double() + rhs.get_double());
			else return math_val(lhs.get_integer() + rhs.get_integer());
		case math_binop_type::mul:
			if(has_float) return math_val(lhs.get_double() * rhs.get_double());
			else return math_val(lhs.get_integer() * rhs.get_integer());
		case math_binop_type::sub:
			if(has_float) return math_val(lhs.get_double() - rhs.get_double());
			else return math_val(lhs.get_integer() - rhs.get_integer());

		case math_binop_type::comp_ge:
		case math_binop_type::comp_le:
		case math_binop_type::comp_gt:
		case math_binop_type::comp_lt:
			if(has_string) return (int64_t)evaluate_binop_compare(lhs.get_str(), rhs.get_str(), type);
			else if(has_float) return (int64_t)evaluate_binop_compare(lhs.get_double(), rhs.get_double(), type);
			else return (int64_t)evaluate_binop_compare(lhs.get_integer(), rhs.get_integer(), type);

		case math_binop_type::comp_eq: return (int64_t)evaluate_eq(lhs, rhs);
		case math_binop_type::comp_ne: return (int64_t)!evaluate_eq(lhs, rhs);
	}
	throw_err_block(pass, err_internal_error, "evaluate_binop invalid binop");
}

math_val math_ast_binop::evaluate(const eval_context &ctx) const {
	math_val lhs = m_left->evaluate(ctx);

	// handle short-circuiting for || and &&
	if (m_type == math_binop_type::logical_or) {
		if (lhs.get_bool() == true)
			return math_val((int64_t)true);
		math_val rhs = m_right->evaluate(ctx);
		return math_val((int64_t)rhs.get_bool());
	}
	if (m_type == math_binop_type::logical_and) {
		if (lhs.get_bool() == false)
			return math_val((int64_t)false);
		math_val rhs = m_right->evaluate(ctx);
		return math_val((int64_t)rhs.get_bool());
	}

	math_val rhs = m_right->evaluate(ctx);
	return evaluate_binop(lhs, rhs, m_type);
}

int math_ast_binop::has_label(const static_context& ctx) const {
	return m_left->has_label(ctx) | m_right->has_label(ctx);
}


int math_ast_binop::get_len(bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
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
	return std::max(m_left->get_len(false, ctx), m_right->get_len(false, ctx));
}
math_val math_ast_unop::evaluate(const eval_context &ctx) const {
	math_val arg = m_arg->evaluate(ctx);
	switch (m_type) {
		case math_unop_type::neg:
			if (arg.m_type == math_val_type::floating)
				return math_val(-arg.get_double());
			else
				return math_val(-arg.get_integer());
		case math_unop_type::bit_not:
			return math_val(~arg.get_integer());
		case math_unop_type::bank_extract:
			return math_val(arg.get_integer() >> 16);
	}
	throw_err_block(pass, err_internal_error, "evaluate_unop invalid unop");
}

int math_ast_unop::has_label(const static_context& ctx) const { return m_arg->has_label(ctx); }

int math_ast_unop::get_len(bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
	if (could_be_bank_ex && m_type == math_unop_type::bank_extract)
		return 1;
	return m_arg->get_len(false, ctx);
}

math_val math_ast_ternary_cond::evaluate(const eval_context& ctx) const {
	if(m_cond->evaluate(ctx).get_bool()) {
		return m_true->evaluate(ctx);
	} else {
		return m_false->evaluate(ctx);
	}
}

int math_ast_ternary_cond::has_label(const static_context& ctx) const {
	return m_cond->has_label(ctx) | m_true->has_label(ctx) | m_false->has_label(ctx);
}

int math_ast_ternary_cond::get_len(bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
	return std::max(m_true->get_len(false, ctx), m_false->get_len(false, ctx));
}

math_val math_ast_label::evaluate(const eval_context &ctx) const {
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		return math_val::make_identifier(m_cur_ns + m_labelname);
	} else if (labels.exists(m_labelname)) {
		return math_val::make_identifier(m_labelname);
	} else {
		// possibly forward label, assume without namespace.
		// TODO: this assumption can cause moving labels :)))))
		if(pass == 0) return math_val::make_identifier(m_labelname);
		// if not pass 0, we know it's not a forward label and can throw the error
		throw_err_block(2, err_label_not_found, m_labelname.data());
	}
}

int math_ast_label::has_label(const static_context& ctx) const {
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		return labels.find(m_cur_ns + m_labelname).is_static ? 1 : 3;
	} else if (labels.exists(m_labelname)) {
		return labels.find(m_labelname).is_static ? 1 : 3;
	}
	// otherwise, non-static forward label
	return 7;
}

int math_ast_label::get_len(bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
	snes_label label;
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		label = labels.find(m_cur_ns + m_labelname);
	} else if (labels.exists(m_labelname)) {
		label = labels.find(m_labelname);
	} else return 2;
	return getlenforlabel(label, true);
}

math_function_ref
math_ast_function_call::lookup_fname(string const &function_name) {
	if (auto it = user_functions.find(function_name);
		it != user_functions.end()) {
		return it->second;
	} else if (auto it = builtin_functions.find(function_name);
		it != builtin_functions.end()) {
		return it->second;
	} else {
		throw_err_block(2, err_function_not_found, function_name.data());
	}
}
math_val math_ast_function_call::evaluate(const eval_context &ctx) const {
	std::vector<math_val> arg_vals;
	for (auto const &p : m_arguments) {
		arg_vals.push_back(p->evaluate(ctx));
	}
	return m_func.call(arg_vals);
}

int math_ast_function_call::has_label(const static_context& ctx) const {
	int out = m_func.has_label(ctx);
	for (auto const &p : m_arguments) {
		out |= p->has_label(ctx);
	}
	return out;
}

int math_ast_function_call::get_len(bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
	return m_func.get_len(m_arguments, could_be_bank_ex, ctx);
}

math_val math_user_function::call(const std::vector<math_val> &args) const {
	math_ast_node::eval_context new_ctx;
	new_ctx.userfunc_params = args;
	if (args.size() != m_arg_count)
		throw_err_block(2, err_argument_count, (int)m_arg_count, (int)args.size());
	return m_func_body->evaluate(new_ctx);
}

int math_user_function::has_label(const math_ast_node::static_context& ctx) const {
	if(ctx.current_user_func == this) return 0;
	math_ast_node::static_context new_ctx = ctx;
	new_ctx.current_user_func = this;
	return m_func_body->has_label(new_ctx);
}

int math_user_function::get_len(const std::vector<owned_node> &args,
								bool could_be_bank_ex, const math_ast_node::static_context& ctx) const {
	if(ctx.current_user_func == this) return 0;
	math_ast_node::static_context new_ctx = ctx;
	new_ctx.current_user_func = this;

	int len = m_func_body->get_len(false, new_ctx);
	for (auto &arg : args) {
		len = std::max(len, arg->get_len(false, ctx));
	}
	return len;
}

bool math_ast_node::is_label_offset(snes_label& out_base, int64_t& out_offset) {
	// callers of this function are all places where forward labels are banned anyways
	if(has_label() > 3) return false;
	if(auto me = dynamic_cast<math_ast_label*>(this)) {
		auto val = me->evaluate({}).get_identifier();
		out_base = labels.find(val);
		out_offset = 0;
		return true;
	}
	auto me = dynamic_cast<math_ast_binop*>(this);
	if(!me) return false;
	if(me->m_type != math_binop_type::add && me->m_type != math_binop_type::sub) return false;

	auto lhs = dynamic_cast<math_ast_label*>(me->m_left.get());
	if(!lhs) return false;
	auto val = lhs->evaluate({}).get_identifier();
	out_base = labels.find(val);

	if(me->m_right->has_label() > 1) return false;
	out_offset = me->m_right->evaluate_static().get_integer();
	if(me->m_type == math_binop_type::sub) out_offset = -out_offset;
	return true;
}
