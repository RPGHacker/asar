#include "warnings.h"

#include "asar.h"
#include <cassert>
#include <cstdarg>

#include "interface-shared.h"

struct warnings_state
{
	bool enabled[warning_id_end];
};

static warnings_state current_warnings_state;
static autoarray<warnings_state> warnings_state_stack;
static warnings_state main_warnings_state;

void asar_throw_warning_impl(int whichpass, asar_warning_id warnid, const char* fmt, ...)
{
	if (pass == whichpass)
	{
		assert(warnid >= 0 && warnid < warning_id_end);

		if (current_warnings_state.enabled[warnid])
		{
			char warning_buffer[1024];
			va_list args;
			va_start(args, fmt);

			vsnprintf(warning_buffer, sizeof(warning_buffer), fmt, args);

			va_end(args);
			warn((int)warnid, warning_buffer);
		}
	}
}

const char* get_warning_name(asar_warning_id warnid)
{
	assert(warnid >= 0 && warnid < warning_id_end);

	const asar_warning_mapping& warning = asar_all_warnings[warnid];

	return warning.name;
}



void set_warning_enabled(asar_warning_id warnid, bool enabled)
{
	assert(warnid >= 0 && warnid < warning_id_end);

	current_warnings_state.enabled[warnid] = enabled;
}

asar_warning_id parse_warning_id_from_string(const char* string)
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
	for(int i = 0; i < warning_id_end; i++)
	{
		if(!stricmpwithlower(pos, asar_all_warnings[i].name+1))
		{
			return asar_warning_id(i);
		}
	}

	return warning_id_end;
}

void reset_warnings_to_default()
{
	for (int i = 0; i < (int)warning_id_end; ++i)
	{
		const asar_warning_mapping& warning = asar_all_warnings[i];

		current_warnings_state.enabled[i] = warning.default_enabled;
	}
}

void push_warnings(bool warnings_command)
{
	if (warnings_command)
	{
		warnings_state_stack.append(current_warnings_state);
	}
	else
	{
		main_warnings_state = current_warnings_state;
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

		current_warnings_state = prev_state;

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
