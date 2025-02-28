#pragma once
#include "libstr.h"
#include "assocarr.h"
#include "autoarray.h"

void startmacro(const char * line);
void tomacro(const char * line);
void endmacro(bool insert);
void callmacro(const char * data);
string replace_macro_args(const char* line);

extern int macrorecursion;
extern int calledmacros;
extern int reallycalledmacros;
extern bool inmacro;
extern int numvarargs;

struct macrodata
{
	autoarray<string> lines;
	int numlines;
	int startline;
	const char * fname;
	const char * const* arguments;
	const char *arguments_buffer;
	int numargs;
	bool variadic;
	const macrodata* parent_macro;
	int parent_macro_num_varargs;
};

void freemacro(macrodata* & macro);

extern assocarr<macrodata*> macros;
