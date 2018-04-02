#include "std-includes.h"
#include "libstr.h"
#include "virtualfile.hpp"

#define malloc(type, count) (type*)malloc(sizeof(type)*(count))
#define realloc(val, type, count) (type*)realloc(val, sizeof(type)*(count))

extern virtual_filesystem* filesystem;
extern string thisfilename;

char * readfile(const char * fname, const char * basepath)
{
	virtual_file_handle myfile = filesystem->open_file(fname, basepath);
	if (myfile == INVALID_VIRTUAL_FILE_HANDLE) return NULL;
	size_t datalen = filesystem->get_file_size(myfile);
	char * data=malloc(char, datalen+1);
	data[filesystem->read_file(myfile, data, 0u, datalen)] = 0;
	filesystem->close_file(myfile);
	int inpos=0;
	int outpos=0;
	while (data[inpos])
	{
		if (data[inpos]!='\r') data[outpos++]=data[inpos];
		inpos++;
	}
	data[outpos]=0;
	return data;
}

// RPG Hacker: like readfile(), but doesn't use virtual file system
// and instead read our file directly.
char * readfilenative(const char * fname)
{
	FILE* myfile = fopen(fname, "rb");
	if (myfile == NULL) return NULL;
	fseek(myfile, 0, SEEK_END);
	size_t datalen = (size_t)ftell(myfile);
	fseek(myfile, 0, SEEK_SET);
	char * data = malloc(char, datalen + 1);
	data[fread(data, 1u, datalen, myfile)] = 0;
	fclose(myfile);
	int inpos = 0;
	int outpos = 0;
	while (data[inpos])
	{
		if (data[inpos] != '\r') data[outpos++] = data[inpos];
		inpos++;
	}
	data[outpos] = 0;
	return data;
}

bool readfile(const char * fname, const char * basepath, char ** data, int * len)
{
	virtual_file_handle myfile = filesystem->open_file(fname, basepath);
	if (!myfile) return false;
	size_t datalen = filesystem->get_file_size(myfile);
	*data=malloc(char, datalen);
	*len = (int)filesystem->read_file(myfile, *data, 0, datalen);
	filesystem->close_file(myfile);
	return true;
}

#define dequote(var, next, error) if (var=='"') do { next; while (var!='"') { if (!var) error; next; } next; } while(0)
#define skippar(var, next, error) dequote(var, next, error); else if (var=='(') { int par=1; next; while (par) { dequote(var, next, error); \
				if (var=='(') par++; if (var==')') par--; if (!var) error; next; } } else if (var==')') error

string& string::replace(const char * instr, const char * outstr, bool all)
{
	string& thisstring=*this;
	if (!all)
	{
		const char * ptr=strstr(thisstring, instr);
		if (!ptr) return thisstring;
		string out=S substr(thisstring, (int)(ptr-thisstring.str))+outstr+(ptr+strlen(instr));
		thisstring =out;
		return thisstring;
	}
	//performance hack (obviously for ", "->"," in asar, but who cares, it's a performance booster)
	if (strlen(instr)==strlen(outstr)+1 && !memcmp(instr, outstr, strlen(outstr)))
	{
		const char * indat= thisstring;
		char * trueoutdat=malloc(char, strlen(indat)+1);
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
			if (!memcmp(in, instr, (size_t)inlen))
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
	if (!strchr(thisstring, '"'))
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
			if (!memcmp((const char*)thisstring +i, instr, strlen(instr)))
			{
				replaced=true;
				out+=outstr;
				i+=(int)strlen(instr);
				if (!all)
				{
					out+=((const char*)thisstring)+i;
					thisstring =out;
					return thisstring;
				}
			}
			else out+= thisstring[i++];
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
		char ** out=malloc(char*, 2);
		out[0]=str;
		out[1]=0;
		if (len) *len=1;
		return out;
	}
	int keylen=(int)strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		if (!memcmp(thisentry, key, (size_t)keylen))
		{
			count++;
			thisentry+=keylen;
		}
		else thisentry++;
	}
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata=malloc(char*, count+1);
	if (len) *len=count;
	int newcount=0;
	thisentry=str;
	outdata[newcount++]=thisentry;
	while (newcount<count)
	{
		if (!memcmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
		}
		else thisentry++;
	}
	outdata[newcount]=NULL;
	return outdata;
}

char ** qnsplit(char * str, const char * key, int maxlen, int * len)
{
	if (!strchr(str, '"')) return nsplit(str, key, maxlen, len);
	int keylen=(int)strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		dequote(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, (size_t)keylen))
		{
			count++;
			thisentry+=keylen;
		}
		else thisentry++;
	}
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata=malloc(char*, count+1);
	if (len) *len=count;
	int newcount=0;
	thisentry=str;
	outdata[newcount++]=thisentry;
	while (newcount<count)
	{
		dequote(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
		}
		else thisentry++;
	}
	outdata[newcount]=NULL;
	return outdata;
}

char ** qpnsplit(char * str, const char * key, int maxlen, int * len)
{
	int keylen=(int)strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		skippar(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, (size_t)keylen))
		{
			count++;
			thisentry+=keylen;
		}
		else thisentry++;
	}
	if (maxlen && count>maxlen) count=maxlen;
	char ** outdata=malloc(char*, count+1);
	if (len) *len=count;
	int newcount=0;
	thisentry=str;
	outdata[newcount++]=thisentry;
	while (newcount<count)
	{
		skippar(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, (size_t)keylen))
		{
			*thisentry=0;
			thisentry+=keylen;
			outdata[newcount++]=thisentry;
		}
		else thisentry++;
	}
	outdata[newcount]=NULL;
	return outdata;
}

char * trim(char * str, const char * left, const char * right, bool multi)
{
	bool nukeright=true;
	int totallen=(int)strlen(str);
	int rightlen=(int)strlen(right);
	if (rightlen<=totallen)
	{
		do
		{
			const char * rightend=right+rightlen;
			char * strend=str+totallen;
			while (right!=rightend)
			{
				rightend--;
				strend--;
				if (*strend!=*rightend) nukeright=false;
			}
			if (nukeright)
			{
				totallen-=rightlen;
				str[totallen]=0;
			}
		} while (multi && nukeright && rightlen<=totallen);
	}
	bool nukeleft=true;
	do
	{
		int leftlen;
		for (leftlen=0;left[leftlen];leftlen++)
		{
			if (str[leftlen]!=left[leftlen]) nukeleft=false;
		}
		if (nukeleft) memmove(str, str+leftlen, (size_t)(totallen-leftlen+1));
	} while (multi && nukeleft);
	return str;
}

char * itrim(char * str, const char * left, const char * right, bool multi)
{
	bool nukeright=true;
	int totallen=(int)strlen(str);
	int rightlen=(int)strlen(right);
	if (rightlen<=totallen)
	{
		do
		{
			const char * rightend=right+rightlen;
			char * strend=str+totallen;
			while (right!=rightend)
			{
				rightend--;
				strend--;
				if (tolower(*strend)!=tolower(*rightend)) nukeright=false;
			}
			if (nukeright)
			{
				totallen-=rightlen;
				str[totallen]=0;
			}
		} while (multi && nukeright && rightlen<=totallen);
	}
	bool nukeleft=true;
	do
	{
		int leftlen;
		for (leftlen=0;left[leftlen];leftlen++)
		{
			if (tolower(str[leftlen])!=tolower(left[leftlen])) nukeleft=false;
		}
		if (nukeleft) memmove(str, str+leftlen, (size_t)(totallen-leftlen+1));
	} while (multi && nukeleft);
	return str;
}

