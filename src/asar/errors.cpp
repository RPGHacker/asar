#include "errors.h"

#include "asar.h"
#include <assert.h>
#include <stdarg.h>

#include "interface-shared.h"

static int asar_num_errors = 0;

struct asar_error_mapping
{
	asar_error_id errid;
	const char* name;
	const char* message;

	asar_error_mapping(asar_error_id inerrid, const char* iname, const char* inmessage)
	{
		++asar_num_errors;

		errid = inerrid;
		name = iname;
		message = inmessage;

		// RPG Hacker: Sanity check. This makes sure that the order
		// of entries in asar_errors matches with the order of
		// constants in asar_error_id. This is important because
		// we access asar_errors as an array.
		// Would love to do this via static_assert(), but can't
		// think of a way to do so.
		assert(errid - error_id_start == asar_num_errors);
	}
};

// Keep in sync with asar_error_id.
// Both, enum mapping and order, must match.
#define ERR(name) error_id_ ## name, "E" #name
static asar_error_mapping asar_errors[] =
{
	{ ERR(limit_reached), "Over %d errors detected. Aborting." },
	{ ERR(werror), "One or more warnings was detected with werror on." },

	{ ERR(16mb_rom_limit), "Can't create ROMs larger than 16MB." },
	{ ERR(buffer_too_small), "The given buffer is too small to contain the resulting ROM." },
	{ ERR(params_null), "params passed to asar_patch_ex() is null." },
	{ ERR(params_invalid_size), "Size of params passed to asar_patch_ex() is invalid." },
	{ ERR(cmdl_define_invalid), "Invalid define name in %s: '%s'." },
	{ ERR(cmdl_define_override), "%s '%s' overrides a previous define. Did you specify the same define twice?" },
	{ ERR(create_rom_failed), "Couldn't create ROM." },
	{ ERR(open_rom_failed), "Couldn't open ROM." },
	{ ERR(open_rom_not_smw_extension), "Doesn't look like an SMW ROM. (Maybe its extension is wrong?)" },
	{ ERR(open_rom_not_smw_header), "Doesn't look like an SMW ROM. (Maybe it's headered?)" },
	{ ERR(stddefines_no_identifier), "stddefines.txt contains a line with a value, but no identifier." },
	{ ERR(stddefine_after_closing_quote), "Broken std defines. (Something after closing quote)" },

	{ ERR(failed_to_open_file), "Failed to open file '%s'." },
	{ ERR(file_not_found), "File '%s' wasn't found." },
	{ ERR(readfile_1_to_4_bytes), "Can only read chunks of 1 to 4 bytes." },
	{ ERR(canreadfile_0_bytes), "Number of bytes to check must be > 0." },
	{ ERR(file_offset_out_of_bounds), "File offset %s out of bounds for file '%s'." },
	{ ERR(rep_at_file_end), "rep at the end of a file" },

	{ ERR(mismatched_parentheses), "Mismatched parentheses." },
	{ ERR(invalid_hex_value), "Invalid hex value." },
	{ ERR(invalid_binary_value), "Invalid binary value." },
	{ ERR(invalid_character), "Invalid character." },
	{ ERR(string_literal_not_terminated), "String literal not terminated." },
	{ ERR(malformed_function_call), "Malformed function call." },
	{ ERR(invalid_number), "Invalid number." },
	{ ERR(garbage_near_quoted_string), "Garbage near quoted string." },
	{ ERR(mismatched_quotes), "Mismatched quotes." },

	{ ERR(rom_too_short), "ROM is too short to have a title. (Expected '%s')" },
	{ ERR(rom_title_incorrect), "ROM title is incorrect. Expected '%s', got '%s'." },

	{ ERR(bank_border_crossed), "A bank border was crossed, SNES address $%06X." },

	{ ERR(start_of_file), "This command may only be used at the start of a file." },
	{ ERR(xkas_asar_conflict), "Using @xkas and @asar in the same patch is not supported." },
	{ ERR(invalid_version_number), "Invalid version number." },
	{ ERR(asar_too_old), "This version of Asar is too old for this patch." },

	{ ERR(relative_branch_out_of_bounds), "Relative branch out of bounds. (Distance is %s)." },
	{ ERR(snes_address_doesnt_map_to_rom), "SNES address %s doesn't map to ROM." },
	{ ERR(snes_address_out_of_bounds), "SNES address %s out of bounds." },
	{ ERR(invalid_tcall), "Invalid tcall." },
	{ ERR(use_xplus), "Use (x+) instead." },
	{ ERR(opcode_length_too_long), "Opcode length is too long." },
	{ ERR(superfx_invalid_short_address), "Invalid short address parameter: $%s. (Must be even number and $0000-$01FE)" },
	{ ERR(superfx_invalid_register), "Invalid register for opcode; valid range is %d-%d." },
	{ ERR(invalid_opcode_length), "Invalid opcode length specification." },
	{ ERR(invalid_mapper), "Invalid mapper." },

	{ ERR(nan), "NaN encountered." },
	{ ERR(division_by_zero), "Division by zero." },
	{ ERR(modulo_by_zero), "Modulo by zero." },
	{ ERR(unknown_operator), "Unknown operator." },
	{ ERR(invalid_input), "Invalid input." },

	{ ERR(invalid_function_name), "Invalid function name." },
	{ ERR(function_not_found), "Function '%s' wasn't found." },
	{ ERR(function_redefined), "Function '%s' redefined." },
	{ ERR(broken_function_declaration), "Broken function declaration." },
	{ ERR(wrong_num_parameters), "Wrong number of parameters to function." },
	{ ERR(invalid_param_name), "Invalid parameter name." },
	{ ERR(math_invalid_type), "Wrong type for parameter %s, expected %s." },

	{ ERR(invalid_label_name), "Invalid label name." },
	{ ERR(label_not_found), "Label '%s' wasn't found." },
	{ ERR(label_redefined), "Label '%s' redefined." },
	{ ERR(broken_label_definition), "Broken label definition." },
	{ ERR(label_cross_assignment), "Setting labels to other non-static labels is not valid." },
	{ ERR(macro_label_outside_of_macro), "Macro label outside of a macro." },
	{ ERR(label_on_third_pass), "Internal error: A label was created on the third pass. Please create an issue on the official GitHub repository and attach a patch which reproduces the error." },
	{ ERR(label_moving), "Internal error: A label is moving around. Please create an issue on the official GitHub repository and attach a patch which reproduces the error." },

	{ ERR(invalid_namespace_name), "Invalid namespace name." },
	{ ERR(invalid_namespace_use), "Invalid use of namespace command." },

	{ ERR(invalid_struct_name), "Invalid struct name." },
	{ ERR(struct_not_found), "Struct '%s' wasn't found." },
	{ ERR(struct_redefined), "Struct '%s' redefined." },
	{ ERR(struct_invalid_parent_name), "Invalid parent name." },
	{ ERR(invalid_label_missing_closer), "Invalid label name, missing array closer." },
	{ ERR(multiple_subscript_operators), "Multiple subscript operators is invalid." },
	{ ERR(invalid_subscript), "Invalid array subscript after first scope resolution." },
	{ ERR(label_missing_parent), "This label has no parent." },
	{ ERR(array_invalid_inside_structs), "Array syntax invalid inside structs." },
	{ ERR(struct_without_endstruct), "struct without matching endstruct." },
	{ ERR(nested_struct), "Can not nest structs." },
	{ ERR(missing_struct_params), "Missing struct parameters." },
	{ ERR(too_many_struct_params), "Too many struct parameters." },
	{ ERR(missing_extends), "Missing extends keyword." },
	{ ERR(invalid_endstruct_count), "Invalid endstruct parameter count." },
	{ ERR(expected_align), "Expected align parameter." },
	{ ERR(endstruct_without_struct), "endstruct can only be used in combination with struct." },
	{ ERR(alignment_too_small), "Alignment must be >= 1." },

	{ ERR(invalid_define_name), "Invalid define name." },
	{ ERR(define_not_found), "Define '%s' wasn't found." },
	{ ERR(broken_define_declaration), "Broken define declaration." },
	{ ERR(overriding_builtin_define), "Trying to set define '%s', which is the name of a built-in define and thus can't be modified." },
	{ ERR(define_label_math), "!Define #= Label is not allowed with non-static labels." },
	{ ERR(mismatched_braces), "Mismatched braces." },

	{ ERR(invalid_macro_name), "Invalid macro name." },
	{ ERR(macro_not_found), "Macro '%s' wasn't found." },
	{ ERR(macro_redefined), "Macro '%s' redefined." },
	{ ERR(broken_macro_declaration), "Broken macro declaration." },
	{ ERR(invalid_macro_param_name), "Invalid macro parameter name." },
	{ ERR(macro_param_not_found), "Macro parameter '%s' wasn't found." },
	{ ERR(macro_param_redefined), "Macro parameter '%s' redefined" },
	{ ERR(broken_macro_usage), "Broken macro usage." },
	{ ERR(macro_wrong_num_params), "Wrong number of parameters to macro." },
	{ ERR(broken_macro_contents), "Broken macro contents." },
	{ ERR(rep_at_macro_end), "rep or if at the end of a macro." },
	{ ERR(nested_macro_definition), "Nested macro definition." },
	{ ERR(misplaced_endmacro), "Misplaced endmacro." },
	{ ERR(unclosed_macro), "Unclosed macro." },

	{ ERR(label_in_conditional), "Non-static label in %s command." },
	{ ERR(broken_conditional), "Broken %s command." },
	{ ERR(invalid_condition), "Invalid condition." },
	{ ERR(misplaced_elseif), "Misplaced elseif." },
	{ ERR(elseif_in_while), "Can't use elseif in a while loop." },
	{ ERR(elseif_in_singleline), "Can't use elseif on single-line statements." },
	{ ERR(misplaced_endif), "Misplaced endif." },
	{ ERR(misplaced_else), "Misplaced else." },
	{ ERR(else_in_while_loop), "Can't use else in a while loop." },
	{ ERR(unclosed_if), "Unclosed if statement." },

	{ ERR(unknown_command), "Unknown command." },
	{ ERR(command_disabled), "This command is disabled." },

	{ ERR(broken_incbin), "Broken incbin command." },
	{ ERR(incbin_64kb_limit), "Can't include more than 64 kilobytes at once." },
	{ ERR(recursion_limit), "Recursion limit reached." },
	{ ERR(command_in_non_root_file), "This command may only be used in the root file." },
	{ ERR(cant_be_main_file), "This file may not be used as the main file.%s" },
	{ ERR(no_labels_here), "Can't use non-static labels here." },

	{ ERR(invalid_freespace_request), "Invalid freespace request." },
	{ ERR(no_banks_with_ram_mirrors), "No banks contain the RAM mirrors in hirom or exhirom." },
	{ ERR(no_freespace_norom), "Can't find freespace in norom." },
	{ ERR(static_freespace_autoclean), "A static freespace must be targeted by at least one autoclean." },
	{ ERR(static_freespace_growing), "A static freespace may not grow." },
	{ ERR(no_freespace_in_mapped_banks), "No freespace found in the mapped banks. (Requested size: %s)" },
	{ ERR(no_freespace), "No freespace found. (Requested size: %s)" },
	{ ERR(freespace_limit_reached), "A patch may not contain more than %d freespaces." },

	{ ERR(prot_not_at_freespace_start), "PROT must be used at the start of a freespace block." },
	{ ERR(prot_too_many_entries), "Too many entries to PROT." },

	{ ERR(autoclean_in_freespace), "autoclean used in freespace." },
	{ ERR(autoclean_label_at_freespace_end), "Don't autoclean a label at the end of a freespace block, you'll remove some stuff you're not supposed to remove." },
	{ ERR(broken_autoclean), "Broken autoclean command." },

	{ ERR(pulltable_without_table), "Using pulltable when there is no table on the stack yet." },
	{ ERR(invalid_table_file), "Invalid table file. Invalid entry on line: %i" },

	{ ERR(pad_in_freespace), "pad does not make sense in a freespaced code." },

	{ ERR(org_label_invalid), "org Label is not valid." },
	{ ERR(org_label_forward), "org Label is only valid for labels earlier in the patch." },

	{ ERR(skip_label_invalid), "skip Label is not valid." },

	{ ERR(spc700_inline_no_base), "base is not implemented for architecture spc700-inline." },
	{ ERR(base_label_invalid), "base Label is not valid." },

	{ ERR(rep_label), "rep Label is not valid." },

	{ ERR(pushpc_without_pullpc), "pushpc without matching pullpc." },
	{ ERR(pullpc_without_pushpc), "pullpc without matching pushpc." },
	{ ERR(pullpc_different_arch), "pullpc in another architecture than the pushpc." },
	{ ERR(pullbase_without_pushbase), "pullbase without matching pushbase." },

	{ ERR(invalid_math), "Invalid math command." },
	{ ERR(invalid_warn), "Invalid warn command." },
	{ ERR(invalid_check), "Invalid check command." },

	{ ERR(warnpc_in_freespace), "warnpc used in freespace." },
	{ ERR(warnpc_broken_param), "Broken warnpc parameter." },
	{ ERR(warnpc_failed), "warnpc failed: Current position (%s) is after end position (%s)." },
	{ ERR(warnpc_failed_equal), "warnpc failed: Current position (%s) is equal to end position (%s)." },

	{ ERR(assertion_failed), "Assertion failed%s" },

	{ ERR(error_command), "error command%s" },

	{ ERR(invalid_print_function_syntax), "Invalid printable string syntax." },
	{ ERR(unknown_variable), "Unknown variable." },

	{ ERR(invalid_warning_id), "Invalid warning ID passed to %s. Expected format is WXXXX where %d <= XXXX <= %d." },

	{ ERR(pushwarnings_without_pullwarnings), "warnings push without matching warnings pull." },
	{ ERR(pullwarnings_without_pushwarnings), "warnings pull without matching warnings push." },

	{ ERR(failed_to_open_file_access_denied), "Failed to open file '%s'. Access denied." },
	{ ERR(failed_to_open_not_regular_file), "Failed to open file '%s'. Not a regular file (did you try to use a directory?)" },

	{ ERR(bad_dp_base), "The dp base should be page aligned (i.e. a multiple of 256)"},
	{ ERR(bad_dp_optimize), "Bad dp optimize value %s, expected: [none, ram, always]"},
	{ ERR(bad_address_optimize), "Bad dp optimize value %s, expected: [default, ram, mirrors]"},
	{ ERR(bad_optimize), "Bad optimize value %s, expected: [dp, address]"},

	{ ERR(require_parameter), "Missing required function parameter"},
	{ ERR(expected_parameter), "Not enough parameters in calling of function %s"},
	{ ERR(unexpected_parameter), "Too many parameters in calling of function %s"},
	{ ERR(duplicate_param_name), "Duplicated parameter name: %s in creation of function %s"},

	{ ERR(invalid_alignment), "Invalid alignment. Expected a power of 2." },
	{ ERR(alignment_too_big), "Requested alignment too large." },

	{ ERR(negative_shift), "Bitshift amount is negative." },

	{ ERR(macro_not_varadic), "Invalid use of sizeof(...), active macro is not variadic." },
	{ ERR(vararg_sizeof_nomacro), "Invalid use of sizeof(...), no active macro." },
	{ ERR(macro_wrong_min_params), "Variadic macro call with too few parameters" },
	{ ERR(vararg_out_of_bounds), "Variadic macro parameter requested is out of bounds." },
	{ ERR(vararg_must_be_last), "Variadic macro parameter must be the last parameter." },
	{ ERR(invalid_global_label), "Global label definition contains an invalid label [%s]."},

	{ ERR(spc700_addr_out_of_range), "Address %s out of range for instruction, valid range is 0000-1FFF" },
	{ ERR(label_ambiguous), "Label (%s) location is ambiguous due to straddling optimization border." },

	{ ERR(feature_unavaliable_in_spcblock), "This feature may not be used while an spcblock is active" },
	{ ERR(endspcblock_without_spcblock), "Use of endspcblock without matching spcblock" },
	{ ERR(missing_endspcblock), "Use of endspcblock without matching spcblock" },
	{ ERR(spcblock_bad_arch), "spcblock only valid inside spc700 arch" },
	{ ERR(spcblock_inside_struct), "Can not start an spcblock while a struct is still open" },
	{ ERR(spcblock_too_few_args), "Too few args passed to spcblock" },
	{ ERR(spcblock_too_many_args), "Too many args passed to spcblock" },
	{ ERR(unknown_spcblock_type), "Unknown spcblock format" },
	{ ERR(custom_spcblock_missing_macro), "Custom spcblock types must refer to a valid macro" },
	{ ERR(spcblock_macro_doesnt_exist), "Macro specified to custom spcblock was not found"},
	{ ERR(extra_spcblock_arg_for_type), "Only custom spcblock type takes a fourth argument" },
	{ ERR(spcblock_macro_must_be_varadic), "Custom spcblock macros must be variadic" },
	{ ERR(spcblock_macro_invalid_static_args), "Custom spcblock macros must have three static arguments" },
	{ ERR(spcblock_custom_types_incomplete), "Custom spcblock types are not yet supported. One day." },
	{ ERR(startpos_without_spcblock), "The startpos command is only valid in spcblocks" },
	{ ERR(invalid_endspcblock_arg), "Invalid argument to endspcblock: \"%s\"" },
	{ ERR(unknown_endspcblock_format), "Unsupported endspcblock format. Currently supported formats are \"endspcblock\" and \"endspcblock execute [label]\"" },
	{ ERR(internal_error), "An internal asar error occured (%s). Send help." },

	{ ERR(pushns_without_pullns), "pushns without matching pullns." },
	{ ERR(pullns_without_pushns), "pullns without matching pushns." },

	{ ERR(label_forward), "The use of forward labels is not allowed in this context." },

	{ ERR(unclosed_vararg), "Variadic macro parameter wasn't closed properly." },
	{ ERR(invalid_vararg), "Trying to use variadic macro parameter syntax to resolve a non variadic argument." },

	{ ERR(macro_param_outside_macro), "Reference to macro parameter outside of macro" },

	{ ERR(broken_for_loop), "Broken for loop declaration." },
	{ ERR(bad_single_line_for), "Single-line for loop not allowed here." },

};
// RPG Hacker: Sanity check. This makes sure that the element count of asar_warnings
// matches with the number of constants in asar_warning_id. This is important, because
// we are going to access asar_warnings as an array.
static_assert(sizeof(asar_errors) / sizeof(asar_errors[0]) == error_id_count, "asar_errors and asar_error_id are not in sync");

