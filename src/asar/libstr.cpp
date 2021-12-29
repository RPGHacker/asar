#include "asar.h"
#include "virtualfile.h"
#include "unicode.h"

#include "platform/file-helpers.h"

#define typed_malloc(type, count) (type*)malloc(sizeof(type)*(count))
#define typed_realloc(type, ptr, count) (type*)realloc(ptr, sizeof(type)*(count))


// Detects if str starts with a UTF-8 byte order mark.
// If so, throws a warning, then returns the number of bytes we should skip ahead in the string.
size_t check_bom(const char* str)
{
	// RPG Hacker: We could also check for BoMs of incompatible encodings here (like UTF-16)
	// and throw errors, but not sure if that's worth adding. Asar never supported any wide
	// encodings to begin with, so it's unreasonable to assume that any UTF-16 patches currently
	// exist for it. As for future patches, those should be caught by the "must be UTF-8" checks
	// I have already implemented further below.
	// I think UTF-8 + BoM is the only case that could lead to confusion if we didn't handle it,
	// so that's why I have added this.
	if (str[0u] == '\xEF' && str[1u] == '\xBB' && str[2u] == '\xBF')
	{
		asar_throw_warning(0, warning_id_byte_order_mark_utf8);
		return 3u;
	}

	return 0u;
}


char * readfile(const char * fname, const char * basepath)
{
	virtual_file_handle myfile = filesystem->open_file(fname, basepath);
	if (myfile == INVALID_VIRTUAL_FILE_HANDLE) return nullptr;
	size_t datalen = filesystem->get_file_size(myfile);
	char * data= typed_malloc(char, datalen+1);
	data[filesystem->read_file(myfile, data, 0u, datalen)] = 0;
	filesystem->close_file(myfile);

	if (!is_valid_utf8(data)) asar_throw_error(0, error_type_block, error_id_invalid_utf8);
	if(check_bom(data)){
		data[0] = ' ';
		data[1] = ' ';
		data[2] = ' ';
	}
	return data;
}

// RPG Hacker: like readfile(), but doesn't use virtual file system
// and instead read our file directly.
char * readfilenative(const char * fname)
{
	FileHandleType myfile = open_file(fname, FileOpenMode_Read);
	if (myfile == InvalidFileHandle) return nullptr;
	size_t datalen = (size_t)get_file_size(myfile);
	char * data = typed_malloc(char, datalen + 1);
	data[read_file(myfile, data, datalen)] = 0;
	close_file(myfile);

	if (!is_valid_utf8(data)) asar_throw_error(0, error_type_block, error_id_invalid_utf8);
	if(check_bom(data)){
		data[0] = ' ';
		data[1] = ' ';
		data[2] = ' ';
	}
	return data;
}

bool readfile(const char * fname, const char * basepath, char ** data, int * len)
{
	virtual_file_handle myfile = filesystem->open_file(fname, basepath);
	if (!myfile) return false;
	size_t datalen = filesystem->get_file_size(myfile);
	*data= typed_malloc(char, datalen);
	*len = (int)filesystem->read_file(myfile, *data, 0, datalen);
	filesystem->close_file(myfile);
	return true;
}

#define isq(n) (((0x2227 ^ (0x0101 * (n))) - 0x0101UL) & ~(0x2227 ^ (0x0101 * (n))) & 0x8080UL)
#define isqp(n) (((0x22272829 ^ (0x01010101 * (n))) - 0x01010101UL) & ~(0x22272829 ^ (0x01010101 * (n))) & 0x80808080UL)

const bool qparlut[256] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//this will leave the last char found as the one pointed at
inline bool skip_quote(char *&str)
{
	if (isq(*str)){
		str = strchr(str + 1, *str);
	}
	return str;
}

//eat 1 char or quote/par
inline bool skip_par(char *&str)
{
	int par = 0;
	if(*str != '\'' && *str != '"' && *str != '(' && *str != ')')
	{
		str++;
		return true;
	}
	while(true)
	{
		char *t = str;
		if(isq(*t))
		{
			t = strchr(t+1, *t);
			if(!t) return false;
		}
		else if(*t == '(')
		{
			par++;
		}
		else if(*t == ')')
		{
			par--;
			if(par < 0) return false;
		}

		str = t + 1;
		if(!*str || !par) return par == 0 ? true : false;
	}
}

