#pragma once

// RPG Hacker: Some standard headers unfortunately like to
// spew loads of warnings on some platforms, and since this
// is kinda annoying, let's wrap the headers in here so
// that we can easily disable certain warnings via pragmas.

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 4987)
#endif

#include <new>//placement new
#include <stdlib.h>//malloc, realloc, free
// we use our own strdup since the standard one is inconsistent
#define strdup strdup_no
#include <string.h>//strcmp, memmove
#undef strdup
inline char * strdup(const char * str) throw ()
{
	char * a = (char*)malloc(sizeof(char)*(strlen(str) + 1));
	strcpy(a, str);
	return a;
}
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <cstdio>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
