#include "platform/file-helpers.h"
#include <stdio.h>

bool file_exists(const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (f) fclose(f);
	return f;
}

bool path_is_absolute(const char* path)
{
	return ('/' == path[0]);
}

char get_native_path_separator()
{
	return '/';
}

bool check_is_regular_file(const char* path)
{
	// no really universal way to check this
	return true;
}
