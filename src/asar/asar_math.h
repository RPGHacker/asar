#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "errors.h"
#include "libstr.h"

// Public interface of the Asar mathematical subsystem //

// Asar's math is dynamically typed, this enum defines the possible types.
enum class math_val_type {
	// 64-bit (double) floating point
	floating,
	// 64-bit signed int
	integer,
	// string
	string,
	// a resolved label with a name and value. used for datasize() and whatnot.
	// auto-converts to integer when necessary.
	identifier,
};

class math_val {
public:
	math_val_type m_type;
	union {
		double double_;
		int64_t int_;
	} m_numeric_val;
	// this is used for both labelname in case of
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
	static math_val make_identifier(string v) {
		math_val res(v);
		res.m_type = math_val_type::identifier;
		return res;
	}

	double get_double() const;
	int64_t get_integer() const;
	const string &get_str() const;
	const string &get_identifier() const;
	bool get_bool() const;
};

class math_ast_node {
public:
	// any info that's necessary during evaluation.
	// external callers shouldn't need to pass in anything other than the default-constructed one.
	class eval_context {
	public:
		// TODO would it be faster to make this a reference? does that avoid any significant copies?
		std::vector<math_val> userfunc_params;
	};

	virtual math_val evaluate(const eval_context& = {}) const = 0;
	// 0 - no label, 1 - static label, 3 - nonstatic label, 7 - forward label
	virtual int has_label() const = 0;
	// how many bytes long should the result of this expression be?
	virtual int get_len(bool could_be_bank_ex) const = 0;
	virtual ~math_ast_node() = default;

	// helper functions:

	// evaluate an expression that doesn't allow non-static label references
	math_val evaluate_static() {
		if(has_label() > 1)
			asar_throw_error(0, error_type_block, error_id_no_labels_here);
		return evaluate();
	}
	// evaluate an expression that doesn't allow forward label references
	math_val evaluate_non_forward() {
		if(has_label() > 3)
			asar_throw_error(0, error_type_block, error_id_label_forward);
		return evaluate();
	}
};

void initmathcore();
void deinitmathcore();
void closecachedfiles();

void createuserfunc(const char * name, const char * arguments, const char * content);

std::unique_ptr<math_ast_node> parse_math_expr(const char * str);

// old interface: to be nuked in due time

int64_t getnum(const char * str);

// still used in arch-spc700... god that one's a proper mess
int getlen(const char * str, bool optimizebankextraction=false);
extern bool foundlabel;
