// RPG Hacker: We can't include that here, because it leads to crazy
// errors in windows.h - probably related to our string class?!?
//#include "platform/file-helpers.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 4987)
#endif

#include <windows.h>
#include <ctype.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#include <ctype.h>

#include "../file-helpers.h"

bool file_exists(const char * filename)
{
	return (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES);
}

bool path_is_absolute(const char* path)
{
	return ((isalpha(path[0]) &&
		(':' == path[1]) && (('\\' == path[2]) || ('/' == path[2])))	/* drive letter + root, e.g. C:\dir or C:/dir */
		|| ('\\' == path[0]) || ('/' == path[0]));						/* just root, e.g. \dir or /dir */
}

char get_native_path_separator()
{
	return '\\';
}

bool check_is_regular_file(const char* path)
{
	return !(GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY);
}
