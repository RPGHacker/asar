#pragma once

// RPG Hacker: Some standard headers unfortunately like to
// spew loads of warnings on some platforms, and since this
// is kinda annoying, let's wrap the headers in here so
// that we can easily disable certain warnings via pragmas.

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4577)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 4987)
#endif

#include <new>//placement new
#include <stdlib.h>//malloc, realloc, free
#include <string.h>//strcmp, memmove
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <cstdio>

inline char * duplicate_string(const char * str)
{
	char * a = (char*)malloc(sizeof(char)*(strlen(str) + 1));
	strcpy(a, str);
	return a;
}

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
