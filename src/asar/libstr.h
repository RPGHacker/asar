#pragma once

#include "std-includes.h"
//ty alcaro
extern const unsigned char char_props[256];
static inline int to_lower(unsigned char c) { return c|(char_props[c]&0x20); }
static inline int to_upper(unsigned char c) { return c&~(char_props[c]&0x20); }

inline bool is_space(unsigned char c) { return char_props[c] & 0x80; } // C standard says \f \v are space, but this one disagrees
inline bool is_digit(unsigned char c) { return char_props[c] & 0x40; }
inline bool is_alpha(unsigned char c) { return char_props[c] & 0x20; }
inline bool is_lower(unsigned char c) { return char_props[c] & 0x04; }
inline bool is_upper(unsigned char c) { return char_props[c] & 0x02; }
inline bool is_alnum(unsigned char c) { return char_props[c] & 0x60; }
inline bool is_ualpha(unsigned char c) { return char_props[c] & 0x28; }
inline bool is_ualnum(unsigned char c) { return char_props[c] & 0x68; }
inline bool is_xdigit(unsigned char c) { return char_props[c] & 0x01; }

inline char *copy(const char *source, int copy_length, char *dest)
{
	memmove(dest, source, copy_length*sizeof(char));
	return dest;
}

inline int min_val(int a, int b)
{
	return a > b ? b : a;
}

