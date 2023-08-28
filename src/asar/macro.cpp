#include "asar.h"
#include "assembleblock.h"
#include "macro.h"
#include "asar_math.h"
#include "warnings.h"

assocarr<macrodata*> macros;
string defining_macro_name;
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
	line.qnormalize();
	char * startpar=(char *)strchr(line.data(), '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_invalid_macro_name);
	defining_macro_name=line;
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
	if (macros.exists(defining_macro_name)) asar_throw_error(0, error_type_block, error_id_macro_redefined, defining_macro_name.data());
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
	thisone->fname= duplicate_string(get_current_file_name());
	thisone->startline=get_current_line();
	thisone->parent_macro=current_macro;
	thisone->parent_macro_num_varargs=0;
	// RPG Hacker: -1 to take the ... into account, which is also being counted.
	if (thisone->parent_macro != nullptr) thisone->parent_macro_num_varargs = current_macro_numargs-(current_macro->numargs-1);
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
	if (insert) macros.create(defining_macro_name) = thisone;
	else
	{
		freemacro(thisone);
		thisone=nullptr;
	}
}

#define cfree(x) free((void*)x)
void freemacro(macrodata* & macro)
{
	macro->lines.~autoarray();
	cfree(macro->fname);
	cfree(macro->arguments_buffer);
	cfree(macro->arguments);
	cfree(macro);
}
#undef cfree


void callmacro(const char * data)
{
	int prev_numvarargs = numvarargs;
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
	if (numargs != thismacro->numargs && !thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_num_params);
	// RPG Hacker: -1, because the ... is also counted as an argument, yet we want it to be entirely optional.
	if (numargs < thismacro->numargs - 1 && thismacro->variadic) asar_throw_error(1, error_type_block, error_id_macro_wrong_min_params);

	macrorecursion++;
	inmacro=true;
	int old_calledmacros = calledmacros;
	calledmacros = reallycalledmacros++;
	int startif=numif;

	for (int i = 0; i < numargs; ++i)
	{
		// RPG Hacker: These casts make me feel very nasty.
		(*reinterpret_cast<autoptr<const char**>*>(&args))[i] = safedequote(strip_whitespace((char*)args[i]));
	}

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

	callstack_push cs_push(callstack_entry_type::MACRO_CALL, data);

	{
		callstack_push cs_push(callstack_entry_type::FILE, thismacro->fname);

		for (int i=0;i<thismacro->numlines;i++)
		{
			bool was_loop_end = do_line_logic(thismacro->lines[i], thismacro->fname, thismacro->startline+i+1);

			if (was_loop_end && whilestatus[numif].cond)
				// RPG Hacker: -1 to compensate for the i++, and another -1
				// because ->lines doesn't include the macro header.
				i = whilestatus[numif].startline - thismacro->startline - 2;
		}
	}

	macroposlabels = oldmacroposlabels;
	macroneglabels = oldmacroneglabels;
	macrosublabels = oldmacrosublabels;

	current_macro = old_macro;
	current_macro_args = old_macro_args;
	current_macro_numargs = old_numargs;

	macrorecursion--;
	inmacro = macrorecursion;
	numvarargs = prev_numvarargs;
	calledmacros = old_calledmacros;
	if (numif!=startif)
	{
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_block, error_id_unclosed_if);
	}
}

string generate_macro_arg_string(const char* named_arg, int depth)
{
	string ret="<";
	for (int i = 0; i < depth;++i)
	{
		ret += '^';
	}
	ret += named_arg;
	ret += ">";
	return ret;
}

string generate_macro_arg_string(int var_arg, int depth)
{
	string ret="<";
	for (int i = 0; i < depth;++i)
	{
		ret += '^';
	}
	ret += dec(var_arg);
	ret += ">";
	return ret;
}

string generate_macro_hint_string(const char* named_arg, const macrodata* thismacro, int desired_depth, int current_depth=0)
{
	// RPG Hacker: This only work when the incorrectly used parameter
	// is inside the macro that is currently being defined. Not great,
	// but still better than nothing.
	if (current_depth == 0 && thisone != nullptr)
	{
		for (int j=0;thisone->arguments[j];j++)
		{
			if (!strcmp(named_arg, thisone->arguments[j]))
			{
				string ret=" Did you mean: '";
				ret += generate_macro_arg_string(thisone->arguments[j], 0);
				ret += "'?";
				return ret;
			}
		}
	}

	// RPG Hacker: Technically, we could skip a level here and go straight
	// to the parent, but maybe at some point we'll want to expand this to
	// also look for similar args in the current level, so I'll leave it
	// like this, just in case.
	if (thismacro != nullptr)
	{
		for (int j=0;thismacro->arguments[j];j++)
		{
			if (!strcmp(named_arg, thismacro->arguments[j]))
			{
				string ret=" Did you mean: '";
				ret += generate_macro_arg_string(thismacro->arguments[j], desired_depth+current_depth);
				ret += "'?";
				return ret;
			}
		}
		return generate_macro_hint_string(named_arg, thismacro->parent_macro, desired_depth, current_depth+1);
	}

	return "";
}

