#include "asar.h"
#include "assembleblock.h"
#include "macro.h"
#include "asar_math.h"

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
	line.qnormalize();
	char * startpar=(char *)strchr(line.data(), '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_invalid_macro_name);
	thisname=line;
	char * endpar=startpar+strlen(startpar)-1;
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nullptrs
	if (*endpar != ')') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*endpar=0;
	for (int i=0;startpar[i];i++)
	{
		char c=startpar[i];
		if (!is_ualnum(c)&& c!=','&& c!='.'&& c!=' ') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
		if (c==',' && is_digit(startpar[i+1])) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	}
	if (*startpar==',' || is_digit(*startpar) || strstr(startpar, ",,") || endpar[-1]==',') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	if (macros.exists(thisname)) asar_throw_error(0, error_type_block, error_id_macro_redefined, thisname.data());
	thisone=(macrodata*)malloc(sizeof(macrodata));
	new(thisone) macrodata;
	if (*startpar)
	{
		char **arguments = split(duplicate_string(startpar), ',', &thisone->numargs);
		thisone->arguments_buffer = arguments[0];
		for (int i=0;arguments[i];i++)
		{
			arguments[i] = strip_whitespace(arguments[i]);
		}
		thisone->arguments=(const char* const*)arguments;
	}
	else
	{
		const char ** noargs=(const char**)malloc(sizeof(const char**));
		*noargs=nullptr;
		thisone->arguments=noargs;
		thisone->arguments_buffer = nullptr;
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
	inmacro=true;
	int numcm=reallycalledmacros++;
	macrodata * thismacro;
	if (!confirmqpar(data)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	string line=data;
	line.qnormalize();
	char * startpar=(char *)strchr(line.data(), '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	if (!macros.exists(line)) asar_throw_error(0, error_type_block, error_id_macro_not_found, line.data());
	thismacro = macros.find(line);
	char * endpar=startpar+strlen(startpar)-1;
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nullptrs
	if (*endpar != ')') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*endpar=0;
	autoptr<const char * const*> args;
	int numargs=0;
	if (*startpar) args=(const char* const*)qpsplit(startpar, ',', &numargs);
	if (numargs != thismacro->numargs && !thismacro->variadic) asar_throw_error(0, error_type_block, error_id_macro_wrong_num_params);
	// RPG Hacker: -1, because the ... is also counted as an argument, yet we want it to be entirely optional.
	if (numargs < thismacro->numargs - 1 && thismacro->variadic) asar_throw_error(0, error_type_block, error_id_macro_wrong_min_params);
	macrorecursion++;
	int startif=numif;

	// RPG Hacker: -1 to take the ... into account, which is also being counted.
	if(thismacro->variadic) numvarargs = numargs-(thismacro->numargs-1);
	else numvarargs = -1;
	new_internal_block(INTERNAL_COMMAND_NUMVARARGS).params.append(numvarargs);
	new_internal_block(INTERNAL_COMMAND_FILENAME).params.append(thismacro->fname);
	string prevthisfilename = thisfilename;

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
			string intmp;
			int skiplines = getconnectedlines<autoarray<string> >(thismacro->lines, i, intmp);
			for (char * in=intmp.temp_raw();*in;)
			{
				if (*in=='<' && in[1]=='<' && in[2] != ':')
				{
					out+="<<";
					in+=2;
				}
				else if (*in=='<')
				{
					char * end=in+1;
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

					*end=0;
					in++;
					string param;
					resolvedefines(param, in);
					in = param.temp_raw();
					bool valid_named_param = confirmname(in);
					if (!valid_named_param && !thismacro->variadic) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
					bool found=false;
					for (int j=0;thismacro->arguments[j];j++)
					{
						if (!strcmp(in, thismacro->arguments[j]))
						{
							found=true;
							out+=(char *)safedequote(strip_whitespace((char *)args[j]));
							break;
						}
					}
					if (!found)
					{
						snes_label ret;
						if(valid_named_param  && !thismacro->variadic) asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
						if(thismacro->variadic && valid_named_param && !labelval(in, &ret, false))  asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
						int arg_num = getnum(in);

						if(forwardlabel) asar_throw_error(0, error_type_block, error_id_label_forward);
						//conditionals deserve all my hate
						if(numif==numtrue || (numif==numtrue+1 && stribegin(out.data(), "elseif "))){
							if (arg_num < 0) asar_throw_error(0, error_type_block, error_id_vararg_out_of_bounds);
							if (arg_num > numargs-thismacro->numargs) asar_throw_error(0, error_type_block, error_id_vararg_out_of_bounds);
							out+=(char *)safedequote(strip_whitespace((char *)args[arg_num+thismacro->numargs-1]));
						}
					}
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
	if (numif!=startif)
	{
		thisblock= nullptr;
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_block, error_id_unclosed_if);
	}
	inmacro = macrorecursion;
	numvarargs = prev_numvarargs;
	new_internal_block(INTERNAL_COMMAND_NUMVARARGS).params.append(numvarargs);
	new_internal_block(INTERNAL_COMMAND_FILENAME).params.append(prevthisfilename);
}
