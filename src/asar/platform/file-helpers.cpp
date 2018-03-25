#include "platform/file-helpers.h"

//modified from somewhere in nall (license: ISC)
string dir(char const *name)
{
	string result = name;
	for (signed i = (int)strlen(result); i >= 0; i--)
	{
		if (result[i] == '/' || result[i] == '\\')
		{
			result[i + 1] = 0;
			break;
		}
		if (i == 0) result = "";
	}
	return result;
}

string create_combined_path(const char* path1, const char* path2)
{
	string combined = path1;

	if (combined.len > 0
		&& combined[combined.len - 1] != '\\' && combined[combined.len - 1] != '/')
	{
		combined += get_native_path_separator();
	}

	combined += path2;

	return combined;
}
