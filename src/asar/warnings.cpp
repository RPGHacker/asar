#include "warnings.h"

#include "asar.h"
#include <assert.h>
#include <stdarg.h>

#include "interface-shared.h"

static int asar_num_warnings = 0;
bool suppress_all_warnings = false;

struct asar_warning_mapping
{
	asar_warning_id warnid;
	const char* name;
	const char* message;
	bool enabled;
	bool enabled_default;

	asar_warning_mapping(asar_warning_id inwarnid, const char *iname, const char* inmessage, bool inenabled = true)
	{
		++asar_num_warnings;

		warnid = inwarnid;
		name = iname;
		message = inmessage;
		enabled = inenabled;
		enabled_default = inenabled;

		// RPG Hacker: Sanity check. This makes sure that the order
		// of entries in asar_warnings matches with the order of
		// constants in asar_warning_id. This is important because
		// we access asar_warnings as an array.
		// Would love to do this via static_assert(), but can't
		// think of a way to do so.
		assert(warnid - warning_id_start == asar_num_warnings);
	}
};

// Keep in sync with asar_warning_id.
// Both, enum mapping and order, must match.
#define WRN(name) warning_id_ ## name, "W" #name
static asar_warning_mapping asar_warnings[] =
{
	{ WRN(relative_path_used), "Relative %s path passed to asar_patch_ex() - please use absolute paths only to prevent undefined behavior!" },

	{ WRN(rom_too_short), "ROM is too short to have a title. (Expected '%s')" },
	{ WRN(rom_title_incorrect), "ROM title is incorrect. Expected '%s', got '%s'." },

	{ WRN(65816_yy_x_does_not_exist), "($yy),x does not exist, assuming $yy,x." },
	{ WRN(65816_xx_y_assume_16_bit), "%s $xx,y is not valid with 8-bit parameters, assuming 16-bit." },
	{ WRN(spc700_assuming_8_bit), "This opcode does not exist with 16-bit parameters, assuming 8-bit." },

	{ WRN(cross_platform_path), "This patch may not assemble cleanly on all platforms. Please use / instead." },

	{ WRN(missing_org), "Missing org or freespace command." },
	{ WRN(set_middle_byte), "It would be wise to set the 008000 bit of this address." },

	{ WRN(unrecognized_special_command), "Unrecognized special command - your version of Asar might be outdated." },

	{ WRN(freespace_leaked), "This freespace appears to be leaked." },

	{ WRN(warn_command), "warn command%s" },

	{ WRN(implicitly_sized_immediate), "Implicitly sized immediate.", false },

	{ WRN(xkas_deprecated), "xkas support is being deprecated and will be removed in a future version of Asar. Please use an older version of Asar (<=1.50) if you need it." },
	{ WRN(xkas_eat_parentheses), "xkas compatibility warning: Unlike xkas, Asar does not eat parentheses after defines." },
	{ WRN(xkas_label_access), "xkas compatibility warning: Label access is always 24bit in emulation mode, but may be 16bit in native mode." },
	{ WRN(xkas_warnpc_relaxed), "xkas conversion warning : warnpc is relaxed one byte in Asar." },
	{ WRN(xkas_style_conditional), "xkas-style conditional compilation detected. Please use the if command instead." },
	{ WRN(xkas_patch), "If you want to assemble an xkas patch, add ;@xkas at the top or you may run into a couple of problems." },
	{ WRN(xkas_incsrc_relative), "xkas compatibility warning: incsrc and incbin look for files relative to the patch in Asar, but xkas looks relative to the assembler." },
	{ WRN(convert_to_asar), "Convert the patch to native Asar format instead of making an Asar-only xkas patch." },

	{ WRN(fixed_deprecated), "the 'fixed' parameter on freespace/freecode/freedata is deprecated - please use 'static' instead." },

	{ WRN(autoclear_deprecated), "'autoclear' is deprecated - please use 'autoclean' instead." },

	{ WRN(check_memory_file), "Accessing file '%s' which is not in memory while W%d is enabled.", false },

	{ WRN(if_not_condition_deprecated), "'if !condition' is deprecated - please use 'if not(condition)' instead." },

	{ WRN(function_redefined), "Function '%s' redefined." },

	{ WRN(datasize_last_label), "Datasize used on last detected label '%s'." },
	{ WRN(datasize_exceeds_size), "Datasize exceeds 0xFFFF for label '%s'." },

	{ WRN(mapper_already_set), "A mapper has already been selected." },
	{ WRN(feature_deprecated), "DEPRECATION NOTIFICATION: Feature \"%s\" is deprecated and will be REMOVED in the future. Please update your code to conform to newer styles. Suggested work around: %s." },

	{ WRN(byte_order_mark_utf8), "UTF-8 byte order mark detected and skipped." },
	{ WRN(optimization_settings), "In Asar 2.0, the default optimization settings will change to `optimize dp always` and `optimize address mirrors`, which changes this instruction's argument from %d to %d bytes. Either specify the desired settings manually or use explicit length suffixes to silence this warning." },
};

