#include "std-includes.h"
#include "libstr.h"
#include "virtualfile.h"
#include "asar.h"
#include "warnings.h"

#define typed_malloc(type, count) (type*)malloc(sizeof(type)*(count))
#define typed_realloc(type, ptr, count) (type*)realloc(ptr, sizeof(type)*(count))


// RPG Hacker: Functions below are copied over from Asar 2.0 branch for the sole purpose of generating
// non-UTF-8 deprecation warnings in 1.9. Don't know if there's a better way to handle this, but it means
// we should delete all of this again once we merge branches.

// vvvvvvvvvvvvvvvvvvvvvv

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


size_t utf8_val(int* codepoint, const char* inp) {
	unsigned char c = *inp++;
	int val;
	if (c < 0x80) {
		// plain ascii
		*codepoint = c;
		return 1u;
	}
	// RPG Hacker: Byte sequences starting with 0xC0 or 0xC1 are invalid.
	// So are byte sequences starting with anything >= 0xF5.
	// And anything below 0xC0 indicates a follow-up byte and should never be at the start of a sequence.
	else if (c > 0xC1 && c < 0xF5) {
		// 1, 2 or 3 continuation bytes
		int cont_byte_count = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : 1;
		// bit hack to extract the significant bits from the start byte
		val = (c & ((1 << (6 - cont_byte_count)) - 1));
		for (int i = 0; i < cont_byte_count; i++) {
			unsigned char next = *inp++;
			if ((next & 0xC0) != 0x80) {
				*codepoint = -1;
				return 0u;
			}
			val = (val << 6) | (next & 0x3F);
		}
		if (// too many cont.bytes
			(*inp & 0xC0) == 0x80 ||

			// invalid codepoints
			val > 0x10FFFF ||

			// check overlong encodings
			(cont_byte_count == 3 && val < 0x1000) ||
			(cont_byte_count == 2 && val < 0x800) ||
			(cont_byte_count == 1 && val < 0x80) ||

			// UTF16 surrogates
			(val >= 0xD800 && val <= 0xDFFF)
			) {
			*codepoint = -1;
			return 0u;
		};
		*codepoint = val;
		return 1u + cont_byte_count;
	}

	// if none of the above, this couldn't possibly be a valid encoding
	*codepoint = -1;
	return 0u;
}

