#include "platform/file-helpers.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4668)
#endif

#include <windows.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

bool file_exists(const char * filename)
{
	return (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES);
}