string generate_macro_hint_string(int var_arg, const macrodata* thismacro, int desired_depth, int current_depth=0)
{
	if (thismacro != nullptr)
	{
		if (thismacro->parent_macro_num_varargs > var_arg)
		{
			string ret=" Did you mean: '";
			ret += generate_macro_arg_string(var_arg, desired_depth+current_depth+1);
			ret += "'?";
			return ret;
		}
		return generate_macro_hint_string(var_arg, thismacro->parent_macro, desired_depth, current_depth+1);
	}

	return "";
}

string replace_macro_args(const char* line) {
	string out;
	if(!inmacro)
	{
		out += line;
		return out;
	}
	for (const char * in=line;*in;)
	{
		if (*in=='<' && in[1]=='<' && in[2] != ':')
		{
			if (in[2] == '^')
			{
				out+="<";
				in+=1;
			}
			else
			{
				out+="<<";
				in+=2;
			}
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

			int depth = 0;
			for (const char* depth_str = in+1; *depth_str=='^'; depth_str++)
			{
				depth++;
			}

			if (depth != in_macro_def)
			{
				string temp(in, end-in+1);
				out+=temp;
				in=end+1;
				if (depth > in_macro_def)
				{
					if (in_macro_def > 0) asar_throw_error(0, error_type_line, error_id_invalid_depth_resolve, "macro parameter", "macro parameter", depth, in_macro_def-1);
					//else asar_throw_error(0, error_type_block, error_id_macro_param_outside_macro);
				}
				continue;
			}

			if (depth > 0 && !inmacro) asar_throw_error(0, error_type_line, error_id_invalid_depth_resolve, "macro parameter", "macro parameter", depth, in_macro_def-1);
			in += depth+1;

			bool is_variadic_arg = false;
			if (in[0] == '.' && in[1] == '.' && in[2] == '.' && in[3] == '[')
			{
				if (end[-1] != ']')
					asar_throw_error(0, error_type_block, error_id_unclosed_vararg);

				is_variadic_arg = true;
				in += 4;
				end--;
			}

			//if(!inmacro) asar_throw_error(0, error_type_block, error_id_macro_param_outside_macro);
			if(is_variadic_arg && !current_macro->variadic) asar_throw_error(0, error_type_block, error_id_macro_not_varadic, "<...[math]>");
			//*end=0;
			string param;
			string temp(in, end-in);
			resolvedefines(param, temp);
			in = param.data();
			bool valid_named_param = confirmname(in);
			if (!is_variadic_arg)
			{
				if (!valid_named_param) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
				bool found=false;
				for (int j=0;current_macro->arguments[j];j++)
				{
					if (!strcmp(in, current_macro->arguments[j]))
					{
						found=true;
						out+=current_macro_args[j];
						break;
					}
				}
				if (!found)
				{
					asar_throw_error(0, error_type_block, error_id_macro_param_not_found, generate_macro_arg_string(in, depth).raw(), generate_macro_hint_string(in, current_macro, depth).raw());
				}
			}
			else
			{
				snes_label ret;
				if(valid_named_param && !labelval(in, &ret, false)) asar_throw_error(0, error_type_block, error_id_invalid_vararg, in);
				int arg_num = getnum(in);

				if(forwardlabel) asar_throw_error(0, error_type_block, error_id_label_forward);

				if (arg_num < 0) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds, generate_macro_arg_string(arg_num, depth).raw(), "");
				if (arg_num > current_macro_numargs-current_macro->numargs) asar_throw_error(1, error_type_block, error_id_vararg_out_of_bounds, generate_macro_arg_string(arg_num, depth).raw(), generate_macro_hint_string(arg_num, current_macro, depth).raw());
				out+=current_macro_args[arg_num+current_macro->numargs-1];
			}
			in=end+1;
			if (is_variadic_arg) in++;
		}
		else out+=*(in++);
	}
	return out;
}