bool is_valid_utf8(const char* inp) {
	while (*inp != '\0') {
		int codepoint;
		inp += utf8_val(&codepoint, inp);

		if (codepoint == -1) return false;
	}

	return true;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


char * readfile(const char * fname, const char * basepath)
{
	virtual_file_handle myfile = filesystem->open_file(fname, basepath);
	if (myfile == INVALID_VIRTUAL_FILE_HANDLE) return nullptr;
	size_t datalen = filesystem->get_file_size(myfile);
	char * data= typed_malloc(char, datalen+1);
	data[filesystem->read_file(myfile, data, 0u, datalen)] = 0;
	filesystem->close_file(myfile);
	int inpos=0;
	int outpos=0;
	inpos += check_bom(data + inpos);
	while (data[inpos])
	{
		if (data[inpos]!='\r') data[outpos++]=data[inpos];
		inpos++;
	}
	data[outpos]=0;
	if (!is_valid_utf8(data)) asar_throw_warning(0, warning_id_feature_deprecated, "non-UTF-8 source files", "Re-save the file as UTF-8 in a text editor of choice and avoid using non-ASCII characters in Asar versions < 2.0");
	return data;
}

// RPG Hacker: like readfile(), but doesn't use virtual file system
// and instead read our file directly.
char * readfilenative(const char * fname)
{
	FILE* myfile = fopen(fname, "rb");
	if (myfile == nullptr) return nullptr;
	fseek(myfile, 0, SEEK_END);
	size_t datalen = (size_t)ftell(myfile);
	fseek(myfile, 0, SEEK_SET);
	char * data = typed_malloc(char, datalen + 1);
	data[fread(data, 1u, datalen, myfile)] = 0;
	fclose(myfile);
	int inpos = 0;
	int outpos = 0;
	inpos += check_bom(data + inpos);
	while (data[inpos])
	{
		if (data[inpos] != '\r') data[outpos++] = data[inpos];
		inpos++;
	}
	data[outpos] = 0;
	if (!is_valid_utf8(data)) asar_throw_warning(0, warning_id_feature_deprecated, "non-UTF-8 source files", "Re-save the file as UTF-8 in a text editor of choice and avoid using non-ASCII characters in Asar versions < 2.0");
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

#define dequote(var, next, error) \
	if (var=='"') \
		do { \
			next; \
			while (var!='"' && var != '\0') { \
				if (!var) error; \
				next; \
			} \
			if (var == '\0') error; \
			next; \
		} while(0); \
	else if (var=='\'') \
		do { \
			next; \
			if (!var) error; /* ''' special case hack */ \
			if (var=='\'') { \
				next; \
				if (var!='\'') error; \
				next; \
			} else { \
				next; \
				while (var!='\'' && var != '\0') { \
					if (!var) error; \
					next; \
				} \
				if (var == '\0') error; \
				next; \
			} \
		} while(0)

#define skippar(var, next, error) \
	dequote(var, next, error); \
	else if (var=='(') { \
		int par=1; next; \
		while (par) { \
			dequote(var, next, error); \
			else { \
				if (var=='(') par++; \
				if (var==')') par--; \
				if (!var) error; \
				next; \
			} \
		} \
	} else if (var==')') error

string& string::replace(const char * instr, const char * outstr, bool all)
{
	string& thisstring=*this;
	if (!all)
	{
		const char * ptr=strstr(thisstring, instr);
		if (!ptr) return thisstring;
		string out=STR substr(thisstring, (int)(ptr-thisstring.data()))+outstr+(ptr+strlen(instr));
		thisstring =out;
		return thisstring;
	}
	//performance hack (obviously for ", "->"," in asar, but who cares, it's a performance booster)
	if (strlen(instr)==strlen(outstr)+1 && !memcmp(instr, outstr, strlen(outstr)))
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
		int inlen=(int)strlen(instr);
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

string& string::qreplace(const char * instr, const char * outstr, bool all)
{
	string& thisstring =*this;
	if (!strstr(thisstring, instr)) return thisstring;
	if (!strchr(thisstring, '"') && !strchr(thisstring, '\''))
	{
		thisstring.replace(instr, outstr, all);
		return thisstring;
	}
	bool replaced=true;
	while (replaced)
	{
		replaced=false;
		string out;
		for (int i=0;thisstring[i];)
		{
			dequote(thisstring[i], out+= thisstring[i++], return thisstring);
			else
			{
				if (!strncmp((const char*)thisstring + i, instr, strlen(instr)))
				{
					replaced = true;
					out += outstr;
					i += (int)strlen(instr);
					if (!all)
					{
						out += ((const char*)thisstring) + i;
						thisstring = out;
						return thisstring;
					}
				}
				// randomdude999: prevent appending the null terminator to the output
				else if (thisstring[i]) out += thisstring[i++];
			}
		}
		thisstring =out;
	}
	return thisstring;
}

bool confirmquotes(const char * str)
{
	for (int i=0;str[i];)
	{
		dequote(str[i], i++, return false);
		else i++;
	}
	return true;
}

bool confirmqpar(const char * str)
{
	for (int i=0;str[i];)
	{
		skippar(str[i], i++, return false);
		else i++;
	}
	return true;
}

char ** nsplit(char * str, const char * key, int maxlen, int * len)
{
	if (!strstr(str, key))
	{
		char ** out= typed_malloc(char*, 2);
		out[0]=str;
		out[1]=nullptr;
		if (len) *len=1;
		return out;
	}
	int keylen=(int)strlen(key);
	int count=7; //makes the default alloc 8 elements, sounds fair.
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata= typed_malloc(char*, (size_t)count+1);
	
	int newcount=0;
	char *thisentry=str;
	outdata[newcount++]=thisentry;
	while((thisentry = strstr(thisentry, key))){
		*thisentry = 0;
		thisentry += keylen;
		outdata[newcount++]=thisentry;
		if(newcount >= count)
		{
			outdata = typed_realloc(char *, outdata, count * 2);
			count *= 2;
		}
	}
	
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

char ** qnsplit(char * str, const char * key, int maxlen, int * len)
{
	if (!strchr(str, '"') && !strchr(str, '\'')) return nsplit(str, key, maxlen, len);
	int keylen=(int)strlen(key);
	int count=7;
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata= typed_malloc(char*, (size_t)count+1);
	int newcount=0;
	char * thisentry=str;
	outdata[newcount++]=thisentry;
	while (*thisentry) /*todo fix*/
	{
		dequote(*thisentry, thisentry++, return nullptr);
		else if (!strncmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
			if(newcount >= count)
			{
				outdata = typed_realloc(char *, outdata, count * 2);
				count *= 2;
			}
		}
		else thisentry++;
	}
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

char ** qpnsplit(char * str, const char * key, int maxlen, int * len)
{
	int keylen=(int)strlen(key);
	int count=7;
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata= typed_malloc(char*, (size_t)count+1);
	
	int newcount=0;
	char * thisentry=str;
	outdata[newcount++]=thisentry;
	while (*thisentry)
	{
		skippar(*thisentry, thisentry++, return nullptr);
		else if (!strncmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
			if(newcount >= count)
			{
				outdata = typed_realloc(char *, outdata, count * 2);
				count *= 2;
			}
		}
		else thisentry++;
	}
	outdata[newcount]= nullptr;
	if (len) *len=newcount;
	return outdata;
}

string &strip_prefix(string &str, char c, bool multi)
{
	if(!multi){
		if(str.data()[0] == c){
			str = string(str.data() + 1, str.length() - 1);
		}
		return str;
	}
	int length = str.length();
	for(int i = 0; i < length; i++){
		if(str.data()[i] != c){
			str = string(str.data() + i, str.length() - i);
			return str;
		}
	}
	return str;
}

string &strip_suffix(string &str, char c, bool multi)
{
	if(!multi){
		if(str.data()[str.length() - 1] == c){
			str.truncate(str.length() - 1);
		}
		return str;
	}
	for(int i = str.length() - 1; i >= 0; i--){
		if(str.data()[i] != c){
			str.truncate(i + 1);
			return str;
		}
	}
	return str;
}

string &strip_both(string &str, char c, bool multi)
{
	return strip_suffix(strip_prefix(str, c, multi), c, multi);
}

string &strip_whitespace(string &str)
{
	for(int i = str.length() - 1; i >= 0; i--){
		if(str.data()[i] != ' ' && str.data()[i] != '\t'){
			str.truncate(i + 1);
			break;
		}
	}
	
	int length = str.length();
	for(int i = 0; i < length; i++){
		if(str.data()[i] != ' ' && str.data()[i] != '\t'){
			str = string(str.data() + i, str.length() - i);
			return str;
		}
	}
	return str;
}

char * itrim(char * str, const char * left, const char * right, bool multi)
{
	string tmp(str);
	return strcpy(str, itrim(tmp, left, right, multi).data());
}

//todo merge above with this
string &itrim(string &input, const char * left, const char * right, bool multi)
{
	bool nukeright=true;
	int totallen=input.length();
	int rightlen=(int)strlen(right);
	if (rightlen && rightlen<=totallen)
	{
		do
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
		} while (multi && nukeright && rightlen<=totallen);
	}
	bool nukeleft=true;
	int leftlen = strlen(left);
	if(!multi && leftlen == 1 && input.data()[0] == left[0])
	{
		return input = string(input.data()+1, (input.length()-1));
	}
	else
	{
		do
		{
			
			for (int i = 0; i < leftlen; i++)
			{
				if (to_lower(input.data()[i])!=to_lower(left[i])) nukeleft=false;
			}
			if (nukeleft) input = string(input.data()+leftlen, (input.length()-leftlen));
		} while (multi && nukeleft);
	}
	return input;
}

char* strqpchr(const char* str, char key)
{
	while (*str)
	{
		skippar(*str, str++, return nullptr);
		else if (*str == key) return const_cast<char*>(str);
		else if (!*str) return nullptr;
		else str++;
	}
	return nullptr;
}


char* strqpstr(const char* str, const char* key)
{
	size_t keylen = strlen(key);
	while (*str)
	{
		skippar(*str, str++, return nullptr);
		else if (!strncmp(str, key, keylen)) return const_cast<char*>(str);
		else if (!*str) return nullptr;
		else str++;
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
