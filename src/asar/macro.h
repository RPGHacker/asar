#pragma once

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
	int numargs;
	bool variadic;
};

extern assocarr<macrodata*> macros;
extern macrodata* current_macro;
extern const char* const* current_macro_args;
extern int current_macro_numargs;
