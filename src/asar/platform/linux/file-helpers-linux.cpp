#include "platform/file-helpers.h"
#include <sys/stat.h>

bool file_exists(const char * filename)
{
	struct stat st;
	return (!stat(filename, &st));
}
