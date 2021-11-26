#include "errors.h"

#include "asar.h"
#include <assert.h>
#include <stdarg.h>

#include "interface-shared.h"

static int asar_num_errors = 0;

struct asar_error_mapping
{
	asar_error_id errid;
	const char* message;

	asar_error_mapping(asar_error_id inerrid, const char* inmessage)
	{
		++asar_num_errors;

		errid = inerrid;
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
static asar_error_mapping asar_errors[] =
{
	{ error_id_limit_reached, "Over %d errors detected. Aborting." },
	{ error_id_werror, "One or more warnings was detected with werror on." },

	{ error_id_16mb_rom_limit, "Can't create ROMs larger than 16MB." },
	{ error_id_buffer_too_small, "The given buffer is too small to contain the resulting ROM." },
	{ error_id_params_null, "params passed to asar_patch_ex() is null." },
	{ error_id_params_invalid_size, "Size of params passed to asar_patch_ex() is invalid." },
	{ error_id_cmdl_define_invalid, "Invalid define name in %s: '%s'." },
	{ error_id_cmdl_define_override, "%s '%s' overrides a previous define. Did you specify the same define twice?" },
	{ error_id_create_rom_failed, "Couldn't create ROM." },
	{ error_id_open_rom_failed, "Couldn't open ROM." },
	{ error_id_open_rom_not_smw_extension, "Doesn't look like an SMW ROM. (Maybe its extension is wrong?)" },
	{ error_id_open_rom_not_smw_header, "Doesn't look like an SMW ROM. (Maybe it's headered?)" },
	{ error_id_stddefines_no_identifier, "stddefines.txt contains a line with a value, but no identifier." },
	{ error_id_stddefine_after_closing_quote, "Broken std defines. (Something after closing quote)" },

	{ error_id_failed_to_open_file, "Failed to open file '%s'." },
	{ error_id_file_not_found, "File '%s' wasn't found." },
	{ error_id_readfile_1_to_4_bytes, "Can only read chunks of 1 to 4 bytes." },
	{ error_id_canreadfile_0_bytes, "Number of bytes to check must be > 0." },
	{ error_id_file_offset_out_of_bounds, "File offset %s out of bounds for file '%s'." },
	{ error_id_rep_at_file_end, "rep at the end of a file" },

	{ error_id_mismatched_parentheses, "Mismatched parentheses." },
	{ error_id_invalid_hex_value, "Invalid hex value." },
	{ error_id_invalid_binary_value, "Invalid binary value." },
	{ error_id_invalid_character, "Invalid character." },
	{ error_id_string_literal_not_terminated, "String literal not terminated." },
	{ error_id_malformed_function_call, "Malformed function call." },
	{ error_id_invalid_number, "Invalid number." },
	{ error_id_garbage_near_quoted_string, "Garbage near quoted string." },
	{ error_id_mismatched_quotes, "Mismatched quotes." },

	{ error_id_rom_too_short, "ROM is too short to have a title. (Expected '%s')" },
	{ error_id_rom_title_incorrect, "ROM title is incorrect. Expected '%s', got '%s'." },

	{ error_id_bank_border_crossed, "A bank border was crossed, SNES address $%06X." },

	{ error_id_start_of_file, "This command may only be used at the start of a file." },
	{ error_id_xkas_asar_conflict, "Using @xkas and @asar in the same patch is not supported." },
	{ error_id_invalid_version_number, "Invalid version number." },
	{ error_id_asar_too_old, "This version of Asar is too old for this patch." },

	{ error_id_relative_branch_out_of_bounds, "Relative branch out of bounds. (Distance is %s)." },
	{ error_id_snes_address_doesnt_map_to_rom, "SNES address %s doesn't map to ROM." },
	{ error_id_snes_address_out_of_bounds, "SNES address %s out of bounds." },
	{ error_id_invalid_tcall, "Invalid tcall." },
	{ error_id_use_xplus, "Use (x+) instead." },
	{ error_id_opcode_length_too_long, "Opcode length is too long." },
	{ error_id_superfx_invalid_short_address, "Invalid short address parameter: $%s. (Must be even number and $0000-$01FE)" },
	{ error_id_superfx_invalid_register, "Invalid register for opcode; valid range is %d-%d." },
	{ error_id_invalid_opcode_length, "Invalid opcode length specification." },
	{ error_id_invalid_mapper, "Invalid mapper." },

	{ error_id_nan, "NaN encountered." },
	{ error_id_division_by_zero, "Division by zero." },
	{ error_id_modulo_by_zero, "Modulo by zero." },
	{ error_id_unknown_operator, "Unknown operator." },
	{ error_id_invalid_input, "Invalid input." },

	{ error_id_invalid_function_name, "Invalid function name." },
	{ error_id_function_not_found, "Function '%s' wasn't found." },
	{ error_id_function_redefined, "Function '%s' redefined." },
	{ error_id_broken_function_declaration, "Broken function declaration." },
	{ error_id_wrong_num_parameters, "Wrong number of parameters to function." },
	{ error_id_invalid_param_name, "Invalid parameter name." },
	{ error_id_math_invalid_type, "Wrong type for parameter %s, expected %s." },

	{ error_id_invalid_label_name, "Invalid label name." },
	{ error_id_label_not_found, "Label '%s' wasn't found." },
	{ error_id_label_redefined, "Label '%s' redefined." },
	{ error_id_broken_label_definition, "Broken label definition." },
	{ error_id_label_cross_assignment, "Setting labels to other non-static labels is not valid." },
	{ error_id_macro_label_outside_of_macro, "Macro label outside of a macro." },
	{ error_id_label_on_third_pass, "Internal error: A label was created on the third pass. Please create an issue on the official GitHub repository and attach a patch which reproduces the error." },
	{ error_id_label_moving, "Internal error: A label is moving around. Please create an issue on the official GitHub repository and attach a patch which reproduces the error." },

	{ error_id_invalid_namespace_name, "Invalid namespace name." },
	{ error_id_invalid_namespace_use, "Invalid use of namespace command." },

	{ error_id_invalid_struct_name, "Invalid struct name." },
	{ error_id_struct_not_found, "Struct '%s' wasn't found." },
	{ error_id_struct_redefined, "Struct '%s' redefined." },
	{ error_id_struct_invalid_parent_name, "Invalid parent name." },
	{ error_id_invalid_label_missing_closer, "Invalid label name, missing array closer." },
	{ error_id_multiple_subscript_operators, "Multiple subscript operators is invalid." },
	{ error_id_invalid_subscript, "Invalid array subscript after first scope resolution." },
	{ error_id_label_missing_parent, "This label has no parent." },
	{ error_id_array_invalid_inside_structs, "Array syntax invalid inside structs." },
	{ error_id_struct_without_endstruct, "struct without matching endstruct." },
	{ error_id_nested_struct, "Can not nest structs." },
	{ error_id_missing_struct_params, "Missing struct parameters." },
	{ error_id_too_many_struct_params, "Too many struct parameters." },
	{ error_id_missing_extends, "Missing extends keyword." },
	{ error_id_invalid_endstruct_count, "Invalid endstruct parameter count." },
	{ error_id_expected_align, "Expected align parameter." },
	{ error_id_endstruct_without_struct, "endstruct can only be used in combination with struct." },
	{ error_id_alignment_too_small, "Alignment must be >= 1." },

	{ error_id_invalid_define_name, "Invalid define name." },
	{ error_id_define_not_found, "Define '%s' wasn't found." },
	{ error_id_broken_define_declaration, "Broken define declaration." },
	{ error_id_overriding_builtin_define, "Trying to set define '%s', which is the name of a built-in define and thus can't be modified." },
	{ error_id_define_label_math, "!Define #= Label is not allowed with non-static labels." },
	{ error_id_mismatched_braces, "Mismatched braces." },

	{ error_id_invalid_macro_name, "Invalid macro name." },
	{ error_id_macro_not_found, "Macro '%s' wasn't found." },
	{ error_id_macro_redefined, "Macro '%s' redefined." },
	{ error_id_broken_macro_declaration, "Broken macro declaration." },
	{ error_id_invalid_macro_param_name, "Invalid macro parameter name." },
	{ error_id_macro_param_not_found, "Macro parameter '%s' wasn't found." },
	{ error_id_macro_param_redefined, "Macro parameter '%s' redefined" },
	{ error_id_broken_macro_usage, "Broken macro usage." },
	{ error_id_macro_wrong_num_params, "Wrong number of parameters to macro." },
	{ error_id_broken_macro_contents, "Broken macro contents." },
	{ error_id_rep_at_macro_end, "rep or if at the end of a macro." },
	{ error_id_nested_macro_definition, "Nested macro definition." },
	{ error_id_misplaced_endmacro, "Misplaced endmacro." },
	{ error_id_unclosed_macro, "Unclosed macro." },

	{ error_id_label_in_conditional, "Non-static label in %s command." },
	{ error_id_broken_conditional, "Broken %s command." },
	{ error_id_invalid_condition, "Invalid condition." },
	{ error_id_misplaced_elseif, "Misplaced elseif." },
	{ error_id_elseif_in_while, "Can't use elseif in a while loop." },
	{ error_id_elseif_in_singleline, "Can't use elseif on single-line statements." },
	{ error_id_misplaced_endif, "Misplaced endif." },
	{ error_id_misplaced_else, "Misplaced else." },
	{ error_id_else_in_while_loop, "Can't use else in a while loop." },
	{ error_id_unclosed_if, "Unclosed if statement." },

	{ error_id_unknown_command, "Unknown command." },
	{ error_id_command_disabled, "This command is disabled." },

	{ error_id_broken_incbin, "Broken incbin command." },
	{ error_id_incbin_64kb_limit, "Can't include more than 64 kilobytes at once." },
	{ error_id_recursion_limit, "Recursion limit reached." },
	{ error_id_command_in_non_root_file, "This command may only be used in the root file." },
	{ error_id_cant_be_main_file, "This file may not be used as the main file.%s" },
	{ error_id_no_labels_here, "Can't use non-static labels here." },

	{ error_id_invalid_freespace_request, "Invalid freespace request." },
	{ error_id_no_banks_with_ram_mirrors, "No banks contain the RAM mirrors in hirom or exhirom." },
	{ error_id_no_freespace_norom, "Can't find freespace in norom." },
	{ error_id_static_freespace_autoclean, "A static freespace must be targeted by at least one autoclean." },
	{ error_id_static_freespace_growing, "A static freespace may not grow." },
	{ error_id_no_freespace_in_mapped_banks, "No freespace found in the mapped banks. (Requested size: %s)" },
	{ error_id_no_freespace, "No freespace found. (Requested size: %s)" },
	{ error_id_freespace_limit_reached, "A patch may not contain more than %d freespaces." },

	{ error_id_prot_not_at_freespace_start, "PROT must be used at the start of a freespace block." },
	{ error_id_prot_too_many_entries, "Too many entries to PROT." },

	{ error_id_autoclean_in_freespace, "autoclean used in freespace." },
	{ error_id_autoclean_label_at_freespace_end, "Don't autoclean a label at the end of a freespace block, you'll remove some stuff you're not supposed to remove." },
	{ error_id_broken_autoclean, "Broken autoclean command." },

	{ error_id_pulltable_without_table, "Using pulltable when there is no table on the stack yet." },
	{ error_id_invalid_table_file, "Invalid table file." },

	{ error_id_pad_in_freespace, "pad does not make sense in a freespaced code." },

	{ error_id_org_label_invalid, "org Label is not valid." },
	{ error_id_org_label_forward, "org Label is only valid for labels earlier in the patch." },

	{ error_id_skip_label_invalid, "skip Label is not valid." },

	{ error_id_spc700_inline_no_base, "base is not implemented for architecture spc700-inline." },
	{ error_id_base_label_invalid, "base Label is not valid." },

	{ error_id_rep_label, "rep Label is not valid." },

	{ error_id_pushpc_without_pullpc, "pushpc without matching pullpc." },
	{ error_id_pullpc_without_pushpc, "pullpc without matching pushpc." },
	{ error_id_pullpc_different_arch, "pullpc in another architecture than the pushpc." },
	{ error_id_pullbase_without_pushbase, "pullbase without matching pushbase." },

	{ error_id_invalid_math, "Invalid math command." },
	{ error_id_invalid_warn, "Invalid warn command." },
	{ error_id_invalid_check, "Invalid check command." },

	{ error_id_warnpc_in_freespace, "warnpc used in freespace." },
	{ error_id_warnpc_broken_param, "Broken warnpc parameter." },
	{ error_id_warnpc_failed, "warnpc failed: Current position (%s) is after end position (%s)." },
	{ error_id_warnpc_failed_equal, "warnpc failed: Current position (%s) is equal to end position (%s)." },

	{ error_id_assertion_failed, "Assertion failed%s" },

	{ error_id_error_command, "error command%s" },

	{ error_id_invalid_print_function_syntax, "Invalid printable string syntax." },
	{ error_id_unknown_variable, "Unknown variable." },

	{ error_id_invalid_warning_id, "Invalid warning ID passed to %s. Expected format is WXXXX where %d <= XXXX <= %d." },

	{ error_id_pushwarnings_without_pullwarnings, "warnings push without matching warnings pull." },
	{ error_id_pullwarnings_without_pushwarnings, "warnings pull without matching warnings push." },

	{ error_id_failed_to_open_file_access_denied, "Failed to open file '%s'. Access denied." },
	{ error_id_failed_to_open_not_regular_file, "Failed to open file '%s'. Not a regular file (did you try to use a directory?)" },

	{ error_id_bad_dp_base, "The dp base should be page aligned (i.e. a multiple of 256)"},
	{ error_id_bad_dp_optimize, "Bad dp optimize value %s, expected: [none, ram, always]"},
	{ error_id_bad_address_optimize, "Bad dp optimize value %s, expected: [default, ram, mirrors]"},
	{ error_id_bad_optimize, "Bad optimize value %s, expected: [dp, address]"},

	{ error_id_require_parameter, "Missing required function parameter"},
	{ error_id_expected_parameter, "Not enough parameters in calling of function %s"},
	{ error_id_unexpected_parameter, "Too many parameters in calling of function %s"},
	{ error_id_duplicate_param_name, "Duplicated parameter name: %s in creation of function %s"},

	{ error_id_invalid_alignment, "Invalid alignment. Expected a power of 2." },
	{ error_id_alignment_too_big, "Requested alignment too large." },

	{ error_id_negative_shift, "Bitshift amount is negative." },

	{ error_id_macro_not_varadic, "Invalid use of sizeof(...), active macro is not variadic." },
	{ error_id_vararg_sizeof_nomacro, "Invalid use of sizeof(...), no active macro." },
	{ error_id_macro_wrong_min_params, "Variadic macro call with too few parameters" },
	{ error_id_vararg_out_of_bounds, "Variadic macro parameter requested is out of bounds." },
	{ error_id_vararg_must_be_last, "Variadic macro parameter must be the last parameter." },
	{ error_id_invalid_global_label, "Global label definition contains an invalid label [%s]."},

	{ error_id_spc700_addr_out_of_range, "Address %s out of range for instruction, valid range is 0000-1FFF" },
	{ error_id_label_ambiguous, "Label (%s) location is ambiguous due to straddling optimization border." },

	{ error_id_feature_unavaliable_in_spcblock, "This feature may not be used while an spcblock is active" },
	{ error_id_endspcblock_without_spcblock, "Use of endspcblock without matching spcblock" },
	{ error_id_missing_endspcblock, "Use of endspcblock without matching spcblock" },
	{ error_id_spcblock_bad_arch, "spcblock only valid inside spc700 arch" },
	{ error_id_spcblock_inside_struct, "Can not start an spcblock while a struct is still open" },
	{ error_id_spcblock_too_few_args, "Too few args passed to spcblock" },
	{ error_id_spcblock_too_many_args, "Too many args passed to spcblock" },
	{ error_id_unknown_spcblock_type, "Unknown spcblock format" },
	{ error_id_custom_spcblock_missing_macro, "Custom spcblock types must refer to a valid macro" },
	{ error_id_spcblock_macro_doesnt_exist, "Macro specified to custom spcblock was not found"},
	{ error_id_extra_spcblock_arg_for_type, "Only custom spcblock type takes a fourth argument" },
	{ error_id_spcblock_macro_must_be_varadic, "Custom spcblock macros must be variadic" },
	{ error_id_spcblock_macro_invalid_static_args, "Custom spcblock macros must have three static arguments" },
	{ error_id_spcblock_custom_types_incomplete, "Custom spcblock types are not yet supported. One day." },
	{ error_id_startpos_without_spcblock, "The startpos command is only valid in spcblocks" },
	{ error_id_internal_error, "An internal asar error occured (%s). Send help." },

	{ error_id_pushns_without_pullns, "pushns without matching pullns." },
	{ error_id_pullns_without_pushns, "pullns without matching pushns." },

	{ error_id_label_forward, "The use of forward labels is not allowed in this context" },
	{ error_id_undefined_char, "'%s' is not defined in the character table" },

	{ error_id_invalid_utf8, "Invalid text encoding detected. Asar expects UTF-8-encoded text. Please re-save this file in a text editor of choice using UTF-8 encoding." },
	{ error_id_cmdl_utf16_to_utf8_failed, "UTF-16 to UTF-8 string conversion failed: %s." },
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
