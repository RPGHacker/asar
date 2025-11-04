#include <vector>
#include "libstr.h"

// construct a mapping between warning ids and warning names.
struct warn_t;
std::vector<warn_t> all_warnings;
int warning_id_end = 0;
struct warn_t {
	string name;
	bool exists;
	bool is_default_enabled;
	warn_t() : name(nullptr), exists(false), is_default_enabled(false) {}
	warn_t(int id, const char *name_in, bool is_default_enabled)
	: exists(true), is_default_enabled(is_default_enabled) {
		if((int)all_warnings.size() <= id) all_warnings.resize(id + 1);
		// convert warn_xxx -> Wxxx
		name = STR "W" + (name_in + 5);
		all_warnings[id] = *this;
		warning_id_end = all_warnings.size();
	}
};

// i love this fucking language
#define CAT(a,b) a##b
#define CAT2(a,b) CAT(a,b)
#define SAVE_WARN_NAME(id, name, is_enabled) static warn_t CAT2(_useless_,__LINE__) { id, name, is_enabled }
#include "warnings.h"

#include "asar.h"
#include <cassert>
#include <cstdarg>

#include "interface-shared.h"

struct warnings_state
{
	std::vector<bool> enabled;
	warnings_state() : enabled(all_warnings.size(), false) {}
};

static warnings_state current_warnings_state;
static autoarray<warnings_state> warnings_state_stack;
static warnings_state main_warnings_state;

void warn_impl(int warnid, const char* fmt, int whichpass, ...)
{
	if (pass == whichpass)
	{
		assert(warnid >= 0 && warnid < warning_id_end);

		if (current_warnings_state.enabled[warnid])
		{
			char warning_buffer[1024];
			va_list args;
			va_start(args, whichpass);

			vsnprintf(warning_buffer, sizeof(warning_buffer), fmt, args);

			va_end(args);
			warn((int)warnid, warning_buffer);
		}
	}
}

const char* get_warning_name(asar_warning_id warnid)
{
	assert(warnid >= 0 && warnid < warning_id_end);

	const warn_t& warning = all_warnings[warnid];
	assert(warning.exists);

	return warning.name.data();
}



void set_warning_enabled(asar_warning_id warnid, bool enabled)
{
	assert(warnid >= 0 && warnid < warning_id_end);
	assert(all_warnings[warnid].exists);

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
		if(all_warnings[i].exists && !stricmpwithlower(pos, all_warnings[i].name.data()+1))
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
		const warn_t& warning = all_warnings[i];

		current_warnings_state.enabled[i] = warning.is_default_enabled;
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
		throw_err_block(0, err_pullwarnings_without_pushwarnings);
	}
}

void verify_warnings()
{
	if (warnings_state_stack.count > 0)
	{
		throw_err_null(0, err_pushwarnings_without_pullwarnings);

		warnings_state_stack.reset();
	}
}
