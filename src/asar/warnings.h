#pragma once

#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#if defined(__clang__) || defined(__GNUC__)
[[gnu::format(printf, 2, 4)]]
#endif
void warn_impl(int warn_id, const char* fmt_string, int whichpass, ...);

#ifndef SAVE_WARN_NAME
#define SAVE_WARN_NAME(...) // only used in warnings.cpp
#endif

#define WRN(id, fmt_string, is_enabled) \
	SAVE_WARN_NAME(__LINE__ - _warn_startline, #id, is_enabled); \
	template<typename... Ts> inline void id(int whichpass, Ts... args) { \
		warn_impl(__LINE__ - _warn_startline, fmt_string, whichpass, args...); \
	}

static constexpr int _warn_startline = __LINE__;
WRN(warn_relative_path_used, "Relative %s path passed to asar_patch_ex() - please use absolute paths only to prevent undefined behavior!", true)
WRN(warn_rom_too_short, "ROM is too short to have a title. (Expected '%s')", true)
WRN(warn_rom_title_incorrect, "ROM title is incorrect. Expected '%s', got '%s'.", true)
WRN(warn_spc700_assuming_8_bit, "This opcode does not exist with 16-bit parameters, assuming 8-bit.", true)
WRN(warn_assuming_address_mode, "The addressing mode %s is not valid for this instruction, assuming %s.%s", true)
WRN(warn_set_middle_byte, "It would be wise to set the 008000 bit of this address.", true)
WRN(warn_freespace_leaked, "This freespace appears to be leaked.", true)
WRN(warn_warn_command, "warn command%s", true)
WRN(warn_implicitly_sized_immediate, "Implicitly sized immediate.", false)
WRN(warn_check_memory_file, "Accessing file '%s' which is not in memory while Wcheck_memory_file is enabled.", false)
WRN(warn_datasize_last_label, "Datasize used on last detected label '%s'.", true)
WRN(warn_datasize_exceeds_size, "Datasize exceeds 0xFFFF for label '%s'.", true)
WRN(warn_mapper_already_set, "A mapper has already been selected.", true)
WRN(warn_feature_deprecated, "DEPRECATION NOTIFICATION: Feature \"%s\" is deprecated and will be REMOVED in the future. Please update your code to conform to newer styles. Suggested work around: %s.", true)
WRN(warn_invalid_warning_id, "Warning '%s' (passed to %s) doesn't exist.", true)
WRN(warn_byte_order_mark_utf8, "UTF-8 byte order mark detected and skipped.", true)
WRN(warn_for_loop_define, "For loop variable name underwent define expansion - this is very likely not what you wanted. Loop variables are specified without !. Remove the !, or use !{name} to silence this warning.", true)
WRN(warn_cx4_a_operand_reads_zero, "Register A reads as 0 when used as an operand. Use #$00 if that is intended, or copy A to another register with ST first.", true)
WRN(warn_cx4_write_ignored, "Writes to %s are ignored, so this ST does nothing.", true)
WRN(warn_cx4_bus_write_ignores_value, "A bus write through %s sends MDR, not the stored value. Use ST %s, MDR.", true)
WRN(warn_cx4_register_low_bits, "%s only uses the low 4 bits of the register code, so %s ($%s) acts as R%s. Write R%s if that is intended.", true)
WRN(warn_cx4_bus_read_reads_zero, "Reading %s returns 0 and starts a bus read into MDR. Use LD MDR, %s, and read MDR after WAIT, instead.", true)
WRN(warn_cx4_last_word_of_page, "This instruction is at the last word of a 512-byte page. The Cx4 switches to page P (or stops) before running it, so execution doesn't go on in the next page.", true)
WRN(warn_cx4_branch_last_word_of_page, "%s is at the last word of a 512-byte page. The Cx4 switches to page P (or stops) before running it.%s", true)
WRN(warn_cx4_skip_second_to_last_word_of_page, "%s is at the second-to-last word of a 512-byte page. If it skips, the Cx4 switches to page P, or stops if it's in cache page 1.", true)
WRN(warn_cx4_constant_shift_count, "Shifting by %s always shifts by 0. A register shift count only uses the low 5 bits of the value ($%s), and a count above 24 acts as 0.", true)
WRN(warn_cx4_shift_count_above_24, "Shift count %s is above 24, so it shifts by 0.", true)
WRN(warn_cx4_mach_write_truncated, "A store to MACH only keeps bits 0-7 of the value, and clears bits 32-47 of the multiply result.", true)

#undef WRN
#define throw_warning(whichpass, id, ...) id(whichpass, ## __VA_ARGS__)

using asar_warning_id = int;
extern asar_warning_id warning_id_end;
const char* get_warning_name(asar_warning_id warnid);

void set_warning_enabled(asar_warning_id warnid, bool enabled);

// Supported string format: wXXXX, WXXXX or XXXX.
// Returns warning_id_end if the string is malformed
// or the ID wasn't found.
asar_warning_id parse_warning_id_from_string(const char* string);

void reset_warnings_to_default();

void push_warnings(bool warnings_command = true);
void pull_warnings(bool warnings_command = true);
void verify_warnings();
