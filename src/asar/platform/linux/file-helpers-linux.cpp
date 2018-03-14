#include "platform/file-helpers.h"
#include <sys/stat.h>

bool file_exists(const char * filename)
{
	struct stat st;
	return (!stat(filename, &st));
}

bool path_is_absolute(const char* path)
{
	return ('/' == path[0]);
}

char get_native_path_separator()
{
	return '/';
}
