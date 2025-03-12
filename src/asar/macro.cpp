#include "asar.h"
#include "assembleblock.h"
#include "macro.h"
#include "asar_math.h"

assocarr<macrodata*> macros;
static string defining_macro_name;
static macrodata * thisone;
static int numlines;

int calledmacros;
int reallycalledmacros;
int macrorecursion;
bool inmacro;
int numvarargs;

static macrodata* current_macro;
static const char* const* current_macro_args;
static int current_macro_numargs;

void startmacro(const char * line_)
{
	thisone= nullptr;
	if (!confirmqpar(line_)) throw_err_block(0, err_broken_macro_declaration);
	string line=line_;
	line.qnormalize();
	char * startpar=(char *)strchr(line.data(), '(');
	if (!startpar) throw_err_block(0, err_broken_macro_declaration);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) throw_err_block(0, err_invalid_macro_name);
	defining_macro_name=(const char*)line; // force strcpy, line's .length() is wrong right now
	char * endpar=startpar+strlen(startpar)-1;
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nullptrs
	if (*endpar != ')') throw_err_block(0, err_broken_macro_declaration);
	*endpar=0;
	for (int i=0;startpar[i];i++)
	{
		char c=startpar[i];
		if (!is_ualnum(c)&& c!=','&& c!='.'&& c!=' ') throw_err_block(0, err_broken_macro_declaration);
		if (c==',' && is_digit(startpar[i+1])) throw_err_block(0, err_broken_macro_declaration);
	}
	if (*startpar==',' || is_digit(*startpar) || strstr(startpar, ",,") || endpar[-1]==',') throw_err_block(0, err_broken_macro_declaration);
	if (macros.exists(defining_macro_name))
	{
		const auto macro = macros[defining_macro_name];
		// i think theoretically it would be "more correct" to print the entire
		// callstack here, but this should be good enough for an error message.
		// needs +1 because startline is 0-indexed
		throw_err_block(0, err_macro_redefined, defining_macro_name.data(), macro->fname, macro->startline + 1);
	}
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
		else if(!strcmp(thisone->arguments[i], "...")) throw_err_block(0, err_vararg_must_be_last);
		else if(strchr(thisone->arguments[i], '.')) throw_err_block(0, err_invalid_macro_param_name);
		else if (!confirmname(thisone->arguments[i])) throw_err_block(0, err_invalid_macro_param_name);
		for (int j=i+1;thisone->arguments[j];j++)
		{
			if (!strcmp(thisone->arguments[i], thisone->arguments[j])) throw_err_block(0, err_macro_param_redefined, thisone->arguments[i]);
		}
	}
	numlines=0;
}

void tomacro(string line)
{
	if (!thisone) return;
	thisone->lines[numlines++]=std::move(line);
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
	if (!confirmqpar(data)) throw_err_block(0, err_broken_macro_usage);
	string line=data;
	line.qnormalize();
	char * startpar=(char *)strchr(line.data(), '(');
	if (!startpar) throw_err_block(0, err_broken_macro_usage);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) throw_err_block(0, err_broken_macro_usage);
	if (!macros.exists(line)) throw_err_block(0, err_macro_not_found, line.data());
	thismacro = macros.find(line);
	char * endpar=startpar+strlen(startpar)-1;
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nullptrs
	if (*endpar != ')') throw_err_block(0, err_broken_macro_usage);
	*endpar=0;
	autoptr<const char * const*> args;
	int numargs=0;
	if (*startpar) {
		args=(const char* const*)qpsplit(startpar, ',', &numargs);
		// qpsplit returns a nullptr when the input is broken, e.g. closing paren before opening or whatnot
		if(args == nullptr) throw_err_block(0, err_broken_macro_usage);
	}
	if (numargs != thismacro->numargs && !thismacro->variadic) throw_err_block(0, err_macro_wrong_num_params);
	// RPG Hacker: -1, because the ... is also counted as an argument, yet we want it to be entirely optional.
	if (numargs < thismacro->numargs - 1 && thismacro->variadic) throw_err_block(0, err_macro_wrong_min_params);

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
		throw_err_block(0, err_unclosed_if);
	}
}

