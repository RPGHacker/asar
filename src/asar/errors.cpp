#include "asar.h"
#include <cassert>
#include <cstdarg>

#include "interface-shared.h"

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
void asar_throw_error_impl(int whichpass, asar_error_type type, asar_error_id errid, const char* fmt, ...)
{
	assert(errid >= 0 && errid < error_id_end);

	char error_buffer[1024];
	va_list args;
	va_start(args, fmt);

#if defined(__clang__)
#	pragma clang diagnostic push
	// "format string is not a literal".
	// The pointer we're passing here should always point to a string literal,
	// thus, I think, we can safely ignore this here.
#	pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

	vsnprintf(error_buffer, sizeof(error_buffer), fmt, args);

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
	assert(errid >= 0 && errid < error_id_end);

	const asar_error_mapping& error = asar_all_errors[errid];

	return error.name;
}
