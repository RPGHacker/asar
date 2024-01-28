#pragma once

#define ALL_ERRORS(ERR) \
	ERR(limit_reached, "Over %d errors detected. Aborting.") \
	ERR(werror, "One or more warnings was detected with werror on.") \
	ERR(buffer_too_small, "The given buffer is too small to contain the resulting ROM.") \
	ERR(params_null, "params passed to asar_patch_ex() is null.") \
	ERR(params_invalid_size, "Size of params passed to asar_patch_ex() is invalid.") \
	ERR(cmdl_define_invalid, "Invalid define name in %s: '%s'.") \
	ERR(cmdl_define_override, "%s '%s' overrides a previous define. Did you specify the same define twice?") \
	ERR(create_rom_failed, "Couldn't create ROM.") \
	ERR(open_rom_failed, "Couldn't open ROM.") \
	ERR(open_rom_not_smw_extension, "Doesn't look like an SMW ROM. (Maybe its extension is wrong?)") \
	ERR(open_rom_not_smw_header, "Doesn't look like an SMW ROM. (Maybe it's headered?)") \
	ERR(stddefines_no_identifier, "stddefines.txt contains a line with a value, but no identifier.") \
	ERR(stddefine_after_closing_quote, "Broken std defines. (Something after closing quote)") \
	ERR(failed_to_open_file, "Failed to open file '%s'.") \
	ERR(file_not_found, "File '%s' wasn't found.") \
	ERR(file_offset_out_of_bounds, "File offset %s out of bounds for file '%s'.") \
	ERR(mismatched_parentheses, "Mismatched parentheses.") \
	ERR(invalid_hex_value, "Invalid hex value.") \
	ERR(invalid_binary_value, "Invalid binary value.") \
	ERR(invalid_character, "Invalid character.") \
	ERR(string_literal_not_terminated, "String literal not terminated.") \
	ERR(malformed_function_call, "Malformed function call.") \
	ERR(invalid_number, "Invalid number.") \
	ERR(garbage_near_quoted_string, "Garbage near quoted string.") \
	ERR(mismatched_quotes, "Mismatched quotes.") \
	ERR(rom_too_short, "ROM is too short to have a title. (Expected '%s')") \
	ERR(rom_title_incorrect, "ROM title is incorrect. Expected '%s', got '%s'.") \
	ERR(bank_border_crossed, "A bank border was crossed, SNES address $%06X.") \
	ERR(start_of_file, "This command may only be used at the start of a file.") \
	ERR(invalid_version_number, "Invalid version number.") \
	ERR(asar_too_old, "This version of Asar is too old for this patch.") \
	ERR(relative_branch_out_of_bounds, "Relative branch out of bounds. (Distance is %s).") \
	ERR(snes_address_doesnt_map_to_rom, "SNES address %s doesn't map to ROM.") \
	ERR(snes_address_out_of_bounds, "SNES address %s out of bounds.") \
	ERR(invalid_tcall, "Invalid tcall.") \
	ERR(use_xplus, "Use (x+) instead.") \
	ERR(opcode_length_too_long, "Opcode length is too long.") \
	ERR(superfx_invalid_short_address, "Invalid short address parameter: $%s. (Must be even number and $0000-$01FE)") \
	ERR(superfx_invalid_register, "Invalid register for opcode; valid range is %d-%d.") \
	ERR(invalid_opcode_length, "Invalid opcode length specification.") \
	ERR(invalid_mapper, "Invalid mapper.") \
	ERR(nan, "NaN encountered.") \
	ERR(division_by_zero, "Division by zero.") \
	ERR(modulo_by_zero, "Modulo by zero.") \
	ERR(unknown_operator, "Unknown operator.") \
	ERR(invalid_input, "Invalid input.") \
	ERR(invalid_function_name, "Invalid function name.") \
	ERR(function_not_found, "Function '%s' wasn't found.") \
	ERR(function_redefined, "Function '%s' redefined.") \
	ERR(broken_function_declaration, "Broken function declaration.") \
	ERR(wrong_num_parameters, "Wrong number of parameters to function.") \
	ERR(invalid_param_name, "Invalid parameter name.") \
	ERR(invalid_label_name, "Invalid label name.") \
	ERR(label_not_found, "Label '%s' wasn't found.") \
	ERR(label_redefined, "Label '%s' redefined.") \
	ERR(broken_label_definition, "Broken label definition.") \
	ERR(label_cross_assignment, "Setting labels to other non-static labels is not valid.") \
	ERR(macro_label_outside_of_macro, "Macro label outside of a macro.") \
	ERR(invalid_namespace_name, "Invalid namespace name.") \
	ERR(invalid_namespace_use, "Invalid use of namespace command.") \
	ERR(invalid_struct_name, "Invalid struct name.") \
	ERR(struct_not_found, "Struct '%s' wasn't found.") \
	ERR(struct_redefined, "Struct '%s' redefined.") \
	ERR(struct_invalid_parent_name, "Invalid parent name.") \
	ERR(invalid_label_missing_closer, "Invalid label name, missing array closer.") \
	ERR(invalid_subscript, "Invalid array subscript after first scope resolution.") \
	ERR(label_missing_parent, "This label has no parent.") \
	ERR(struct_without_endstruct, "struct without matching endstruct.") \
	ERR(nested_struct, "Can not nest structs.") \
	ERR(missing_struct_params, "Missing struct parameters.") \
	ERR(too_many_struct_params, "Too many struct parameters.") \
	ERR(missing_extends, "Missing extends keyword.") \
	ERR(invalid_endstruct_count, "Invalid endstruct parameter count.") \
	ERR(expected_align, "Expected align parameter.") \
	ERR(endstruct_without_struct, "endstruct can only be used in combination with struct.") \
	ERR(alignment_too_small, "Alignment must be >= 1.") \
	ERR(invalid_define_name, "Invalid define name.") \
	ERR(define_not_found, "Define '%s' wasn't found.") \
	ERR(broken_define_declaration, "Broken define declaration.") \
	ERR(overriding_builtin_define, "Trying to set define '%s', which is the name of a built-in define and thus can't be modified.") \
	ERR(define_label_math, "!Define #= Label is not allowed with non-static labels.") \
	ERR(mismatched_braces, "Mismatched braces.") \
	ERR(invalid_macro_name, "Invalid macro name.") \
	ERR(macro_not_found, "Macro '%s' wasn't found.") \
	ERR(macro_redefined, "Macro '%s' redefined. First defined at: %s:%d") \
	ERR(broken_macro_declaration, "Broken macro declaration.") \
	ERR(invalid_macro_param_name, "Invalid macro parameter name.") \
	ERR(macro_param_not_found, "Macro parameter '%s' wasn't found.%s") \
	ERR(macro_param_redefined, "Macro parameter '%s' redefined") \
	ERR(broken_macro_usage, "Broken macro usage.") \
	ERR(macro_wrong_num_params, "Wrong number of parameters to macro.") \
	ERR(misplaced_endmacro, "Misplaced endmacro.") \
	ERR(unclosed_macro, "Unclosed macro: '%s'.") \
	ERR(label_in_conditional, "Non-static label in %s command.") \
	ERR(misplaced_elseif, "Misplaced elseif.") \
	ERR(elseif_in_while, "Can't use elseif in a while loop.") \
	ERR(misplaced_endif, "Misplaced endif.") \
	ERR(misplaced_else, "Misplaced else.") \
	ERR(else_in_while_loop, "Can't use else in a while loop.") \
	ERR(unclosed_if, "Unclosed if statement.") \
	ERR(unknown_command, "Unknown command.") \
	ERR(broken_incbin, "Broken incbin command.") \
	ERR(recursion_limit, "Recursion limit reached.") \
	ERR(cant_be_main_file, "This file may not be used as the main file.%s") \
	ERR(no_labels_here, "Can't use non-static labels here.") \
	ERR(invalid_freespace_request, "Invalid freespace request.") \
	ERR(static_freespace_autoclean, "A static freespace must be targeted by at least one autoclean.") \
	ERR(static_freespace_growing, "A static freespace may not grow.") \
	ERR(no_freespace_in_mapped_banks, "No freespace found in the mapped banks. (Requested size: %s)") \
	ERR(no_freespace, "No freespace found. (Requested size: %s)") \
	ERR(prot_not_at_freespace_start, "PROT must be used at the start of a freespace block.") \
	ERR(prot_too_many_entries, "Too many entries to PROT.") \
	ERR(autoclean_in_freespace, "autoclean used in freespace.") \
	ERR(autoclean_label_at_freespace_end, "Don't autoclean a label at the end of a freespace block, you'll remove some stuff you're not supposed to remove.") \
	ERR(broken_autoclean, "Broken autoclean command.") \
	ERR(pulltable_without_table, "Using pulltable when there is no table on the stack yet.") \
	ERR(pad_in_freespace, "pad does not make sense in a freespaced code.") \
	ERR(org_label_forward, "org Label is only valid for labels earlier in the patch.") \
	ERR(base_label_invalid, "base Label is not valid.") \
	ERR(pushpc_without_pullpc, "pushpc without matching pullpc.") \
	ERR(pullpc_without_pushpc, "pullpc without matching pushpc.") \
	ERR(pullpc_different_arch, "pullpc in another architecture than the pushpc.") \
	ERR(pullbase_without_pushbase, "pullbase without matching pushbase.") \
	ERR(invalid_check, "Invalid check command.") \
	ERR(assertion_failed, "Assertion failed%s") \
	ERR(error_command, "error command%s") \
	ERR(invalid_print_function_syntax, "Invalid printable string syntax.") \
	ERR(unknown_variable, "Unknown variable.") \
	ERR(pushwarnings_without_pullwarnings, "warnings push without matching warnings pull.") \
	ERR(pullwarnings_without_pushwarnings, "warnings pull without matching warnings push.") \
	ERR(failed_to_open_file_access_denied, "Failed to open file '%s'. Access denied.") \
	ERR(failed_to_open_not_regular_file, "Failed to open file '%s'. Not a regular file (did you try to use a directory?)") \
	ERR(bad_dp_base, "The dp base should be page aligned (i.e. a multiple of 256), got %s") \
	ERR(bad_dp_optimize, "Bad dp optimize value %s, expected: [none, ram, always]") \
	ERR(bad_address_optimize, "Bad dp optimize value %s, expected: [default, ram, mirrors]") \
	ERR(bad_optimize, "Bad optimize value %s, expected: [dp, address]") \
	ERR(require_parameter, "Missing required function parameter") \
	ERR(expected_parameter, "Not enough parameters in calling of function %s") \
	ERR(unexpected_parameter, "Too many parameters in calling of function %s") \
	ERR(duplicate_param_name, "Duplicated parameter name: %s in creation of function %s") \
	ERR(invalid_alignment, "Invalid alignment. Expected a power of 2.") \
	ERR(alignment_too_big, "Requested alignment too large.") \
	ERR(negative_shift, "Bitshift amount is negative.") \
	ERR(macro_not_varadic, "Invalid use of %s, active macro is not variadic.") \
	ERR(vararg_sizeof_nomacro, "Invalid use of sizeof(...), no active macro.") \
	ERR(macro_wrong_min_params, "Variadic macro call with too few parameters") \
	ERR(vararg_out_of_bounds, "Variadic macro parameter %s is out of bounds.%s") \
	ERR(vararg_must_be_last, "Variadic macro parameter must be the last parameter.") \
	ERR(invalid_global_label, "Global label definition contains an invalid label [%s].") \
	ERR(spc700_addr_out_of_range, "Address %s out of range for instruction, valid range is 0000-1FFF") \
	ERR(label_ambiguous, "Label (%s) location is ambiguous due to straddling optimization border.") \
	ERR(feature_unavaliable_in_spcblock, "This feature may not be used while an spcblock is active") \
	ERR(endspcblock_without_spcblock, "Use of endspcblock without matching spcblock") \
	ERR(missing_endspcblock, "Use of endspcblock without matching spcblock") \
	ERR(spcblock_inside_struct, "Can not start an spcblock while a struct is still open") \
	ERR(spcblock_too_few_args, "Too few args passed to spcblock") \
	ERR(spcblock_too_many_args, "Too many args passed to spcblock") \
	ERR(unknown_spcblock_type, "Unknown spcblock format") \
	ERR(custom_spcblock_missing_macro, "Custom spcblock types must refer to a valid macro") \
	ERR(spcblock_macro_doesnt_exist, "Macro specified to custom spcblock was not found") \
	ERR(extra_spcblock_arg_for_type, "Only custom spcblock type takes a fourth argument") \
	ERR(spcblock_macro_must_be_varadic, "Custom spcblock macros must be variadic") \
	ERR(spcblock_macro_invalid_static_args, "Custom spcblock macros must have three static arguments") \
	ERR(spcblock_custom_types_incomplete, "Custom spcblock types are not yet supported. One day.") \
	ERR(invalid_endspcblock_arg, "Invalid argument to endspcblock: \"%s\"") \
	ERR(unknown_endspcblock_format, "Unsupported endspcblock format. Currently supported formats are \"endspcblock\" and \"endspcblock execute [label]\"") \
	ERR(internal_error, "An internal error occured (%s). This is a bug in Asar, please report it at https://github.com/RPGHacker/asar/issues, along with a patch that reproduces it if possible.") \
	ERR(pushns_without_pullns, "pushns without matching pullns.") \
	ERR(pullns_without_pushns, "pullns without matching pushns.") \
	ERR(label_forward, "The use of forward labels is not allowed in this context.") \
	ERR(undefined_char, "'%s' is not defined in the character table") \
	ERR(invalid_utf8, "Invalid text encoding detected. Asar expects UTF-8-encoded text. Please re-save this file in a text editor of choice using UTF-8 encoding.") \
	ERR(cmdl_utf16_to_utf8_failed, "UTF-16 to UTF-8 string conversion failed: %s.") \
	ERR(broken_command, "Broken %s command. %s") \
	ERR(oob, "Operation out of bounds: Requested index %d for object of size %d") \
	ERR(unclosed_vararg, "Variadic macro parameter wasn't closed properly.") \
	ERR(invalid_vararg, "Trying to use variadic macro parameter syntax to resolve a non variadic argument <%s>.") \
	ERR(invalid_depth_resolve, "Invalid %s resolution depth: Trying to backwards-resolve a %s using %i '^', but current scope only supports up to %i '^'.") \
	ERR(platform_paths, "Platform-specific paths aren'supported. Please use platform-independent paths (use / instead of \\).") \
	ERR(bad_single_line_for, "Single-line for loop not allowed here.") \
	ERR(broken_for_loop, "Broken for loop command: %s") \
	ERR(missing_org, "Missing org or freespace command.") \
	ERR(unclosed_block_comment, "Unclosed block comment.") \
	ERR(bad_addr_mode, "This addressing mode is not valid for this instruction.") \
	ERR(bad_access_width, "This addressing mode can accept %s arguments, but the provided argument is %d-bit.") \
	ERR(label_before_if, "Labels are not allowed before \"%s\" commands. Suggestion: move the label to a separate line.") \
