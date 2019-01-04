#pragma once

#include "std-includes.h"

class string {
public:
char * str;
int len;

int truelen() const
{
	return len?len:(int)strlen(str);
}

void fixlen()
{
	if (!len) len=(int)strlen(str);
}

private:
//char shortbuf[32];
int bufferlen;

void resize(int inlen)
{
	if (inlen >=bufferlen-4)
	{
		while (inlen >=bufferlen-4) bufferlen*=2;
		str=(char*)realloc(str, sizeof(char)*(size_t)bufferlen);
		memset(str+ inlen, 0, (size_t)(bufferlen- inlen));
	}
}

void init()
{
	bufferlen=32;
	str=(char*)malloc(sizeof(char)*(size_t)bufferlen);
	memset(str, 0, (size_t)bufferlen);
	len=0;
}

public:

void assign(const char * newstr)
{
	if (!newstr) newstr="";
	len=(int)strlen(newstr);
	resize(len);
	memmove(str, newstr, (size_t)len);
	str[len] = '\0';
}

void assign(const char * newstr, int newlen)
{
	if (!newstr) return;
	len=newlen;
	resize(len);
	memmove(str, newstr, (size_t)len);
	str[len] = '\0';
	len=(int)strlen(str);
}

string& operator=(const char * newstr)
{
	if (bufferlen == 0) init();
	assign(newstr);
	return *this;
}

string& operator=(string newstr)
{
	if (bufferlen == 0) init();
	assign(newstr);
	return *this;
}

string& operator+=(const string& newstr)
{
	fixlen();
	resize(len+newstr.truelen());
	memmove(str+len, newstr.str, (size_t)newstr.len);
	len+=newstr.len;
	str[len] = '\0';
	return *this;
}

string& operator+=(const char * newstr)
{
	fixlen();
	int newlen=(int)strlen(newstr);
	resize(len+newlen);
	memmove(str+len, newstr, (size_t)newlen);
	len+=newlen;
	str[len] = '\0';
	return *this;
}

string& operator+=(char c)
{
	fixlen();
	resize(len);
	str[len]=c;
	str[len+1]='\0';
	len++;
	return *this;
}

string operator+(char right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

const char * operator+(int right) const
{
	return str+right;
}

string operator+(const char * right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

string operator+(const string& right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

char& operator*()
{
	len=0;
	return str[0];
}

bool operator==(const char * right) const
{
	return !strcmp(str, right);
}

bool operator==(string& right) const
{
	return !strcmp(str, right.str);
}

bool operator!=(const char * right) const
{
	return (strcmp(str, right) != 0);
}

bool operator!=(string& right) const
{
	return (strcmp(str, right.str) != 0);
}

bool operator<(string& right) const
{
	return strcmp(str, right.str)<0;
}

char& operator[](int id)
{
	static char nul;
	nul='\0';
	if (id<0) return nul;
	len=0;
	resize(id);
	return str[id];
}

operator const char*() const
{
	return str;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
explicit
#endif
operator bool() const
{
	return (str[0] != 0);
}

string()
{
	init();
}
string(const char * newstr)
{
	init();
	assign(newstr);
}
string(const char * newstr, int newlen)
{
	init();
	assign(newstr, newlen);
}
string(const string& old)
{
	init();
	assign(old.str);
}
string(int intval)
{
	init();
	char buf[16];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%i", intval);
	assign(buf);
}
~string()
{
	free(str);
}

string& replace(const char * instr, const char * outstr, bool all=true);
string& qreplace(const char * instr, const char * outstr, bool all=true);

#ifdef SERIALIZER
void serialize(serializer & s)
{
	s(str, bufferlen);
	len=strlen(str);
}
#endif
#define SERIALIZER_BANNED
};
#define S (string)

class cstring {
public:
const char * str;

cstring& operator=(const char * newstr)
{
	str=newstr;
	return *this;
}

cstring& operator=(cstring newstr)
{
	str=newstr.str;
	return *this;
}

char operator*() const
{
	return str[0];
}

bool operator==(const char * right) const
{
	return !strcmp(str, right);
}

bool operator==(cstring& right) const
{
	return !strcmp(str, right.str);
}

bool operator!=(const char * right) const
{
	return (strcmp(str, right) != 0);
}

bool operator!=(cstring& right) const
{
	return (strcmp(str, right.str) != 0);
}

bool operator<(cstring& right) const
{
	return strcmp(str, right.str)<0;
}

char operator[](int id) const
{
	return str[id];
}

operator const char*() const
{
	return str;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
explicit
#endif
operator bool() const
{
	return (str != nullptr);
}

cstring()
{
	str= nullptr;
}
cstring(const char * newstr)
{
	str=newstr;
}
cstring(const cstring& old)
{
	str=old.str;
}

string operator+(const cstring& right) const
{
	return (string)str+(const char*)right;
}
string operator+(const string& right) const
{
	return (string)str+(const char*)right;
}
string operator+(const char * right) const
{
	return (string)str+right;
}
};

//string operator+(const char * left, string right)
//{
//	string s=left;
//	s+=right;
//	return s;
//}

char * readfile(const char * fname, const char * basepath);
char * readfilenative(const char * fname);
bool readfile(const char * fname, const char * basepath, char ** data, int * len);//if you want an uchar*, cast it
char ** nsplit(char * str, const char * key, int maxlen, int * len);
char ** qnsplit(char * str, const char * key, int maxlen, int * len);
char ** qpnsplit(char * str, const char * key, int maxlen, int * len);
inline char ** split(char * str, const char * key, int * len= nullptr) { return nsplit(str, key, 0, len); }
inline char ** qsplit(char * str, const char * key, int * len= nullptr) { return qnsplit(str, key, 0, len); }
inline char ** qpsplit(char * str, const char * key, int * len= nullptr) { return qpnsplit(str, key, 0, len); }
inline char ** split1(char * str, const char * key, int * len= nullptr) { return nsplit(str, key, 2, len); }
inline char ** qsplit1(char * str, const char * key, int * len= nullptr) { return qnsplit(str, key, 2, len); }
inline char ** qpsplit1(char * str, const char * key, int * len= nullptr) { return qpnsplit(str, key, 2, len); }
//void replace(string& str, const char * instr, const char * outstr, bool all);
//void qreplace(string& str, const char * instr, const char * outstr, bool all);
bool confirmquotes(const char * str);
bool confirmqpar(const char * str);
char* strqpchr(const char* str, char key);

inline string hex(unsigned int value)
{
	char buffer[64];
	if(0);
	else if (value<=0x000000FF) sprintf(buffer, "%.2X", value);
	else if (value<=0x0000FFFF) sprintf(buffer, "%.4X", value);
	else if (value<=0x00FFFFFF) sprintf(buffer, "%.6X", value);
	else sprintf(buffer, "%.8X", value);
	return buffer;
}

inline string hex(unsigned int value, int width)
{
	char buffer[64];
	sprintf(buffer, "%.*X", width, value);
	return buffer;
}

inline string hex0(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%X", value);
	return buffer;
}

inline string hex2(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.2X", value);
	return buffer;
}

inline string hex3(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.3X", value);
	return buffer;
}

inline string hex4(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.4X", value);
	return buffer;
}

inline string hex5(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.5X", value);
	return buffer;
}

inline string hex6(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.6X", value);
	return buffer;
}

inline string hex8(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.8X", value);
	return buffer;
}

inline string dec(int value)
{
	char buffer[64];
	sprintf(buffer, "%i", value);
	return buffer;
}

inline string ftostr(double value)
{
	char rval[256];
	// RPG Hacker: Ridiculously high precision, I know, but we're working with doubles
	// here and can afford it, so no need to waste any precision
	sprintf(rval, "%.100f", value);
	if (strchr(rval, '.'))//nuke useless zeroes
	{
		char * end=strrchr(rval, '\0')-1;
		while (*end=='0')
		{
			*end='\0';
			end--;
		}
		if (*end=='.') *end='\0';
	}
	return rval;
}

// Same as above, but with variable precision
inline string ftostrvar(double value, int precision)
{
	int clampedprecision = precision;
	if (clampedprecision < 0) clampedprecision = 0;
	if (clampedprecision > 100) clampedprecision = 100;
	
	char rval[256];
	sprintf(rval, "%.*f", clampedprecision, (double)value);
	if (strchr(rval, '.'))//nuke useless zeroes
	{
		char * end = strrchr(rval, '\0') - 1;
		while (*end == '0')
		{
			*end = '\0';
			end--;
		}
		if (*end == '.') *end = '\0';
	}
	return rval;
}

inline bool stribegin(const char * str, const char * key)
{
	for (int i=0;key[i];i++)
	{
		if (tolower(str[i])!=tolower(key[i])) return false;
	}
	return true;
}

inline bool striend(const char * str, const char * key)
{
	const char * keyend=strrchr(key, '\0');
	const char * strend=strrchr(str, '\0');
	while (key!=keyend)
	{
		keyend--;
		strend--;
		if (tolower(*strend)!=tolower(*keyend)) return false;
	}
	return true;
}

//function: return the string without quotes around it, if any exists
//if they don't exist, return it unaltered
//it is not guaranteed to return str
//it is not guaranteed to not edit str
//the input must be freed even though it's garbage, the output must not
inline const char * dequote(char * str)
{
	if (*str!='"') return str;
	int inpos=1;
	int outpos=0;
	while (true)
	{
		if (str[inpos]=='"')
		{
			if (str[inpos+1]=='"') inpos++;
			else if (str[inpos+1]=='\0') break;
			else return nullptr;
		}
		if (!str[inpos]) return nullptr;
		str[outpos++]=str[inpos++];
	}
	str[outpos]=0;
	return str;
}

inline char * strqchr(const char * str, char key)
{
	while (*str)
	{
		if (*str=='"')
		{
			str++;
			while (*str!='"')
			{
				if (!*str) return nullptr;
				str++;
			}
			str++;
		}
		else
		{
			if (*str==key) return const_cast<char*>(str);
			str++;
		}
	}
	return nullptr;
}

// RPG Hacker: WOW, these functions are poopy!
inline char * strqrchr(const char * str, char key)
{
	const char * ret= nullptr;
	while (*str)
	{
		if (*str=='"')
		{
			str++;
			while (*str!='"')
			{
				if (!*str) return nullptr;
				str++;
			}
			str++;
		}
		else
		{
			if (*str==key) ret=str;
			str++;
		}
	}
	return const_cast<char*>(ret);
}

inline string substr(const char * str, int len)
{
	string s;
	s.assign(str, len);
	return s;
}

char * trim(char * str, const char * left, const char * right, bool multi=false);
char * itrim(char * str, const char * left, const char * right, bool multi=false);

inline string upper(const char * old)
{
	string s=old;
	for (int i=0;s.str[i];i++) s.str[i]=(char)toupper(s.str[i]);
	return s;
}

inline string lower(const char * old)
{
	string s=old;
	for (int i=0;s.str[i];i++) s.str[i]=(char)tolower(s.str[i]);
	return s;
}

inline int isualnum ( int c )
{
	return (c >= -1 && c <= 255 && (c=='_' || isalnum(c)));
}

inline int isualpha ( int c )
{
	return (c >= -1 && c <= 255 && (c=='_' || isalpha(c)));
}

#define ctype(type) \
		inline bool ctype_##type(const char * str) \
		{ \
			while (*str) \
			{ \
				if (!is##type(*str)) return false; \
				str++; \
			} \
			return true; \
		} \
		\
		inline bool ctype_##type(const char * str, int num) \
		{ \
			for (int i=0;i<num;i++) \
			{ \
				if (!is##type(str[i])) return false; \
			} \
			return true; \
		}
		
ctype(alnum)
ctype(alpha)
ctype(cntrl)
ctype(digit)
ctype(graph)
ctype(lower)
ctype(print)
ctype(punct)
ctype(space)
ctype(upper)
ctype(xdigit)
ctype(ualnum)
ctype(ualpha)
#undef ctype

inline const char * stristr(const char * string, const char * pattern)
{
	if (!*pattern) return string;
	const char * pptr;
	const char * sptr;
	const char * start;
	for (start=string;*start!=0;start++)
	{
		for (;(*start && (toupper(*start)!=toupper(*pattern)));start++);
		if (!*start) return nullptr;
		pptr=pattern;
		sptr=start;
		while (toupper(*sptr)==toupper(*pptr))
		{
			sptr++;
			pptr++;
			if (!*pptr) return start;
		}
	}
	return nullptr;
}



// Returns number of connected lines - 1
template<typename stringarraytype>
inline int getconnectedlines(stringarraytype& lines, int startline, string& out)
{
	out = string("");
	int count = 1;

	for (int i = startline; lines[i]; i++)
	{
		// The line should already be stripped of any comments at this point
		int linestartpos = (int)strlen(lines[i]);

		bool found = false;

		for (int j = linestartpos; j > 0; j--)
		{
			if (!isspace(lines[i][j]) && lines[i][j] != '\0' && lines[i][j] != ';')
			{
				if (lines[i][j] == '\\')
				{
					count++;
					out += string(lines[i], j);
					found = true;
					break;
				}
				else
				{
					out += string(lines[i], j + 1);
					return count - 1;
				}
			}
		}

		if (!found)
		{
			out += string(lines[i], 1);
			return count - 1;
		}
	}

	return count - 1;
}