inline int bit_round(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

class string {
public:
const char *data() const
{
	return cached_data;
}

char *temp_raw() const	//things to cleanup and take a look at
{
	return cached_data;
}

char *raw() const
{
	return cached_data;
}

int length() const
{
	return is_inlined() ? inlined.len : allocated.len;
}

void set_length(int length)
{
	if(length > max_inline_length_){
		inlined.len = (unsigned char)-1;
		allocated.len = length;
	}else{
		inlined.len = length;
	}
}

void truncate(int newlen)
{
	resize(newlen);
}

void assign(const char * newstr)
{
	if (!newstr) newstr = "";
	assign(newstr, strlen(newstr));
}

void assign(const string &newstr)
{
	assign(newstr, newstr.length());
}

void assign(const char * newstr, int end)
{
	resize(end);
	copy(newstr, length(), raw());
}


string& operator=(const char * newstr)
{
	assign(newstr);
	return *this;
}

string& operator=(const string &newstr)
{
	assign(newstr);
	return *this;
}

string& operator+=(const string& other)
{
	int current_end = length();
	resize(length() + other.length());
	copy(other.data(), other.length(), raw() + current_end);
	return *this;
}

string& operator+=(const char *other)
{
	int current_end = length();
	int otherlen=(int)strlen(other);
	resize(length() + otherlen);
	copy(other, otherlen, raw() + current_end);
	return *this;
}

string& operator+=(char c)
{
	resize(length() + 1);
	raw()[length() - 1] = c;
	return *this;
}

string operator+(char right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

string operator+(const char * right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

bool operator==(const char * right) const
{
	return !strcmp(data(), right);
}

bool operator==(const string& right) const
{
	return !strcmp(data(), right.data());
}

bool operator!=(const char * right) const
{
	return (strcmp(data(), right) != 0);
}

bool operator!=(const string& right) const
{
	return (strcmp(data(), right.data()) != 0);
}

operator const char*() const
{
	return data();
}

explicit operator bool() const
{
	return length();
}

string()
{
	//todo reduce I know this isn't all needed
	allocated.bufferlen = 0;
	allocated.str = 0;
	allocated.len = 0;
	inlined.len = 0;
	cached_data = inlined.str;
	next_resize = max_inline_length_+1;

}
string(const char * newstr) : string()
{
	assign(newstr);
}
string(const char * newstr, int newlen) : string()
{
	assign(newstr, newlen);
}
string(const string& old) : string()
{
	assign(old.data());
}

string(string &&move) : string()
{
	*this = move;
}

string& operator=(string&& move)
{
	if(!is_inlined()) free(allocated.str);
	if(!move.is_inlined()){
		allocated.str = move.allocated.str;
		allocated.bufferlen = move.allocated.bufferlen;
		set_length(move.allocated.len);
		
		move.inlined.len = 0;
		move.inlined.str[0] = 0;
		cached_data = allocated.str;
		next_resize = move.next_resize;
		
	}else{
		inlined.len = 0;
		cached_data = inlined.str;
		next_resize = max_inline_length_+1;
		assign(move);
	}
	return *this;
}

~string()
{
	if(!is_inlined()){
		free(allocated.str);
	}
}

string& replace(const char * instr, const char * outstr, bool all=true);
string& qreplace(const char * instr, const char * outstr, bool all=true);

#ifdef SERIALIZER
void serialize(serializer & s)
{
	s(str, allocated.bufferlen);
	set_length(strlen(str));
}
#endif
#define SERIALIZER_BANNED

private:
static const int scale_factor = 3; //scale sso
static const int max_inline_length_ = ((sizeof(char *) + sizeof(int) * 2) * scale_factor) - 2;
char *cached_data;
int next_resize;
struct si{
		char str[max_inline_length_ + 1];
		unsigned char len;
};

struct sa{
		char *str;
		int len;
		int bufferlen ;
};
union{
	si inlined;
	sa allocated;
};
		

void resize(int new_length)
{
	const char *old_data = data();
	if(new_length >= next_resize || (!is_inlined() && new_length <= max_inline_length_)) {
		if(new_length > max_inline_length_ && (is_inlined() || allocated.bufferlen <= new_length)){ //SSO or big to big
			int new_size = bit_round(new_length + 1);
			if(old_data == inlined.str){
				allocated.str = copy(old_data, min_val(length(), new_length), (char *)malloc(new_size));
			}else{
				allocated.str = (char *)realloc(allocated.str, new_size);
				old_data = inlined.str;	//this will prevent freeing a dead realloc ptr
			}
			allocated.bufferlen = new_size;
			cached_data = allocated.str;
			next_resize = allocated.bufferlen;
		}else if(length() > max_inline_length_ && new_length <= max_inline_length_){ //big to SSO
			copy(old_data, new_length, inlined.str);
			cached_data = inlined.str;
			next_resize = max_inline_length_+1;
		}
		if(old_data != inlined.str && old_data != data()){
			free((char *)old_data);
		}
	}
	set_length(new_length);
	
	raw()[new_length] = 0; //always ensure null terminator
}

bool is_inlined() const
{
	return inlined.len != (unsigned char)-1;
}
};
#define STR (string)

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
char* strqpstr(const char* str, const char* key);

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
	// randomdude999: With 100 digits of precision, the buffer needs to be approx. 311+100,
	// but let's be safe here https://stackoverflow.com/questions/7235456
	char rval[512];
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
	
	// see above
	char rval[512];
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
		if (to_lower(str[i])!=to_lower(key[i])) return false;
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
		if (to_lower(*strend)!=to_lower(*keyend)) return false;
	}
	return true;
}

inline bool stricmpwithupper(const char *word1, const char *word2)
{
	while(*word2)
	{
		if(to_upper(*word1++) != *word2++) return true;
	}
	return *word1;
}

inline bool stricmpwithlower(const char *word1, const char *word2)
{
	while(*word2)
	{
		if(to_lower(*word1++) != *word2++) return true;
	}
	return *word1;
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
	while (*str != '\0')
	{
		if (*str == key) { return const_cast<char*>(str); }
		else if (*str == '"' || *str == '\'')
		{
			// Special case hack for ''', which is currently our official way of handling the ' character.
			// Even though it really stinks.
			if (str[0] == '\'' && str[1] == '\'' && str[2] == '\'') { str += 2; }
			else
			{
				char delimiter = *str;

				do
				{
					str++;

					// If we want to support backslash escapes, we'll have to add that right here.
				} while (*str != delimiter && *str != '\0');

				// This feels like a superfluous check, but I can't really find a clean way to avoid it.
				if (*str == '\0') { return nullptr; }
			}
		}

		str++;
	}

	return nullptr;
}

// RPG Hacker: WOW, these functions are poopy!
inline char * strqrchr(const char * str, char key)
{
	const char * ret= nullptr;
	while (*str)
	{
		if (*str=='"' || *str=='\'')
		{
			char token = *str;

			str++;

			// Special case hack for '''
			if (str[0] == '\'' && str[1] == '\'') { str += 2; }
			else
			{
				while (*str != token)
				{
					if (!*str) return nullptr;
					str++;
				}
				str++;
			}
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
	return string(str, len);
}

string &strip_prefix(string &str, char c, bool multi=false);
string &strip_suffix(string &str, char c, bool multi=false);
string &strip_both(string &str, char c, bool multi=false);
string &strip_whitespace(string &str);

char * itrim(char * str, const char * left, const char * right, bool multi=false);
string &itrim(string &str, const char * left, const char * right, bool multi=false);

inline string &upper(string &old)
{
	int length = old.length();
	for (int i=0;i<length;i++) old.raw()[i]=(char)to_upper(old.data()[i]);
	return old;
}

inline string &lower(string &old)
{
	int length = old.length();
	for (int i=0;i<length;i++) old.raw()[i]=(char)to_lower(old.data()[i]);
	return old;
}

inline const char * stristr(const char * string, const char * pattern)
{
	if (!*pattern) return string;
	const char * pptr;
	const char * sptr;
	const char * start;
	for (start=string;*start!=0;start++)
	{
		for (;(*start && (to_lower(*start)!=to_lower(*pattern)));start++);
		if (!*start) return nullptr;
		pptr=pattern;
		sptr=start;
		while (to_lower(*sptr)==to_lower(*pptr))
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
			if (!is_space(lines[i][j]) && lines[i][j] != '\0' && lines[i][j] != ';')
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