string& string::replace(const char * instr, const char * outstr)
{
	string& thisstring=*this;
	int inlen=(int)strlen(instr);
	//performance hack (obviously for ", "->"," in asar, but who cares, it's a performance booster)
	if (inlen==strlen(outstr)+1 && !memcmp(instr, outstr, strlen(outstr)))
	{
		const char * indat= thisstring;
		char * trueoutdat= typed_malloc(char, strlen(indat)+1);
		char * outdat=trueoutdat;
		int thelen=(int)strlen(outstr);
		//nested hack
		if (thelen==0)
		{
			char thechar=instr[0];
			while (*indat)
			{
				if (*indat==thechar) indat++;
				else *outdat++=*indat++;
			}
			*outdat=0;
			thisstring =trueoutdat;
			free(trueoutdat);
			return thisstring;
		}
		else if (thelen==1)
		{
			char firstchar=instr[0];
			char secondchar=instr[1];
			while (*indat)
			{
				if (*indat==firstchar)
				{
					*outdat++=*indat++;
					while (*indat==secondchar) indat++;
				}
				else *outdat++=*indat++;
			}
			*outdat=0;
			thisstring =trueoutdat;
			free(trueoutdat);
			return thisstring;
		}
		else
		//end hack
		{
			char thehatedchar=instr[thelen];
			while (*indat)
			{
				if (!memcmp(indat, outstr, (size_t)thelen))
				{
					memcpy(outdat, outstr, (size_t)thelen);
					outdat+=thelen;
					indat+=thelen;
					while (*indat==thehatedchar) indat++;
				}
				else *outdat++=*indat++;
			}
		}
		*outdat=0;
		thisstring =trueoutdat;
		free(trueoutdat);
		return thisstring;
	}
	//end hack
	bool replaced=true;
	while (replaced)
	{
		replaced=false;
		string out;
		const char * in= thisstring;
		while (*in)
		{
			if (!strncmp(in, instr, (size_t)inlen))
			{
				replaced=true;
				out+=outstr;
				in+=inlen;
			}
			else out+=*in++;
		}
		thisstring =out;
	}
	return thisstring;
}

//instr should not be duplicate chars.  Instr should also not be 1 char
string& string::qreplace(const char * instr, const char * outstr)
{
	string& thisstring =*this;
	if (!strstr(thisstring, instr)) return thisstring;
	int inlen = strlen(instr);
	string out;
	for (int i=0;thisstring[i];)
	{
		if (!strncmp((const char*)thisstring +i, instr, inlen))
		{
			out+=outstr;
			i+=inlen;
		}
		// randomdude999: prevent appending the null terminator to the output
		else if(!isq(thisstring[i])) out+= thisstring[i++];
		else
		{
			char *start = raw() + i;
			char *end = start;
			if(!skip_quote(end)) return thisstring;
			out.append(raw(), i, end - start + i + 1);
			i += end - start + 1;

		}
	}
	thisstring =out;
	return thisstring;
}

string& string::qnormalize()
{
	string& thisstring =*this;
	string out;
	char *startstr = thisstring.raw();
	char *str = startstr;
	while(str = strpbrk(str, "'\" \t,\r"))
	{
		if(is_space(*str))
		{
			if(str[0] == ' ' && !is_space(str[1]))
			{
				str++;
				continue;
			}
			out.append(startstr, 0, str - startstr);
			out += ' ';
			while(is_space(*str)) str++;
			startstr = str;
		}else if(*str == ',')
		{
			str++;
			if(is_space(*str))
			{
				out.append(startstr, 0, str - startstr);
				while(is_space(*str)) str++;
				startstr = str;
			}
		}
		else
		{
			str = strchr(str + 1, *str);  //confirm quotes has already been run, so this should be okay
			if(!str) return thisstring;
			str++;
		}
	}
	if(startstr != thisstring.raw())
	{
		out.append(startstr, 0, strlen(startstr)); //the remaining

		thisstring = out;
	}
	return thisstring;
}

bool confirmquotes(const char * str)
{
	while(*str)
	{
		char *dquote = strchr((char *)str, '"');
		char *squote = strchr((char *)str, '\'');
		if(dquote || squote)
		{
			if(dquote && (dquote < squote || !squote))
			{
				dquote = strchr(dquote+1, '"');
				if(dquote) str = dquote+1;
				else return false;
			}
			else
			{
				squote = strchr(squote+1, '\'');
				if(squote) str = squote+1;
				else return false;
			}
		}
		else
		{
			return true;
		}
	}
	return true;
}

bool confirmqpar(const char * str)
{
	//todo fully optimize
	int par = 0;
	while(!qparlut[*str]) str++;
	while(*str)
	{
		if(isq(*str))
		{
			str = strchr(str + 1, *str);
			if(!str++) return false;
		}
		else
		{
			par += 1 - ((*str++ - '(') << 1);
			if(par < 0) return false;
		}
		while(!qparlut[*str]) str++;
	}
	return !par;
}

