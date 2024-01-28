#include "asar.h"
#include <cassert>
#include <cstdarg>

#include "interface-shared.h"

static void fmt_error(int whichpass, asar_error_id errid, const char* fmt, ...) {
	assert(errid >= 0 && errid < error_id_end);

	char error_buffer[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(error_buffer, sizeof(error_buffer), fmt, args);
	va_end(args);

	error_interface((int)errid, whichpass, error_buffer);
}

// this is uncomfortably much to copy paste,,, but whatever
void asar_throw_error_impl_error_type_null(int whichpass, asar_error_id errid, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	fmt_error(whichpass, errid, fmt, args);
	va_end(args);
}
void asar_throw_error_impl_error_type_block(int whichpass, asar_error_id errid, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	fmt_error(whichpass, errid, fmt, args);
	va_end(args);
	throw errblock{};
}
void asar_throw_error_impl_error_type_line(int whichpass, asar_error_id errid, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	fmt_error(whichpass, errid, fmt, args);
	va_end(args);
	throw errline{};
}
void asar_throw_error_impl_error_type_fatal(int whichpass, asar_error_id errid, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	fmt_error(whichpass, errid, fmt, args);
	va_end(args);
	throw errfatal{};
}

const char* get_error_name(asar_error_id errid)
{
	assert(errid >= 0 && errid < error_id_end);

	const asar_error_mapping& error = asar_all_errors[errid];

	return error.name;
}
