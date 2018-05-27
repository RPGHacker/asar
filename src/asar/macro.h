#pragma once

void startmacro(const char * line);
void tomacro(const char * line);
void endmacro(bool insert);
void callmacro(const char * data);

extern int macrorecursion;
extern int reallycalledmacros;
extern int calledmacros;

struct macrodata
{
	autoarray<string> lines;
	int numlines;
	int startline;
	const char * fname;
	const char * const* arguments;
	int numargs;
};

extern assocarr<macrodata*> macros;
