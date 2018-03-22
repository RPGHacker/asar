#include "platform/file-helpers.h"
#include "autoarray.h"

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

// get a path that you can actually use to open a file
// (note that this may still be relative, but fopen() will work on it, so you can use it as a basepath properly)
// returns empty string if not found
// copied from virtualfile.cpp, maybe make that use this function?
string get_real_path(const char* path, const char* base_path, const autoarray<string>& include_paths)
{
	string path_to_use = "";

	// First check if path is absolute
	if (path_is_absolute(path))
	{
		if (file_exists(path))
		{
			path_to_use = path;
		}
	}
	else
	{
		// Now check if path exists relative to the base path
		string test_path = "";

		if (base_path != nullptr && base_path[0] != '\0')
		{
			test_path = create_combined_path(dir(base_path), path);
		}

		if (test_path != "" && file_exists(test_path))
		{
			path_to_use = test_path;
		}
		else
		{
			// Finally check if path exists relative to any include path
			for (int i = 0; i < include_paths.count; ++i)
			{
				test_path = create_combined_path(include_paths[i], path);

				if (file_exists(test_path))
				{
					path_to_use = test_path;
					break;
				}
			}
		}
	}
	return path_to_use;
}