static string generate_macro_arg_string(const char* named_arg, int depth)
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

static string generate_macro_arg_string(int var_arg, int depth)
{
	string ret="<";
	for (int i = 0; i < depth;++i)
	{
		ret += '^';
	}
	ret += "...[";
	ret += dec(var_arg);
	ret += "]>";
	return ret;
}

static string generate_macro_hint_string(const char* named_arg, const macrodata* thismacro, int desired_depth, int current_depth=0)
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

static string generate_macro_hint_string(int var_arg, const macrodata* thismacro, int desired_depth, int current_depth=0)
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

string replace_macro_args(const string& line) {
	if(!inmacro)
	{
		return line;
	}
	string out;
	for (const char * in=line;*in;)
	{
		if (*in != '<') {
			out += *in++;
			continue;
		}

		const char * end=in+1;
		int depth = 0;
		for (; *end=='^'; end++) {
			depth++;
		}

		const char* name_start = end;
		bool is_variadic = false;

		if(is_ualpha(*end)) {
			// must be a named arg ref
			while(is_ualnum(*end)) end++;
			if(*end != '>') {
				// not a valid param ref
				out += *in++;
				continue;
			}
		} else if(end[0] == '.' && end[1] == '.' && end[2] == '.' && end[3] == '[') {
			// must be a <...[something]>
			end += 4;
			end = strqpchr(end, ']');
			if(!end || end[1] != '>') throw_err_line(0, err_unclosed_vararg);
			end++;
			is_variadic = true;
		} else {
			// not a macro param
			out += *in++;
			continue;
		}

		// end is now pointing at '>'
		if (depth != in_macro_def)
		{
			if (depth > in_macro_def && in_macro_def > 0) {
				throw_err_line(0, err_invalid_depth_resolve, "macro parameter", "macro parameter", depth, in_macro_def-1);
			}
			// valid param ref, but not meant to be resolved at this depth.
			// copy it over in its entirety (including the last '>')
			out.append(in, 0, end + 1 - in);
			in = end + 1;
			continue;
		}

		if (depth > 0 && !inmacro) {
			throw_err_line(0, err_invalid_depth_resolve, "macro parameter", "macro parameter", depth, in_macro_def-1);
		}

		if(is_variadic) {
			if(!current_macro->variadic) throw_err_block(0, err_macro_not_varadic, "<...[math]>");
			const char* num_start = name_start + 4;
			const char* num_end = end - 1;
			string num_str(num_start, num_end - num_start);
			string num_resolved;
			resolvedefines(num_resolved, num_str);
			int arg_num = parse_math_expr(num_resolved)->evaluate_static().get_integer();
			if (arg_num < 0) {
				throw_err_block(0, err_vararg_out_of_bounds, generate_macro_arg_string(arg_num, depth).data(), "");
			}
			if (arg_num > current_macro_numargs-current_macro->numargs) {
				string argstr = generate_macro_arg_string(arg_num, depth);
				string hintstr = generate_macro_hint_string(arg_num, current_macro, depth);
				throw_err_block(0, err_vararg_out_of_bounds, argstr.data(), hintstr.data());
			}
			out += current_macro_args[arg_num + current_macro->numargs - 1];
		} else {
			string name(name_start, end - name_start);
			bool found = false;
			for (int j=0; current_macro->arguments[j]; j++) {
				if(name == current_macro->arguments[j]) {
					found = true;
					out += current_macro_args[j];
					break;
				}
			}
			if (!found) {
				string argstr = generate_macro_arg_string(name, depth);
				string hintstr = generate_macro_hint_string(name, current_macro, depth);
				throw_err_block(0, err_macro_param_not_found, argstr.data(), hintstr.data());
			}
		}
		in = end+1;
	}
	return out;
}
