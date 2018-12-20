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

bool check_is_regular_file(const char* path)
{
	struct stat finfo;
	if (stat(path, &finfo) == 0)
	{
		// either regular file or symlink
		if (finfo.st_mode & (S_IFREG | S_IFLNK))
			return true;
	}
	return false;
}
