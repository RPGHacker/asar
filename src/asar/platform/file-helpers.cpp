#include "platform/file-helpers.h"
// This code is based in part on nall, which is under the following licence.
// This modified version is licenced under the LGPL version 3 or later. See the LICENSE file
// for details.
//
//   Copyright (c) 2006-2015  byuu
//
// Permission to use, copy, modify, and/or distribute this software for
// any purpose with or without fee is hereby granted, provided that the
// above copyright notice and this permission notice appear in all
// copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
// DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
// PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
// TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

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

	int length = combined.truelen();

	if (combined.len > 0
		&& combined[length - 1] != '\\' && combined[length - 1] != '/')
	{
		combined += get_native_path_separator();
	}

	combined += path2;

	return combined;
}

string get_base_name(char const *name)
{
	string result = name;
	for(signed i = strlen(result); i >= 0; i--)
	{
		if(result[i] == '/' || result[i] == '\\')
		{
			//file has no extension
			break;
		}
		if(result[i] == '.')
		{
			result[i] = 0;
			break;
		}
	}
	return result;
}
