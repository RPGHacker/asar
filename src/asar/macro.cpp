#include "libstr.h"
#include "asar.h"
#include "autoarray.h"
#include "assocarr.h"
#include "errors.h"
#include "assembleblock.h"
#include "macro.h"

assocarr<macrodata*> macros;
static string thisname;
static macrodata * thisone;
static int numlines;

int reallycalledmacros;
int calledmacros;
int macrorecursion;
bool inmacro;
int numvarargs;

void startmacro(const char * line_)
{
	thisone= nullptr;
	if (!confirmqpar(line_)) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	string line=line_;
	clean(line);
	char * startpar=strqchr(line.data(), '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_invalid_macro_name);
	thisname=line;
	char * endpar=strqrchr(startpar, ')');
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nullptrs
	if (endpar[1]) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*endpar=0;
	for (int i=0;startpar[i];i++)
	{
		char c=startpar[i];
		if (!is_alnum(c) && c!='_' && c!=','&& c!='.') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
		if (c==',' && is_digit(startpar[i+1])) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	}
	if (*startpar==',' || is_digit(*startpar) || strstr(startpar, ",,") || endpar[-1]==',') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	if (macros.exists(thisname)) asar_throw_error(0, error_type_block, error_id_macro_redefined, thisname.data());
	thisone=(macrodata*)malloc(sizeof(macrodata));
	new(thisone) macrodata;
	if (*startpar)
	{
		thisone->arguments=(const char* const*)qpsplit(duplicate_string(startpar), ",", &thisone->numargs);
	}
	else
	{
		const char ** noargs=(const char**)malloc(sizeof(const char**));
		*noargs=nullptr;
		thisone->arguments=noargs;
		thisone->numargs=0;
	}
	for (int i=0;thisone->arguments[i];i++)
	{
		if(!strcmp(thisone->arguments[i], "...") && !thisone->arguments[i+1]) thisone->variadic = true;
		else if(!strcmp(thisone->arguments[i], "...")) asar_throw_error(0, error_type_block, error_id_vararg_must_be_last);
		else if(strchr(thisone->arguments[i], '.')) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
		else if (!confirmname(thisone->arguments[i])) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
		for (int j=i+1;thisone->arguments[j];j++)
		{
			if (!strcmp(thisone->arguments[i], thisone->arguments[j])) asar_throw_error(0, error_type_block, error_id_macro_param_redefined, thisone->arguments[i]);
		}
	}
	thisone->fname= duplicate_string(thisfilename);
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
	if (insert) macros.create(thisname) = thisone;
	else delete thisone;
}


void callmacro(const char * data)
{
	inmacro=true;
	int numcm=reallycalledmacros++;
	macrodata * thismacro;
	if (!confirmqpar(data)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	string line=data;
	clean(line);
	char * startpar=strqchr(line.data(), '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	if (!macros.exists(line)) asar_throw_error(0, error_type_block, error_id_macro_not_found, line.data());
	thismacro = macros.find(line);
	char * endpar=strqrchr(startpar, ')');
	if (endpar[1]) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	*endpar=0;
	autoptr<const char * const*> args;
	int numargs=0;
	if (*startpar) args=(const char* const*)qpsplit(duplicate_string(startpar), ",", &numargs);
	if (numargs != thismacro->numargs && !thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_num_params);
	if (numargs < thismacro->numargs && thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_min_params);
	macrorecursion++;
	int startif=numif;
	
	if(thismacro->variadic) numvarargs = numargs-thismacro->numargs;
	else numvarargs = -1;

	autoarray<int>* oldmacroposlabels = macroposlabels;
	autoarray<int>* oldmacroneglabels = macroneglabels;
	autoarray<string>* oldmacrosublabels = macrosublabels;

	autoarray<int> newmacroposlabels;
	autoarray<int> newmacroneglabels;
	autoarray<string> newmacrosublabels;

	macroposlabels = &newmacroposlabels;
	macroneglabels = &newmacroneglabels;
	macrosublabels = &newmacrosublabels;

	for (int i=0;i<thismacro->numlines;i++)
	{
		try
		{
			thisfilename= thismacro->fname;
			thisline= thismacro->startline+i+1;
			thisblock= nullptr;
			string out;
			string connectedline;
			int skiplines = getconnectedlines<autoarray<string> >(thismacro->lines, i, connectedline);
			string intmp;
			if(thismacro->variadic) resolvedefines(intmp, connectedline.data());
			else intmp = connectedline;
			for (char * in=intmp.temp_raw();*in;)
			{
				if (*in=='<' && in[1]=='<')
				{
					out+="<<";
					in+=2;
				}
				else if (thismacro->variadic && *in=='<' && (is_digit(in[1]) || in[1] == '-'))
				{
					char * end=in+2;
					while (is_digit(*end)) end++;
					if (*end!='>')
					{
						out+=*(in++);
						continue;
					}
					*end=0;
					in++;
					int arg_num = strtol(in, nullptr, 10);
					
					if(numif<=numtrue){
						if (arg_num < 0) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds);
						if (arg_num > numargs-thismacro->numargs) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds);
						if (args[arg_num+thismacro->numargs-1][0]=='"')
						{
							string s=args[arg_num+thismacro->numargs-1];
							out+=safedequote(s.temp_raw());
						}
						else out+=args[arg_num+thismacro->numargs-1];
					}
					in=end+1;
					
				}
				else if (*in=='<' && is_alnum(in[1]))
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
					if (!confirmname(in)) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
					bool found=false;
					for (int j=0;thismacro->arguments[j];j++)
					{
						if (!strcmp(in, thismacro->arguments[j]))
						{
							found=true;
							if (args[j][0]=='"')
							{
								string s=args[j];
								out+=safedequote(s.temp_raw());
							}
							else out+=args[j];
							break;
						}
					}
					if (!found) asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
					in=end+1;
				}
				else out+=*(in++);
			}
			calledmacros = numcm;
			int prevnumif = numif;
			assembleline(thismacro->fname, thismacro->startline+i, out);
			i += skiplines;
			if (numif != prevnumif && whilestatus[numif].iswhile && whilestatus[numif].cond)
				i = whilestatus[numif].startline - thismacro->startline - 1;
		}
		catch(errline&){}
	}

	macroposlabels = oldmacroposlabels;
	macroneglabels = oldmacroneglabels;
	macrosublabels = oldmacrosublabels;

	macrorecursion--;
	if (repeatnext!=1)
	{
		thisblock= nullptr;
		repeatnext=1;
		asar_throw_error(0, error_type_block, error_id_rep_at_macro_end);
	}
	if (numif!=startif)
	{
		thisblock= nullptr;
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_block, error_id_unclosed_if);
	}
	inmacro = false;
}
