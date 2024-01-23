#pragma once

#define ASAR_ERROR_RANGE_START	5000

// NOTE: Don't reorder these. That would change their ID.
// If you need to remove one, stub it out.
// If you need to add one, add it at the end (before error_id_end).
// Keep in sync with asar_errors.
enum asar_error_id : int
{
	error_id_start = ASAR_ERROR_RANGE_START,

	error_id_limit_reached,
	error_id_werror,

	error_id_16mb_rom_limit,
	error_id_buffer_too_small,
	error_id_params_null,
	error_id_params_invalid_size,
	error_id_cmdl_define_invalid,
	error_id_cmdl_define_override,
	error_id_create_rom_failed,
	error_id_open_rom_failed,
	error_id_open_rom_not_smw_extension,
	error_id_open_rom_not_smw_header,
	error_id_stddefines_no_identifier,
	error_id_stddefine_after_closing_quote,

	error_id_failed_to_open_file,
	error_id_file_not_found,
	error_id_readfile_1_to_4_bytes,
	error_id_canreadfile_0_bytes,
	error_id_file_offset_out_of_bounds,
	error_id_rep_at_file_end,

	error_id_mismatched_parentheses,
	error_id_invalid_hex_value,
	error_id_invalid_binary_value,
	error_id_invalid_character,
	error_id_string_literal_not_terminated,
	error_id_malformed_function_call,
	error_id_invalid_number,
	error_id_garbage_near_quoted_string,
	error_id_mismatched_quotes,

	error_id_rom_too_short,
	error_id_rom_title_incorrect,

	error_id_bank_border_crossed,

	error_id_start_of_file,
	error_id_xkas_asar_conflict,
	error_id_invalid_version_number,
	error_id_asar_too_old,

	error_id_relative_branch_out_of_bounds,
	error_id_snes_address_doesnt_map_to_rom,
	error_id_snes_address_out_of_bounds,
	error_id_invalid_tcall,
	error_id_use_xplus,
	error_id_opcode_length_too_long,
	error_id_superfx_invalid_short_address,
	error_id_superfx_invalid_register,
	error_id_invalid_opcode_length,
	error_id_invalid_mapper,

	error_id_nan,
	error_id_division_by_zero,
	error_id_modulo_by_zero,
	error_id_unknown_operator,
	error_id_invalid_input,

	error_id_invalid_function_name,
	error_id_function_not_found,
	error_id_function_redefined,
	error_id_broken_function_declaration,
	error_id_wrong_num_parameters,
	error_id_invalid_param_name,
	error_id_math_invalid_type,

	error_id_invalid_label_name,
	error_id_label_not_found,
	error_id_label_redefined,
	error_id_broken_label_definition,
	error_id_label_cross_assignment,
	error_id_macro_label_outside_of_macro,
	error_id_label_on_third_pass,
	error_id_label_moving,

	error_id_invalid_namespace_name,
	error_id_invalid_namespace_use,

	error_id_invalid_struct_name,
	error_id_struct_not_found,
	error_id_struct_redefined,
	error_id_struct_invalid_parent_name,
	error_id_invalid_label_missing_closer,
	error_id_multiple_subscript_operators,
	error_id_invalid_subscript,
	error_id_label_missing_parent,
	error_id_array_invalid_inside_structs,
	error_id_struct_without_endstruct,
	error_id_nested_struct,
	error_id_missing_struct_params,
	error_id_too_many_struct_params,
	error_id_missing_extends,
	error_id_invalid_endstruct_count,
	error_id_expected_align,
	error_id_endstruct_without_struct,
	error_id_alignment_too_small,

	error_id_invalid_define_name,
	error_id_define_not_found,
	error_id_broken_define_declaration,
	error_id_overriding_builtin_define,
	error_id_define_label_math,
	error_id_mismatched_braces,

	error_id_invalid_macro_name,
	error_id_macro_not_found,
	error_id_macro_redefined,
	error_id_broken_macro_declaration,
	error_id_invalid_macro_param_name,
	error_id_macro_param_not_found,
	error_id_macro_param_redefined,
	error_id_broken_macro_usage,
	error_id_macro_wrong_num_params,
	error_id_broken_macro_contents,
	error_id_rep_at_macro_end,
	error_id_nested_macro_definition,
	error_id_misplaced_endmacro,
	error_id_unclosed_macro,