// RPG Hacker: Sanity check. This makes sure that the element count of asar_warnings
// matches with the number of constants in asar_warning_id. This is important, because
// we are going to access asar_warnings as an array.
static_assert(sizeof(asar_warnings) / sizeof(asar_warnings[0]) == warning_id_count, "asar_warnings and asar_warning_id are not in sync");

void asar_throw_warning(int whichpass, asar_warning_id warnid, ...)
{
	if (pass == whichpass && !suppress_all_warnings)
	{
		assert(warnid > warning_id_start && warnid < warning_id_end);

		const asar_warning_mapping& warning = asar_warnings[warnid - warning_id_start - 1];

		if (warning.enabled)
		{
			char warning_buffer[1024];
			va_list args;
			va_start(args, warnid);

#if defined(__clang__)
#	pragma clang diagnostic push
// "format string is not a literal".
// The pointer we're passing here should always point to a string literal,
// thus, I think, we can safely ignore this here.
#	pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

			vsnprintf(warning_buffer, sizeof(warning_buffer), warning.message, args);

#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

			va_end(args);
			warn((int)warnid, warning_buffer);
		}
	}
}

const char* get_warning_name(asar_warning_id warnid)
{
	assert(warnid > warning_id_start && warnid < warning_id_end);

	const asar_warning_mapping& warning = asar_warnings[warnid - warning_id_start - 1];

	return warning.name;
}



void set_warning_enabled(asar_warning_id warnid, bool enabled)
{
	assert(warnid > warning_id_start && warnid < warning_id_end);

	asar_warning_mapping& warning = asar_warnings[warnid - warning_id_start - 1];

	warning.enabled = enabled;
}

asar_warning_id parse_warning_id_from_string(const char* string, int warn_pass)
{
	const char* pos = string;

	if (pos == nullptr)
	{
		return warning_id_end;
	}


	if (pos[0] == 'w' || pos[0] == 'W')
	{
		++pos;
	}
	for(int i = 0; i < warning_id_end-warning_id_start-1; i++)
	{
		if(!stricmpwithlower(pos, asar_warnings[i].name+1))
		{
			return asar_warnings[i].warnid;
		}
	}
	char* endpos = nullptr;
	int numid = (int)strtol(pos, &endpos, 10);

	if (endpos == nullptr || endpos[0] != '\0')
	{
		return warning_id_end;
	}

	asar_warning_id warnid = (asar_warning_id)numid;

	if (warnid <= warning_id_start || warnid >= warning_id_end)
	{
		return warning_id_end;
	}

	asar_throw_warning(warn_pass, warning_id_feature_deprecated, "Numerical warnings", "Please transition to Wwarning_name");
	return warnid;
}

void reset_warnings_to_default()
{
	for (int i = (int)(warning_id_start + 1); i < (int)warning_id_end; ++i)
	{
		asar_warning_mapping& warning = asar_warnings[i - (int)warning_id_start - 1];

		warning.enabled = warning.enabled_default;
	}
}

struct warnings_state
{
	bool enabled[warning_id_count];
};

static autoarray<warnings_state> warnings_state_stack;
static warnings_state main_warnings_state;

void push_warnings(bool warnings_command)
{
	warnings_state current_state;

	for (int i = 0; i < (int)warning_id_count; ++i)
	{
		current_state.enabled[i] = asar_warnings[i].enabled;
	}

	if (warnings_command)
	{
		warnings_state_stack.append(current_state);
	}
	else
	{
		main_warnings_state = current_state;
	}
}

void pull_warnings(bool warnings_command)
{
	if (warnings_state_stack.count > 0 || !warnings_command)
	{
		warnings_state prev_state;

		if (warnings_command)
		{
			prev_state = warnings_state_stack[warnings_state_stack.count - 1];
		}
		else
		{
			prev_state = main_warnings_state;
		}

		for (int i = 0; i < (int)warning_id_count; ++i)
		{
			asar_warnings[i].enabled = prev_state.enabled[i];
		}

		if (warnings_command)
		{
			warnings_state_stack.remove(warnings_state_stack.count - 1);
		}
	}
	else
	{
		asar_throw_error(0, error_type_block, error_id_pullwarnings_without_pushwarnings);
	}
}

void verify_warnings()
{
	if (warnings_state_stack.count > 0)
	{
		asar_throw_error(0, error_type_null, error_id_pushwarnings_without_pullwarnings);

		warnings_state_stack.reset();
	}
}