// this line intentionally left blank

enum asar_error_id : int {
#define DO(id, fmt) error_id_ ## id,
	ALL_ERRORS(DO)
#undef DO
	error_id_end,
};

struct asar_error_mapping {
	const char* name;
	const char* fmt_string;
};

inline constexpr asar_error_mapping asar_all_errors[] = {
#define DO(id, fmt) { "E" #id, fmt },
	ALL_ERRORS(DO)
#undef DO
};

constexpr const char* get_error_fmt(asar_error_id errid) {
	return asar_all_errors[errid].fmt_string;
}

enum asar_error_type
{
	error_type_fatal,
	error_type_line,
	error_type_block,
	error_type_null
};

struct errfatal {};
struct errline : public errfatal {};
struct errblock : public errline {};
struct errnull : public errblock {};

#ifdef __clang__
// okay so this ## thing isn't very nice of me, but it works on all compilers
// for now. i'll refactor all use sites of asar_throw_error at some point so i
// can do a different hack to always pass at least 1 variadic argument
//edit: lol nevermind, different hack didn't work either, guess we're stuck
// with this shit until we upgrade to c++20
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#if defined(__clang__) || defined(__GNUC__)
#define FORMAT [[gnu::format(printf, 3, 4)]]
#else
// msvc only has format string checking in enterprise edition lmao
// ehh who tf even develops on windows anyways
#define FORMAT
#endif

// "Magic Trick" to get noreturn on all but the null throw functions: use
// preprocessor macros as makeshift template specializations

// i would do this with explicit template specializations, but there's a 6 year
// old open Clang bug report about [[noreturn]] not working with function
// specializations.

FORMAT void asar_throw_error_impl_error_type_null(int whichpass, asar_error_id errid, const char* fmt, ...);
FORMAT [[noreturn]] void asar_throw_error_impl_error_type_block(int whichpass, asar_error_id errid, const char* fmt, ...);
FORMAT [[noreturn]] void asar_throw_error_impl_error_type_line(int whichpass, asar_error_id errid, const char* fmt, ...);
FORMAT [[noreturn]] void asar_throw_error_impl_error_type_fatal(int whichpass, asar_error_id errid, const char* fmt, ...);
#undef FORMAT
#define asar_throw_error(whichpass, type, errid, ...) asar_throw_error_impl_ ## type(whichpass, errid, get_error_fmt(errid), ## __VA_ARGS__)
const char* get_error_name(asar_error_id errid);