template<typename t>
void asar_error_template(asar_error_id errid, int whichpass, const char* message)
{
	try
	{
		error_interface((int)errid, whichpass, message);
		t err;
		throw err;
	}
	catch (errnull&) {}
}

#if !defined(__clang__)
void(*shutupgcc1)(asar_error_id, int, const char*) = asar_error_template<errnull>;
void(*shutupgcc2)(asar_error_id, int, const char*) = asar_error_template<errblock>;
void(*shutupgcc3)(asar_error_id, int, const char*) = asar_error_template<errline>;
void(*shutupgcc4)(asar_error_id, int, const char*) = asar_error_template<errfatal>;
#endif

void asar_throw_error(int whichpass, asar_error_type type, asar_error_id errid, ...)
{
	assert(errid > error_id_start && errid < error_id_end);

	const asar_error_mapping& error = asar_errors[errid - error_id_start - 1];

	char error_buffer[1024];
	va_list args;
	va_start(args, errid);

#if defined(__clang__)
#	pragma clang diagnostic push
	// "format string is not a literal".
	// The pointer we're passing here should always point to a string literal,
	// thus, I think, we can safely ignore this here.
#	pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

	vsnprintf(error_buffer, sizeof(error_buffer), error.message, args);

#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

	va_end(args);

	switch (type)
	{
	case error_type_null:
		asar_error_template<errnull>(errid, whichpass, error_buffer);
		break;
	case error_type_block:
		asar_error_template<errblock>(errid, whichpass, error_buffer);
		break;
	case error_type_line:
		asar_error_template<errline>(errid, whichpass, error_buffer);
		break;
	case error_type_fatal:
	default:
		asar_error_template<errfatal>(errid, whichpass, error_buffer);
		break;
	}
}

const char* get_error_name(asar_error_id errid)
{
	assert(errid > error_id_start && errid < error_id_end);

	const asar_error_mapping& error = asar_errors[errid - error_id_start - 1];

	return error.name;
}
