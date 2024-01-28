#pragma once

#define ALL_WARNINGS(WRN) \
	WRN(relative_path_used, "Relative %s path passed to asar_patch_ex() - please use absolute paths only to prevent undefined behavior!") \
	WRN(rom_too_short, "ROM is too short to have a title. (Expected '%s')") \
	WRN(rom_title_incorrect, "ROM title is incorrect. Expected '%s', got '%s'.") \
	WRN(spc700_assuming_8_bit, "This opcode does not exist with 16-bit parameters, assuming 8-bit.") \
	WRN(assuming_address_mode, "The addressing mode %s is not valid for this instruction, assuming %s.%s") \
	WRN(set_middle_byte, "It would be wise to set the 008000 bit of this address.") \
	WRN(freespace_leaked, "This freespace appears to be leaked.") \
	WRN(warn_command, "warn command%s") \
	WRN(implicitly_sized_immediate, "Implicitly sized immediate.", false) \
	WRN(check_memory_file, "Accessing file '%s' which is not in memory while W%d is enabled.", false) \
	WRN(datasize_last_label, "Datasize used on last detected label '%s'.") \
	WRN(datasize_exceeds_size, "Datasize exceeds 0xFFFF for label '%s'.") \
	WRN(mapper_already_set, "A mapper has already been selected.") \
	WRN(feature_deprecated, "DEPRECATION NOTIFICATION: Feature \"%s\" is deprecated and will be REMOVED in the future. Please update your code to conform to newer styles. Suggested work around: %s.") \
	WRN(invalid_warning_id, "Warning '%s' (passed to %s) doesn't exist.") \
	WRN(byte_order_mark_utf8, "UTF-8 byte order mark detected and skipped.") \
// this line intentionally left blank

enum asar_warning_id : int {
#define DO(id, ...) warning_id_ ## id,
	ALL_WARNINGS(DO)
#undef DO
	warning_id_end,
};

struct asar_warning_mapping {
	const char* name;
	const char* fmt_string;
	bool default_enabled = true;
};

inline constexpr asar_warning_mapping asar_all_warnings[] = {
#define DO(id, ...) { "W" #id, __VA_ARGS__ },
	ALL_WARNINGS(DO)
#undef DO
};

constexpr const char* get_warning_fmt(asar_warning_id warnid) {
	return asar_all_warnings[warnid].fmt_string;
}

// see errors.h for some explanation of this whole mess
#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#if defined(__clang__) || defined(__GNUC__)
[[gnu::format(printf, 3, 4)]]
#endif
void asar_throw_warning_impl(int whichpass, asar_warning_id warnid, const char* fmt, ...);
#define asar_throw_warning(whichpass, warnid, ...) asar_throw_warning_impl(whichpass, warnid, get_warning_fmt(warnid), ## __VA_ARGS__)
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
