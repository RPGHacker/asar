#include "platform/file-helpers.h"
#include <stdio.h>

bool file_exists(const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (f) fclose(f);
	return f;
}
