#include "math_ast.h"
// TODO: make all these conversions print the current type aswell instead of just expected type
double math_val::get_double() const {
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
int64_t math_val::get_integer() const {
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
const string &math_val::get_str() const {
	if (m_type == math_val_type::string)
		return m_string_val;
	asar_throw_error(2, error_type_block, error_id_expected_string);
}

const string &math_val::get_identifier() const {
	if (m_type == math_val_type::identifier)
		return m_string_val;
	asar_throw_error(2, error_type_block, error_id_expected_ident);
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
}

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

math_val evaluate_binop(math_val lhs, math_val rhs,
						math_binop_type type) {
	// todo: do this a bit smarter (bit_ops shouldn't cast int->float->int)
	if (lhs.m_type == math_val_type::floating)
		rhs = math_val(rhs.get_double());
	else if (rhs.m_type == math_val_type::floating)
		lhs = math_val(lhs.get_double());
	switch (type) {
		case math_binop_type::pow:
			return math_val(pow(lhs.get_double(), rhs.get_double()));
		case math_binop_type::div:
			if (rhs.get_double() == 0.0)
				asar_throw_error(2, error_type_block, error_id_division_by_zero);
			return math_val(lhs.get_double() / rhs.get_double());
		case math_binop_type::mod:
			if (rhs.get_double() == 0.0)
				asar_throw_error(2, error_type_block, error_id_division_by_zero);
			// TODO: negative semantics
			if (lhs.m_type == math_val_type::floating) {
				return math_val(fmod(lhs.get_double(), rhs.get_double()));
			} else {
				return math_val(lhs.get_integer() % rhs.get_integer());
			}
			break;

		case math_binop_type::shift_left: {
			int64_t rhs_v = rhs.get_integer();
			if (rhs_v < 0)
				asar_throw_error(2, error_type_block, error_id_negative_shift);
			return math_val(lhs.get_integer() << (uint64_t)rhs_v);
		}
		case math_binop_type::shift_right: {
			int64_t rhs_v = rhs.get_integer();
			if (rhs_v < 0)
				asar_throw_error(2, error_type_block, error_id_negative_shift);
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
			asar_throw_error(2, error_type_block, error_id_internal_error,
					"evaluate_binop() on logical ops loses short-circuiting");

		case math_binop_type::mul:
		case math_binop_type::add:
		case math_binop_type::sub:
			// TODO error on string (also TODO support string +)
			if (lhs.m_type == math_val_type::floating)
				return math_val(evaluate_binop_arithmetic<double>(
					lhs.get_double(), rhs.get_double(), type));
			else
				return math_val(evaluate_binop_arithmetic<int64_t>(
					lhs.get_integer(), rhs.get_integer(), type));

		case math_binop_type::comp_ge:
		case math_binop_type::comp_le:
		case math_binop_type::comp_gt:
		case math_binop_type::comp_lt:
		case math_binop_type::comp_eq:
		case math_binop_type::comp_ne:
			if (lhs.m_type == math_val_type::floating)
				return math_val((int64_t)evaluate_binop_compare<double>(
					lhs.get_double(), rhs.get_double(), type));
			else
				return math_val((int64_t)evaluate_binop_compare<int64_t>(
					lhs.get_integer(), rhs.get_integer(), type));
	}
}

math_val math_ast_binop::evaluate(const math_eval_context &ctx) const {
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

int math_ast_binop::has_label() const {
	return m_left->has_label() | m_right->has_label();
}


int math_ast_binop::get_len(bool could_be_bank_ex) const {
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
math_val math_ast_unop::evaluate(const math_eval_context &ctx) const {
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
}

int math_ast_unop::has_label() const { return m_arg->has_label(); }

int math_ast_unop::get_len(bool could_be_bank_ex) const {
	if (could_be_bank_ex && m_type == math_unop_type::bank_extract)
		return 1;
	return m_arg->get_len(false);
}

math_val math_ast_label::evaluate(const math_eval_context &ctx) const {
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		return math_val::make_identifier(m_cur_ns + m_labelname);
	} else if (labels.exists(m_labelname)) {
		return math_val::make_identifier(m_labelname);
	} else {
		// i think in this context we always should throw???
		asar_throw_error(2, error_type_block, error_id_label_not_found,
				   m_labelname.data());
	}
}

int math_ast_label::has_label() const {
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		return labels.find(m_cur_ns + m_labelname).is_static ? 1 : 3;
	} else if (labels.exists(m_labelname)) {
		return labels.find(m_labelname).is_static ? 1 : 3;
	}
	// otherwise, non-static label
	return 3;
}

int math_ast_label::get_len(bool could_be_bank_ex) const {
	snes_label label;
	if (m_cur_ns && labels.exists(m_cur_ns + m_labelname)) {
		label = labels.find(m_cur_ns + m_labelname);
	} else if (labels.exists(m_labelname)) {
		label = labels.find(m_labelname);
	} else
	return 2;
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
		asar_throw_error(2, error_type_block, error_id_function_not_found,
				   function_name.data());
	}
}
math_val math_ast_function_call::evaluate(const math_eval_context &ctx) const {
	std::vector<math_val> arg_vals;
	for (auto const &p : m_arguments) {
		arg_vals.push_back(p->evaluate(ctx));
	}
	return m_func.call(arg_vals);
}

int math_ast_function_call::has_label() const {
	int out = m_func.has_label();
	for (auto const &p : m_arguments) {
		out |= p->has_label();
	}
	return out;
}

int math_ast_function_call::get_len(bool could_be_bank_ex) const {
	return m_func.get_len(m_arguments, could_be_bank_ex);
}

math_val math_user_function::call(const std::vector<math_val> &args) const {
	math_eval_context new_ctx;
	new_ctx.userfunc_params = args;
	if (args.size() != m_arg_count)
		asar_throw_error(2, error_type_block, error_id_argument_count, m_arg_count,
				   (int)args.size());
	return m_func_body->evaluate(new_ctx);
}

int math_user_function::has_label() const { return m_func_body->has_label(); }

int math_user_function::get_len(const std::vector<owned_node> &args,
								bool could_be_bank_ex) const {
	// TODO: this doesn't forward could_be_bank_ex to the fn call...
	// supporting that properly would require stringing some context through all
	// get_len calls supporting it less properly (making a special return value of
	// get_len signify bankextract) could be viable tho...
	int len = m_func_body->get_len(false);
	for (auto &arg : args) {
		len = std::max(len, arg->get_len(false));
	}
	return len;
}

