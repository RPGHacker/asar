#pragma once

#include "std-includes.h"
#include <cstdint>
#include <cstring>
#include <utility>
#include <string_view>

//ty alcaro
extern const unsigned char char_props[256];
static inline int to_lower(unsigned char c) { return c|(char_props[c]&0x20); }
static inline int to_upper(unsigned char c) { return c&~(char_props[c]&0x20); }

inline bool is_space(unsigned char c) { return char_props[c] & 0x80; } // C standard says \f \v are space, but this one disagrees
// TODO is this opaque table lookup really faster than c >= '0' && c <= '9'?
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
	memcpy(dest, source, copy_length*sizeof(char));
	return dest;
}

class string {
public:
const char *data() const
{
	return data_ptr;
}

char *temp_raw() const	//things to cleanup and take a look at
{
	return data_ptr;
}

char *raw() const
{
	return data_ptr;
}

int length() const
{
	return len;
}

void resize(unsigned int new_length)
{
	if (new_length > capacity()) {
		reallocate_capacity(new_length);
	}

	len = new_length;
	data_ptr[new_length] = 0; //always ensure null terminator
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
	assign(newstr.data(), newstr.length());
}

void assign(const char * newstr, int end)
{
	resize(end);
	copy(newstr, end, data_ptr);
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

string& append(const string& other, int start, int end)
{
	int current_end = length();
	resize(length() + end - start);
	copy(other.data() + start, end - start, data_ptr + current_end);
	return *this;
}

string& append(const char *other, int start, int end)
{
	int current_end = length();
	resize(length() + end - start);
	copy(other + start, end - start, data_ptr + current_end);
	return *this;
}

string& operator+=(const string& other)
{
	int current_end = length();
	resize(length() + other.length());
	copy(other.data(), other.length(), data_ptr + current_end);
	return *this;
}

string& operator+=(const char *other)
{
	int current_end = length();
	int otherlen=(int)strlen(other);
	resize(length() + otherlen);
	copy(other, otherlen, data_ptr + current_end);
	return *this;
}

string& operator+=(char c)
{
	resize(length() + 1);
	data_ptr[length() - 1] = c;
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
	data_ptr = inlined.data;
	len = 0;
	inlined.data[0] = '\0';
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

string(string &&move) noexcept : string()
{
	*this = move;
}

string& operator=(string&& other) noexcept
{
	if (other.is_inlined()) {
		// No resources to steal so just do a normal assignment
		*this = other;
	} else {
		if (is_inlined()) {
			data_ptr = other.data_ptr;
			other.data_ptr = 0;
		} else {
			// Give our old allocation back so other can free it for us
			std::swap(data_ptr, other.data_ptr);
		}
		len = other.len;
		allocated = other.allocated;
	}
	return *this;
}

~string()
{
	if(!is_inlined()){
		free(data_ptr);
	}
}

//maybe these should return refs to chain.  but also good not to encourage chaining
void strip_prefix(char c)
{
	if(data()[0] == c){
		std::memmove(data_ptr, data_ptr + 1, length() - 1);
		resize(length() - 1);
	}
}

void strip_suffix(char c)
{
	if (data()[length() - 1] == c) {
		truncate(length() - 1);
	}
}

string& qreplace(const char * instr, const char * outstr);
string& qnormalize();

// RPG Hacker: My hack shmeck to get around no longer supporting text mode.
// Symbol files are currently the only thing that use text mode, anyways, and I don't even know
// if the emulators that read them care about line endings.
string& convert_line_endings_to_native()
{
#if defined(windows)
	// RPG Hacker: This is quite stinky, but doing the replacement directly will lead to a dead-lock.
	// \x08 = backspace should never appear inside a string, so I'm abusing it here.
	return qreplace("\n", "\x08").qreplace("\x08", "\r\n");
#else
	return *this;
#endif
}

private:
static const int scale_factor = 4; //scale sso
static const int inline_capacity = ((sizeof(char *) + sizeof(int) * 2) * scale_factor) - 2;

// Points to a malloc'd data block or to inlined.data
char *data_ptr;
unsigned int len;
union {
	struct {
		// Actual allocated capacity is +1 this value, to cover for the terminating NUL
		unsigned int capacity;
	} allocated;
	struct {
		char data[inline_capacity + 1];
	} inlined;
};

void reallocate_capacity(unsigned int new_length);

unsigned capacity() const
{
	return is_inlined() ? inline_capacity : allocated.capacity;
}

bool is_inlined() const
{
	return data_ptr == inlined.data;
}
};
#define STR (string)

#define ASAR_STRCMP_OPERATORS(op) \
	inline bool operator op(const string& left, const string& right) { \
		return strcmp(left, right) op 0; \
	} \
	inline bool operator op(const string& left, const char* right) { \
		return strcmp(left, right) op 0; \
	} \
	inline bool operator op(const char* left, const string& right) { \
		return strcmp(left, right) op 0; \
	}

ASAR_STRCMP_OPERATORS(==)
ASAR_STRCMP_OPERATORS(!=)
ASAR_STRCMP_OPERATORS(<)
ASAR_STRCMP_OPERATORS(<=)
ASAR_STRCMP_OPERATORS(>)
ASAR_STRCMP_OPERATORS(>=)
#undef ASAR_STRCMP_OPERATORS

template<>
struct std::hash<string> {
	size_t operator()(const ::string& s) const {
		return std::hash<std::string_view>()(std::string_view(s.data(), s.length()));
	}
};

char * readfile(const char * fname, const char * basepath);
char * readfilenative(const char * fname);
bool readfile(const char * fname, const char * basepath, char ** data, int * len);//if you want an uchar*, cast it
char ** split(char * str, char key, int * len= nullptr);
char ** qsplit(char * str, char key, int * len= nullptr);
char ** qpsplit(char * str, char key, int * len= nullptr);
char ** qsplitstr(char * str, const char * key, int * len= nullptr);
bool confirmquotes(const char * str);
bool confirmqpar(const char * str);
char* strqpchr(char* str, char key);
char* strqpstr(char* str, const char* key);

inline string hex(unsigned int value)
{
	char buffer[64];
	if(0);
	else if (value<=0x000000FF) snprintf(buffer, sizeof(buffer), "%.2X", value);
	else if (value<=0x0000FFFF) snprintf(buffer, sizeof(buffer), "%.4X", value);
	else if (value<=0x00FFFFFF) snprintf(buffer, sizeof(buffer), "%.6X", value);
	else snprintf(buffer, sizeof(buffer), "%.8X", value);
	return buffer;
}

inline string hex(unsigned int value, int width)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%.*X", width, value);
	return buffer;
}

inline string dec(int value)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%i", value);
	return buffer;
}

