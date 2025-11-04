#include <cassert>
#include <cstdarg>

#include "std-includes.h"
#include "errors.h"
#include "interface-shared.h"
#include "libstr.h"

void error_impl(const char* error_id, const char* fmt_string, int whichpass, ...) {
	va_list args; va_start(args, whichpass);
	char error_buffer[1024];
	vsnprintf(error_buffer, sizeof(error_buffer), fmt_string, args);

	// convert err_xxx -> Exxx
	string errname = STR "E" + (error_id + 4);

	error_interface(errname.data(), whichpass, error_buffer);
}
