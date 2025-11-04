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
