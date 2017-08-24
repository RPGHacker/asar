#include "libstr.h"
#include "asar.h"
#include "autoarray.h"
#include "scapegoat.hpp"

template<typename t> void error(int pass, const char * e_);
bool confirmname(const char * name);

struct macrodata
{
autoarray<string> lines;
int numlines;
int startline;
const char * fname;
const char ** arguments;
int numargs;
};

lightweight_map<string, macrodata*> macros;
string thisname;
macrodata * thisone;
int numlines;

extern const char * thisfilename;
extern int thisline;
extern const char * thisblock;

extern autoarray<whiletracker> whilestatus;

void assembleline(const char * fname, int linenum, const char * line);

int reallycalledmacros;
int calledmacros;
int macrorecursion;

extern int repeatnext;
extern int numif;
extern int numtrue;

extern const char * callerfilename;
extern int callerline;

void startmacro(const char * line_)
{
	thisone=NULL;
	if (!confirmqpar(line_)) error<errblock>(0, "Broken macro declaration");
	string line=line_;
	clean(line);
	char * startpar=strqchr(line.str, '(');
	if (!startpar) error<errblock>(0, "Broken macro declaration");
	*startpar=0;
	startpar++;
	if (!confirmname(line)) error<errblock>(0, "Bad macro name");
	thisname=line;
	char * endpar=strqrchr(startpar, ')');
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nulls
	if (endpar[1]) error<errblock>(0, "Broken macro declaration");
	*endpar=0;
	for (int i=0;startpar[i];i++)
	{
		char c=startpar[i];
		if (!isalnum(c) && c!='_' && c!=',') error<errline>(0, "Broken macro declaration");
		if (c==',' && isdigit(startpar[i+1])) error<errline>(0, "Broken macro declaration");
	}
	if (*startpar==',' || isdigit(*startpar) || strstr(startpar, ",,") || endpar[-1]==',') error<errline>(0, "Broken macro declaration");
	macrodata * ignored;
	if (macros.find(thisname, ignored)) error<errblock>(0, "Duplicate macro");
	thisone=(macrodata*)malloc(sizeof(macrodata));
	new(thisone) macrodata;
	if (*startpar)
	{
		thisone->arguments=(const char**)qpsplit(strdup(startpar), ",", &thisone->numargs);
	}
	else
	{
		const char ** noargs=(const char**)malloc(sizeof(const char**));
		*noargs=NULL;
		thisone->arguments=noargs;
		thisone->numargs=0;
	}
	for (int i=0;thisone->arguments[i];i++)
	{
		if (!confirmname(thisone->arguments[i])) error<errblock>(0, "Bad macro argument name");
		for (int j=i+1;thisone->arguments[j];j++)
		{
			if (!strcmp(thisone->arguments[i], thisone->arguments[j])) error<errblock>(0, S"Duplicate macro argument '"+thisone->arguments[i]+"'");
		}
	}
	thisone->fname=strdup(thisfilename);
	thisone->startline=thisline;
	numlines=0;
}

void tomacro(const char * line)
{
	if (!thisone) return;
	thisone->lines[numlines++]=line;
}

void endmacro(bool insert)
{
	if (!thisone) return;
	thisone->numlines=numlines;
	if (insert) macros.insert(thisname, thisone);
	else delete thisone;
}

#define merror(str) do { if (!macrorecursion) { callerfilename=NULL; callerline=-1; } error<errblock>(0, str); } while(0)

void callmacro(const char * data)
{
	int numcm=reallycalledmacros++;
	macrodata * thisone;
	if (!confirmqpar(data)) merror("Broken macro usage");
	string line=data;
	clean(line);
	char * startpar=strqchr(line.str, '(');
	if (!startpar) merror("Broken macro usage");
	*startpar=0;
	startpar++;
	if (!confirmname(line)) merror("Bad macro name");
	if (!macros.find(line, thisone)) merror("Unknown macro");
	char * endpar=strqrchr(startpar, ')');
	if (endpar[1]) merror("Broken macro usage");
	*endpar=0;
	autoptr<const char **> args;
	int numargs=0;
	if (*startpar) args=(const char**)qpsplit(strdup(startpar), ",", &numargs);
	if (numargs!=thisone->numargs) merror("Wrong number of arguments to macro");
	macrorecursion++;
	int startif=numif;
	for (int i=0;i<thisone->numlines;i++)
	{
		try
		{
			thisfilename=thisone->fname;
			thisline=thisone->startline+i+1;
			thisblock=NULL;
			string out;
			string connectedline;
			int skiplines = getconnectedlines<autoarray<string> >(thisone->lines, i, connectedline);
			string intmp = connectedline;
			for (char * in=intmp.str;*in;)
			{
				if (*in=='<' && in[1]=='<')
				{
					out+="<<";
					in+=2;
				}
				else if (*in=='<' && isalnum(in[1]))
				{
					char * end=in+1;
					while (*end && *end!='<' && *end!='>') end++;
					if (*end!='>')
					{
						out+=*(in++);
						continue;
					}
					*end=0;
					in++;
					if (!confirmname(in)) error<errline>(0, "Broken macro contents");
					bool found=false;
					for (int i=0;thisone->arguments[i];i++)
					{
						if (!strcmp(in, thisone->arguments[i]))
						{
							found=true;
							if (args[i][0]=='"')
							{
								string s=args[i];
								out+=dequote(s.str);
							}
							else out+=args[i];
							break;
						}
					}
					if (!found) error<errline>(0, "Unknown macro argument");
					in=end+1;
				}
				else out+=*(in++);
			}
			calledmacros = numcm;
			int prevnumif = numif;
			assembleline(thisone->fname, thisone->startline+i, out);
			i += skiplines;
			if (numif != prevnumif && whilestatus[numif].iswhile && whilestatus[numif].cond)
				i = whilestatus[numif].startline - thisone->startline - 1;
		}
		catch(errline&){}
	}
	macrorecursion--;
	if (repeatnext!=1)
	{
		thisblock=NULL;
		repeatnext=1;
		merror("rep or if at the end of a macro");
	}
	if (numif!=startif)
	{
		thisblock=NULL;
		numif=startif;
		numtrue=startif;
		merror("Unclosed if statement");
	}
}
