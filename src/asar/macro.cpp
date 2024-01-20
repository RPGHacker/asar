#include "libstr.h"
#include "asar.h"
#include "autoarray.h"
#include "assocarr.h"
#include "errors.h"
#include "assembleblock.h"
#include "macro.h"
#include "asar_math.h"
#include "warnings.h"

assocarr<macrodata*> macros;
static string thisname;
static macrodata * thisone;
static int numlines;

int calledmacros;
int reallycalledmacros;
int macrorecursion;
bool inmacro;
int numvarargs;

macrodata* current_macro;
const char* const* current_macro_args;
int current_macro_numargs;

void startmacro(const char * line_)
{
	thisone= nullptr;
	if (!confirmqpar(line_)) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	string line=line_;
	clean_and_trim(line);
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
	thisone->variadic = false;
	thisone->fname= duplicate_string(thisfilename);
	thisone->startline=thisline;
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
	int prev_numvarargs = numvarargs;
	macrodata * thismacro;
	if (!confirmqpar(data)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	string line=data;
	clean_and_trim(line);
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
	if (*startpar) args=(const char* const*)qpsplit(startpar, ",", &numargs);
	if (numargs != thismacro->numargs && !thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_num_params);
	// RPG Hacker: -1, because the ... is also counted as an argument, yet we want it to be entirely optional.
	if (numargs < thismacro->numargs - 1 && thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_min_params);

	macrorecursion++;
	inmacro=true;
	int old_calledmacros = calledmacros;
	calledmacros = reallycalledmacros++;
	int startif=numif;

	// RPG Hacker: -1 to take the ... into account, which is also being counted.
	if(thismacro->variadic) numvarargs = numargs-(thismacro->numargs-1);
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

	macrodata* old_macro = current_macro;
	const char* const* old_macro_args = current_macro_args;
	int old_numargs = current_macro_numargs;
	current_macro = thismacro;
	current_macro_args = args;
	current_macro_numargs = numargs;

	for (int i=0;i<thismacro->numlines;i++)
	{
		try
		{
			thisfilename= thismacro->fname;
			thisline= thismacro->startline+i+1;
			thisblock= nullptr;
			string connectedline;
			int skiplines = getconnectedlines<autoarray<string> >(thismacro->lines, i, connectedline);
			//string out = replace_macro_args(connectedline); // done in assembleline
			int prevnumif = numif;
			assembleline(thismacro->fname, thismacro->startline+i, connectedline);
			i += skiplines;
			if ((numif != prevnumif || single_line_for_tracker == 3) && (whilestatus[numif].iswhile || whilestatus[numif].is_for) && whilestatus[numif].cond)
				i = whilestatus[numif].startline - thismacro->startline - 1;
		}
		catch(errline&){}
	}

	current_macro = old_macro;
	current_macro_args = old_macro_args;
	current_macro_numargs = old_numargs;

	macroposlabels = oldmacroposlabels;
	macroneglabels = oldmacroneglabels;
	macrosublabels = oldmacrosublabels;

	macrorecursion--;
	inmacro = macrorecursion;
	numvarargs = prev_numvarargs;
	calledmacros = old_calledmacros;
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
}

string replace_macro_args(const char* line) {
	string out;
	for (const char * in=line;*in;)
	{
		if (*in=='<' && in[1]=='<' && in[2] != ':')
		{
			out+="<<";
			in+=2;
		}
		else if (*in=='<')
		{
			const char * end=in+1;
			// RPG Hacker: Added checking for space here, because this code would consider
			// if a < b && a > c
			// a macro arg expansion. In practice, this is still a sloppy solution and is
			// likely to fail in some edge case I can't think of right now. Should parse
			// this in a much more robust way at some point...
			if (*end==' ')
			{
				out += *(in++);
				continue;
			}

			while (*end && *end!='>'&& *end!='<' && *(end+1)!=':') end++; //allow for conditionals and <:
			if (*end!='>')
			{
				out+=*(in++);
				continue;
			}
					
			bool proper_variadic = false;
			if (in[1] == '.' && in[2] == '.' && in[3] == '.' && in[4] == '[')
			{
				if (end[-1] != ']')
					asar_throw_error(0, error_type_block, error_id_unclosed_vararg);
							
				proper_variadic = true;
				in += 4;
				end--;
			}

			if(!inmacro) asar_throw_error(0, error_type_block, error_id_macro_param_outside_macro);
			//*end=0;
			in++;
			string param;
			string temp(in, end-in);
			resolvedefines(param, temp);
			in = param.data();
			bool valid_named_param = confirmname(in);
			if (!valid_named_param && !current_macro->variadic) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
			bool found=false;
			for (int j=0;current_macro->arguments[j]&&!proper_variadic;j++)
			{
				if (!strcmp(in, current_macro->arguments[j]))
				{
					found=true;
					if (current_macro_args[j][0]=='"')
					{
						string s=current_macro_args[j];
						out+=safedequote(s.temp_raw());
					}
					else out+=current_macro_args[j];
					break;
				}
			}
			if (!found)
			{
				snes_label ret;
				if(valid_named_param && !current_macro->variadic)
				{
					if (proper_variadic) asar_throw_error(0, error_type_block, error_id_invalid_vararg, in);
					else asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
				}
				if(current_macro->variadic && valid_named_param && !labelval(in, &ret, false))
				{
					if (proper_variadic) asar_throw_error(0, error_type_block, error_id_invalid_vararg, in);
					else asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
				}
				if(!proper_variadic) asar_throw_warning(0, warning_id_feature_deprecated, "'<math>' syntax for variadic macro parameters", "Use '<...[math]>' instead.");
				int arg_num = getnum(in);

				if(forwardlabel) asar_throw_error(0, error_type_block, error_id_label_forward);

				if(numif<=numtrue){
					if (arg_num < 0) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds);
					if (arg_num > current_macro_numargs-current_macro->numargs) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds);
					if (current_macro_args[arg_num+current_macro->numargs-1][0]=='"')
					{
						string s=current_macro_args[arg_num+current_macro->numargs-1];
						out+=safedequote(s.temp_raw());
					}
					else out+=current_macro_args[arg_num+current_macro->numargs-1];
				}
			}
			in=end+1;
			if (proper_variadic) in++;
		}
		else out+=*(in++);
	}
	return out;
}