char ** split(char * str, char key, int * len)
{
	char *thisentry=strchr(str, key);
	if (!thisentry)
	{
		char ** out= typed_malloc(char*, 2);
		out[0]=str;
		out[1]=nullptr;
		if (len) *len=1;
		return out;
	}
	int count=15; //makes the default alloc 8 elements, sounds fair.
	char ** outdata= typed_malloc(char*, (size_t)count+1);

	int newcount=0;
	outdata[newcount++]=str;
	do{
		*thisentry = 0;
		thisentry++;
		outdata[newcount++]=thisentry;
		if(newcount >= count)
		{
			count *= 2;
			outdata = typed_realloc(char *, outdata, count);
		}
	}while((thisentry = strchr(thisentry, key)));

	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

char ** qsplit(char * str, char key, int * len)
{
	if (!strchr(str, '"') && !strchr(str, '\'')) return split(str, key, len);

	int count=15;
	char ** outdata= typed_malloc(char*, (size_t)count+1);
	int newcount=0;
	char * thisentry=str;
	outdata[newcount++]=thisentry;
	while (*thisentry) /*todo fix*/
	{
		if (*thisentry == key)
		{
			*thisentry=0;
			thisentry++;
			outdata[newcount++]=thisentry;
			if(newcount >= count)
			{
				count *= 2;
				outdata = typed_realloc(char *, outdata, count);
			}
		}
		else if(skip_quote(thisentry)) thisentry++;
		else return nullptr;
	}
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

char ** qsplitstr(char * str, const char * key, int * len)
{
	//check if the str is found first
	if (!strstr(str, key))
	{
		char ** out= typed_malloc(char*, 2);
		out[0]=str;
		out[1]=nullptr;
		if (len) *len=1;
		return out;
	}

	int keylen=(int)strlen(key);
	int count=15;
	char ** outdata= typed_malloc(char*, (size_t)count+1);
	int newcount=0;
	char * thisentry=str;
	outdata[newcount++]=thisentry;
	while (*thisentry) /*todo fix*/
	{
		if (!strncmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
			if(newcount >= count)
			{
				count *= 2;
				outdata = typed_realloc(char *, outdata, count);
			}
		}
		else if(skip_quote(thisentry)) thisentry++;
		else return nullptr;
	}
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

//this function is most commonly called in cases where additional chars are very likely
char ** qpsplit(char * str, char key, int * len)
{
	if (!strchr(str, '(') && !strchr(str, ')')) return qsplit(str, key, len);
	int count=7;
	char ** outdata= typed_malloc(char*, (size_t)count+1);

	int newcount=0;
	char * thisentry=str;
	outdata[newcount++]=thisentry;
	while (*thisentry)
	{
		//skippar(*thisentry, thisentry++, return nullptr;)
		if (*thisentry == key)
		{
			*thisentry=0;
			thisentry++;
			outdata[newcount++]=thisentry;
			if(newcount >= count)
			{
				count *= 2;
				outdata = typed_realloc(char *, outdata, count);
			}
		}
		else if(!skip_par(thisentry)) return nullptr;
	}
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

string &itrim(string &input, const char * left, const char * right)
{
	bool nukeright=true;
	int totallen=input.length();
	int rightlen=(int)strlen(right);
	if (rightlen && rightlen<=totallen)
	{
		const char * rightend=right+rightlen;
		const char * strend=input.data()+totallen;
		while (right!=rightend)
		{
			rightend--;
			strend--;
			if (to_lower(*strend)!=to_lower(*rightend)) nukeright=false;
		}
		if (nukeright)
		{
			totallen-=rightlen;
			input.truncate(totallen);
		}
	}
	bool nukeleft=true;
	int leftlen = strlen(left);
	if(leftlen == 1 && input.data()[0] == left[0])
	{
		return input = string(input.data()+1, (input.length()-1));
	}
	else
	{
		for (int i = 0; i < leftlen; i++)
		{
			if (to_lower(input.data()[i])!=to_lower(left[i])) nukeleft=false;
		}
		if (nukeleft) input = string(input.data()+leftlen, (input.length()-leftlen));
	}
	return input;
}

char* strqpchr(char* str, char key)
{
	while (*str)
	{
		if (*str == key) return const_cast<char*>(str);
		else if(!skip_par(str)) return nullptr;
	}
	return nullptr;
}

extern const uint8_t char_props[256] = {
	//x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x00,0x00, // 0x
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 1x
	0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 2x  !"#$%&'()*+,-./
	0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x00,0x00,0x00,0x00,0x00,0x00, // 3x 0123456789:;<=>?
	0x00,0x23,0x23,0x23,0x23,0x23,0x23,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22, // 4x @ABCDEFGHIJKLMNO
	0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x00,0x00,0x00,0x00,0x08, // 5x PQRSTUVWXYZ[\]^_
	0x00,0x25,0x25,0x25,0x25,0x25,0x25,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24, // 6x `abcdefghijklmno
	0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x00,0x00,0x00,0x00,0x00, // 7x pqrstuvwxyz{|}~
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 8x
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 9x
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Ax
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Bx
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Cx
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Dx
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Ex
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Fx
};
