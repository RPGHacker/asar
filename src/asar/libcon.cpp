#include "std-includes.h"
#include "libcon.h"
#include <signal.h>

static char * progname;
static char ** args;
static int argsleft;
bool libcon_interactive;
static const char * usage;

static volatile bool confirmclose=true;
void libcon_pause()
{
	if (confirmclose)
	{
		confirmclose=false;
#if defined(_WIN32)
		system("pause");
#else
		printf("Press Enter to continue");
		getchar();
#endif
		confirmclose=true;
	}
}

void libcon_badusage()
{
	printf("usage: %s %s", progname, usage);
	exit(1);
}

static const char * getarg(bool tellusage, const char * defval= nullptr)
{
	if (!argsleft)
	{
		if (tellusage) libcon_badusage();
		return defval;
	}
	args++;
	argsleft--;
	return args[0];
}

static const char * getfname(bool tellusage, const char * defval= nullptr)
{
	return getarg(tellusage, defval);
	//char * rval=malloc(char, 256);
	//char * rvalend=rval;
	//*rval=0;
	//while (!strchr(rval, '.'))
	//{
	//	char * thisword=getarg(false, nullptr);
	//	if (!thisword)
	//	{
	//		if (tellusage) libcon_badusage();
	//		else return defval;
	//	}
	//	if (rval!=rvalend) *(rvalend++)=' ';
	//	rvalend+=sprintf(rvalend, "%s", thisword);
	//}
	//return rval;
}

static const char * requirestrfromuser(const char * question, bool filename)
{
	confirmclose=false;
	char * rval=(char*)malloc(256);
	*rval=0;
	while (!strchr(rval, '\n') || *rval=='\n')
	{
		*rval=0;
		printf("%s ", question);
		(void)fgets(rval, 250, stdin);
	}
	*strchr(rval, '\n')=0;
	confirmclose=true;
#ifdef _WIN32
	if (filename && rval[0]=='"' && rval[2]==':')
	{
		char * rvalend=strchr(rval, '\0');
		if (rvalend[-1]=='"') rvalend[-1]='\0';
		return rval+1;
	}
#endif
	return rval;
}

static const char * requeststrfromuser(const char * question, bool filename, const char * defval)
{
	confirmclose=false;
	char * rval=(char*)malloc(256);
	*rval=0;
	printf("%s ", question);
	(void)fgets(rval, 250, stdin);
	*strchr(rval, '\n')=0;
	confirmclose=true;
	if (!*rval) return defval;
#ifdef _WIN32
	if (filename && rval[0]=='"' && rval[2]==':')
	{
		char * rvalend=strchr(rval, '\0');
		if (rvalend[-1]=='"') rvalend[-1]='\0';
		return rval+1;
	}
#endif
	return rval;
}

void libcon_init(int argc, char ** argv, const char * usage_)
{
	progname=argv[0];
	args=argv;
	argsleft=argc-1;
	usage=usage_;
	libcon_interactive=(!argsleft);
#if defined(_WIN32)
	if (libcon_interactive) atexit(libcon_pause);
#endif
}

const char * libcon_require(const char * desc)
{
	if (libcon_interactive) return requirestrfromuser(desc, false);
	else return getarg(true);
}

const char * libcon_require_filename(const char * desc)
{
	if (libcon_interactive) return requirestrfromuser(desc, true);
	else return getfname(true);
}

const char * libcon_optional(const char * desc, const char * defval)
{
	if (libcon_interactive) return requeststrfromuser(desc, false, defval);
	else return getarg(false, defval);
}

const char * libcon_optional_filename(const char * desc, const char * defval)
{
	if (libcon_interactive) return requeststrfromuser(desc, true, defval);
	else return getfname(false, defval);
}

const char * libcon_option()
{
	if (!libcon_interactive && argsleft && args[1][0]=='-') return getarg(false);
	return nullptr;
}

const char * libcon_option_value()
{
	if (!libcon_interactive) return getarg(false);
	return nullptr;
}

const char * libcon_question(const char * desc, const char * defval)
{
	if (libcon_interactive) return libcon_optional(desc, defval);
	return defval;
}

bool libcon_question_bool(const char * desc, bool defval)
{
	if (!libcon_interactive) return defval;
	while (true)
	{
		const char * answer=requeststrfromuser(desc, false, defval?"y":"n");
		if (!stricmp(answer, "y") || !stricmp(answer, "yes")) return true;
		if (!stricmp(answer, "n") || !stricmp(answer, "no")) return false;
	}
}

void libcon_end()
{
	if (!libcon_interactive && argsleft) libcon_badusage();
}