	error_id_label_in_conditional,
	error_id_broken_conditional,
	error_id_invalid_condition,
	error_id_misplaced_elseif,
	error_id_elseif_in_while,
	error_id_elseif_in_singleline,
	error_id_misplaced_endif,
	error_id_misplaced_else,
	error_id_else_in_while_loop,
	error_id_unclosed_if,

	error_id_unknown_command,
	error_id_command_disabled,

	error_id_broken_incbin,
	error_id_incbin_64kb_limit,
	error_id_recursion_limit,
	error_id_command_in_non_root_file,
	error_id_cant_be_main_file,
	error_id_no_labels_here,

	error_id_invalid_freespace_request,
	error_id_no_banks_with_ram_mirrors,
	error_id_no_freespace_norom,
	error_id_static_freespace_autoclean,
	error_id_static_freespace_growing,
	error_id_no_freespace_in_mapped_banks,
	error_id_no_freespace,
	error_id_freespace_limit_reached,

	error_id_prot_not_at_freespace_start,
	error_id_prot_too_many_entries,

	error_id_autoclean_in_freespace,
	error_id_autoclean_label_at_freespace_end,
	error_id_broken_autoclean,

	error_id_pulltable_without_table,
	error_id_invalid_table_file,

	error_id_pad_in_freespace,

	error_id_org_label_invalid,
	error_id_org_label_forward,

	error_id_skip_label_invalid,

	error_id_spc700_inline_no_base,
	error_id_base_label_invalid,

	error_id_rep_label,

	error_id_pushpc_without_pullpc,
	error_id_pullpc_without_pushpc,
	error_id_pullpc_different_arch,
	error_id_pullbase_without_pushbase,

	error_id_invalid_math,
	error_id_invalid_warn,
	error_id_invalid_check,

	error_id_warnpc_in_freespace,
	error_id_warnpc_broken_param,
	error_id_warnpc_failed,
	error_id_warnpc_failed_equal,

	error_id_assertion_failed,

	error_id_error_command,

	error_id_invalid_print_function_syntax,
	error_id_unknown_variable,

	error_id_invalid_warning_id,

	error_id_pushwarnings_without_pullwarnings,
	error_id_pullwarnings_without_pushwarnings,

	error_id_failed_to_open_file_access_denied,
	error_id_failed_to_open_not_regular_file,

	error_id_bad_dp_base,
	error_id_bad_dp_optimize,
	error_id_bad_address_optimize,
	error_id_bad_optimize,

	error_id_require_parameter,
	error_id_expected_parameter,
	error_id_unexpected_parameter,
	error_id_duplicate_param_name,

	error_id_invalid_alignment,
	error_id_alignment_too_big,

	error_id_negative_shift,

	error_id_macro_not_varadic,
	error_id_vararg_sizeof_nomacro,
	error_id_macro_wrong_min_params,
	error_id_vararg_out_of_bounds,
	error_id_vararg_must_be_last,

	error_id_invalid_global_label,

	error_id_spc700_addr_out_of_range,
	error_id_label_ambiguous,

	error_id_feature_unavaliable_in_spcblock,
	error_id_endspcblock_without_spcblock,
	error_id_missing_endspcblock,
	error_id_spcblock_bad_arch,
	error_id_spcblock_inside_struct,
	error_id_spcblock_too_few_args,
	error_id_spcblock_too_many_args,
	error_id_unknown_spcblock_type,
	error_id_custom_spcblock_missing_macro,
	error_id_spcblock_macro_doesnt_exist,
	error_id_extra_spcblock_arg_for_type,
	error_id_spcblock_macro_must_be_varadic,
	error_id_spcblock_macro_invalid_static_args,
	error_id_spcblock_custom_types_incomplete,
	error_id_startpos_without_spcblock,
	error_id_invalid_endspcblock_arg,
	error_id_unknown_endspcblock_format,
	error_id_internal_error,

	error_id_pushns_without_pullns,
	error_id_pullns_without_pushns,

	error_id_label_forward,
	
	error_id_unclosed_vararg,
	error_id_invalid_vararg,

	error_id_macro_param_outside_macro,

	error_id_broken_for_loop,
	error_id_bad_single_line_for,

	error_id_end,
	error_id_count = error_id_end - error_id_start - 1,
};

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

void asar_throw_error(int whichpass, asar_error_type type, asar_error_id errid, ...);
const char* get_error_name(asar_error_id errid);
