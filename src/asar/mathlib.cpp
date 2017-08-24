#if 0
//This file is not used by Asar, it's just included as a smarter alternative to editing math.cpp if
//you want it for your own projects (unless, of course, you need Asar's highly specialized features for some weird reason).
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "autoarray.h"

bool math_pri=true;
bool math_round=false;
bool math_xorexp=false;

#define error(str) throw str
static const char * str;

static long double getnumcore();
static long double getnum();
static long double eval(int depth);

static long double getnumcore()
{
	if (*str=='(')
	{
		str++;
		long double rval=eval(0);
		if (*str!=')') error("Mismatched parentheses.");
		str++;
		return rval;
	}
	if (*str=='$')
	{
		if (!isxdigit(str[1])) error("Invalid hex value.");
		if (tolower(str[2])=='x') return -42;//let str get an invalid value so it'll throw an invalid operator later on
		return strtoul(str+1, (char**)&str, 16);
	}
	if (*str=='%')
	{
		if (str[1]!='0' && str[1]!='1') error("Invalid binary value.");
		return strtoul(str+1, (char**)&str, 2);
	}
	if (isdigit(*str) || *str=='.')
	{
		return strtod(str, (char**)&str);
	}
	if (isalpha(*str) || *str<0 || *str>127)
	{
		const char * start=str;
		while (isalnum(*str) || *str=='_' || *str<0 || *str>127) str++;
		int len=str-start;
		while (*str==' ') str++;
		if (*str=='(')
		{
			str++;
			while (*str==' ') str++;
			autoarray<long double> params;
			int numparams=0;
			if (*str!=')')
			{
				while (true)
				{
					while (*str==' ') str++;
					params[numparams++]=eval(0);
					while (*str==' ') str++;
					if (*str==',')
					{
						str++;
						continue;
					}
					if (*str==')')
					{
						str++;
						break;
					}
					error("Malformed function call.");
				}
			}
#define func(name, numpar, code)                                   \
					if (!strncasecmp(start, name, len) && name[len]==0)      \
					{                                                        \
						if (numparams==numpar) return (code);                  \
						else error("Wrong number of parameters to function."); \
					}
#define varfunc(name, code)                                   \
					if (!strncasecmp(start, name, len) && name[len]==0) \
					{                                                   \
						code;                                             \
					}
			func("sqrt", 1, sqrt(params[0]));
			func("sin", 1, sin(params[0]));
			func("cos", 1, cos(params[0]));
			func("tan", 1, tan(params[0]));
			func("asin", 1, asin(params[0]));
			func("acos", 1, acos(params[0]));
			func("atan", 1, atan(params[0]));
			func("arcsin", 1, asin(params[0]));
			func("arccos", 1, acos(params[0]));
			func("arctan", 1, atan(params[0]));
			func("log", 1, log(params[0]));
			func("log10", 1, log10(params[0]));
			func("log2", 1, log(params[0])/log(2.0));
			func("ln", 1, log(params[0]));
			func("lg", 1, log10(params[0]));
			//varfunc("min", {
			//		if (!numparams) error("Wrong number of parameters to function.");
			//		double minval=params[0];
			//		for (int i=1;i<numparams;i++)
			//		{
			//			if (params[i]<minval) minval=params[i];
			//		}
			//		return minval;
			//	});
			//varfunc("max", {
			//		if (!numparams) error("Wrong number of parameters to function.");
			//		double maxval=params[0];
			//		for (int i=1;i<numparams;i++)
			//		{
			//			if (params[i]>maxval) maxval=params[i];
			//		}
			//		return maxval;
			//	});
#undef func
#undef varfunc
			error("Unknown function.");
		}
		else
		{
#define const(name, val) if (!strnicmp(start, name, len)) return val
			const("pi", 3.141592653589793238462);
			const("\xCF\x80", 3.141592653589793238462);
			const("\xCE\xA0", 3.141592653589793238462);//case insensitive pi, yay
			const("e", 2.718281828459045235360);
#undef const
			error("Unknown constant.");
		}
	}
	error("Invalid number.");
}

static long double sanitize(long double val)
{
	if (val!=val) error("NaN encountered.");
	if (math_round) return (int)val;
	return val;
}

static long double getnum()
{
	while (*str==' ') str++;
#define prefix(name, func) if (!strnicmp(str, name, strlen(name))) { str+=strlen(name); long double val=getnum(); return sanitize(func); }
	prefix("-", -val);
	prefix("~", ~(int)val);
	prefix("+", val);
	//prefix("#", val);
#undef prefix
	return sanitize(getnumcore());
}

static long double eval(int depth)
{
	long double left=getnum();
	long double right;
	while (*str==' ') str++;
	while (*str && *str!=')' && *str!=',')
	{
		while (*str==' ') str++;
#define oper(name, thisdepth, contents)       \
			if (!strnicmp(str, name, strlen(name))) \
			{                                       \
				if (math_pri)                         \
				{                                     \
					if (depth<=thisdepth)               \
					{                                   \
						str+=strlen(name);                \
						right=eval(thisdepth+1);          \
					}                                   \
					else return left;                   \
				}                                     \
				else                                  \
				{                                     \
					str+=strlen(name);                  \
					right=getnum();                     \
				}                                     \
				left=sanitize(contents);              \
				continue;                             \
			}
		oper("**", 4, pow(left, right));
		if (math_xorexp) oper("^", 4, pow(left, right));
		oper("*", 3, left*right);
		oper("/", 3, right?left/right:error("Division by zero."));
		oper("%", 3, right?fmod(left, right):error("Modulos by zero."));
		oper("+", 2, left+right);
		oper("-", 2, left-right);
		oper("<<", 1, (unsigned int)left<<(unsigned int)right);
		oper(">>", 1, (unsigned int)left>>(unsigned int)right);
		oper("&", 0, (unsigned int)left&(unsigned int)right);
		oper("|", 0, (unsigned int)left|(unsigned int)right);
		oper("^", 0, (unsigned int)left^(unsigned int)right);
		error("Unknown operator.");
#undef oper
	}
	return left;
}

long double math(const char * s, const char ** e)
{
	try
	{
		str=s;
		long double rval=eval(0);
		if (*str)
		{
			if (*str==',') error("Invalid input.");
			else error("Mismatched parentheses.");
		}
		*e=NULL;
		return rval;
	}
	catch (const char * error)
	{
		*e=error;
		return 0;
	}
}
#endif
