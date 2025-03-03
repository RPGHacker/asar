#include <cassert>
#include <cstdarg>

#include "std-includes.h"
#include "errors.h"
#include "interface-shared.h"

void error_impl(const char* error_id, const char* fmt_string, int whichpass, ...) {
	va_list args; va_start(args, whichpass);
	char error_buffer[1024];
	vsnprintf(error_buffer, sizeof(error_buffer), fmt_string, args);

	error_interface(error_id, whichpass, error_buffer);
}