inline string ftostr(double value)
{
	// randomdude999: With 100 digits of precision, the buffer needs to be approx. 311+100,
	// but let's be safe here https://stackoverflow.com/questions/7235456
	char rval[512];
	// RPG Hacker: Ridiculously high precision, I know, but we're working with doubles
	// here and can afford it, so no need to waste any precision
	snprintf(rval, sizeof(rval), "%.100f", value);
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
	snprintf(rval, sizeof(rval), "%.*f", clampedprecision, (double)value);
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
	if(keyend-key > strend-str) return false;

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
	char *end = strrchr(str, '"');
	if (end)
	{
		*end = 0;
		char *quote = str+1;
		while((quote = strstr(quote, "\"\""))) memmove(quote, quote+1, strlen(quote));
		return str + 1;
	}
	return nullptr;
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

inline string substr(const char * str, int len)
{
	return string(str, len);
}


inline char *strip_whitespace(char *str)
{
	while(is_space(*str)) str++;
	for(int i = strlen(str) - 1; i >= 0; i--)
	{
		if(!is_space(str[i]))
		{
			str[i + 1] = 0;
			return str;
		}
	}
	return str;
}
inline void strip_whitespace(string &str)
{
	str = string(strip_whitespace(str.temp_raw()));
}

string &itrim(string &str, const char * left, const char * right);

inline string &lower(string &old)
{
	int length = old.length();
	for (int i=0;i<length;i++) old.raw()[i]=(char)to_lower(old.data()[i]);
	return old;
}


// Returns number of connected lines - 1
template<typename stringarraytype>
inline int getconnectedlines(stringarraytype& lines, int startline, string& out)
{
	int count = 0;

	for (int i = startline; lines[i]; i++)
	{
		// The line should already be stripped of any comments at this point
		int linestartpos = (int)strlen(lines[i]);

		if(linestartpos && lines[i][linestartpos - 1] == '\\')
		{
			count++;
			out += string(lines[i], linestartpos - 1);
			continue;
		}
		else
		{
			out += string(lines[i], linestartpos);
			return count;
		}
	}

	return count;
}
