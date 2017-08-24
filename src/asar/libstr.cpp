#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libstr.h"

#define malloc(type, count) (type*)malloc(sizeof(type)*(count))
#define realloc(val, type, count) (type*)realloc(val, sizeof(type)*(count))

char * readfile(const char * fname)
{
	FILE * myfile=fopen(fname, "rt");
	if (!myfile) return NULL;
	fseek(myfile, 0, SEEK_END);
	int datalen=ftell(myfile);
	fseek(myfile, 0, SEEK_SET);
	char * data=malloc(char, datalen+1);
	data[fread(data, 1, datalen, myfile)]=0;
	fclose(myfile);
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

bool readfile(const char * fname, char ** data, int * len)
{
	FILE * myfile=fopen(fname, "rb");
	if (!myfile) return false;
	fseek(myfile, 0, SEEK_END);
	int datalen=ftell(myfile);
	fseek(myfile, 0, SEEK_SET);
	*data=malloc(char, datalen);
	*len=(int)fread(*data, 1, datalen, myfile);
	fclose(myfile);
	return true;
}

#define dequote(var, next, error) if (var=='"') do { next; while (var!='"') { if (!var) error; next; } next; } while(0)
#define skippar(var, next, error) dequote(var, next, error); else if (var=='(') { int par=1; next; while (par) { dequote(var, next, error); \
				if (var=='(') par++; if (var==')') par--; if (!var) error; next; } } else if (var==')') error

string& string::replace(const char * instr, const char * outstr, bool all)
{
	string& str=*this;
	if (!all)
	{
		const char * ptr=strstr(str, instr);
		if (!ptr) return str;
		string out=S substr(str, ptr-str.str)+outstr+(ptr+strlen(instr));
		str=out;
		return str;
	}
	//performance hack (obviously for ", "->"," in asar, but who cares, it's a performance booster)
	if (strlen(instr)==strlen(outstr)+1 && !memcmp(instr, outstr, strlen(outstr)))
	{
		const char * indat=str;
		char * trueoutdat=malloc(char, strlen(indat)+1);
		char * outdat=trueoutdat;
		int thelen=strlen(outstr);
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
			str=trueoutdat;
			free(trueoutdat);
			return str;
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
			str=trueoutdat;
			free(trueoutdat);
			return str;
		}
		else
		//end hack
		{
			char thehatedchar=instr[thelen];
			while (*indat)
			{
				if (!memcmp(indat, outstr, thelen))
				{
					memcpy(outdat, outstr, thelen);
					outdat+=thelen;
					indat+=thelen;
					while (*indat==thehatedchar) indat++;
				}
				else *outdat++=*indat++;
			}
		}
		*outdat=0;
		str=trueoutdat;
		free(trueoutdat);
		return str;
	}
	//end hack
	bool replaced=true;
	while (replaced)
	{
		replaced=false;
		string out;
		const char * in=str;
		int inlen=strlen(instr);
		while (*in)
		{
			if (!memcmp(in, instr, inlen))
			{
				replaced=true;
				out+=outstr;
				in+=inlen;
			}
			else out+=*in++;
		}
		str=out;
	}
	return str;
}

string& string::qreplace(const char * instr, const char * outstr, bool all)
{
	string& str=*this;
	if (!strstr(str, instr)) return str;
	if (!strchr(str, '"'))
	{
		str.replace(instr, outstr, all);
		return str;
	}
	bool replaced=true;
	while (replaced)
	{
		replaced=false;
		string out;
		for (int i=0;str[i];)
		{
			dequote(str[i], out+=str[i++], return str);
			if (!memcmp((const char*)str+i, instr, strlen(instr)))
			{
				replaced=true;
				out+=outstr;
				i+=strlen(instr);
				if (!all)
				{
					out+=((const char*)str)+i;
					str=out;
					return str;
				}
			}
			else out+=str[i++];
		}
		str=out;
	}
	return str;
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
	int keylen=strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		if (!memcmp(thisentry, key, keylen))
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
		if (!memcmp(thisentry, key, keylen))
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
	int keylen=strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		dequote(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, keylen))
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
		else if (!memcmp(thisentry, key, keylen))
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
	int keylen=strlen(key);
	int count=1;
	char * thisentry=str;
	while (*thisentry)
	{
		skippar(*thisentry, thisentry++, return NULL);
		else if (!memcmp(thisentry, key, keylen))
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
		else if (!memcmp(thisentry, key, keylen))
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

string urlencode(const char * in)
{
	unsigned const char * uin=(unsigned const char*)in;
	string out;
	for (int i=0;uin[i];i++)
	{
		if (uin[i]==' ') out+='+';
		else if (isalnum(uin[i]) && uin[i]<128) out+=(char)uin[i];
		else out+=S"%"+hex2(uin[i]);
	}
	return out;
}

char * trim(char * str, const char * left, const char * right, bool multi)
{
	bool nukeright=true;
	int totallen=strlen(str);
	int rightlen=strlen(right);
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
		if (nukeleft) memmove(str, str+leftlen, totallen-leftlen+1);
	} while (multi && nukeleft);
	return str;
}

char * itrim(char * str, const char * left, const char * right, bool multi)
{
	bool nukeright=true;
	int totallen=strlen(str);
	int rightlen=strlen(right);
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
		if (nukeleft) memmove(str, str+leftlen, totallen-leftlen+1);
	} while (multi && nukeleft);
	return str;
}

const char * unhtml(const char * html)
{
	static string str;
	str=html;
	char * out=str.str;
	char * in=str.str;
	bool space=true;
	while (*in)
	{
		if (*in=='\r' || *in=='\n' || *in==' ')
		{
			in++;
			if (!space) *out++=' ';
			space=true;
			continue;
		}
		space=false;
		if (!strncmp(in, "<br>", 4))
		{
			*out++='\n';
			space=true;
			in+=4;
		}
		else if (!strncmp(in, "<!--", 4))
		{
			char * end=strstr(in, "-->");
			if (end) in=end+3;
		}
		else if (!strncmp(in, "<a href=\"", 9))
		{
			//<a href="http://www.smwiki.net/wiki/RAM_Address/$7E:0012">here</a>.
			in+=9;
			//http://www.smwiki.net/wiki/RAM_Address/$7E:0012">here</a>.
			char * tmp=strchr(in, '"');
			if (!tmp) continue;
			if (strncmp(tmp, "\">here</a>", 10) && strncmp(tmp, "\" rel=\"nofollow\">here</a>", 25)) continue;
			*tmp=0;
			//http://www.smwiki.net/wiki/RAM_Address/$7E:0012@>here</a>.
			out+=sprintf(out, "at %s ", in);
			tmp=strchr(tmp+1, '>');
			if (!tmp)
			{
				out+=sprintf(out, "AAAAAAAAAAAA");
				continue;
			}
			in=tmp+9;
			if ((*in=='.' && in[1]=='\0') || !strncmp(in, ".<br>", 5) || (!strncmp(in, ".<!--", 5) && strstr(in, "-->") && strstr(in, "-->")[3]=='\0'))
			{
				in++;
				out--;
			}
		}
#define entity(from, to) else if (!strncmp(in, from, strlen(from))) do { *out++=to; in+=strlen(from); } while(0)
		entity("&lt;", '<');
		entity("&gt;", '>');
		entity("&amp;", '\"');
		entity("&#010;", '\n');
#undef entity
		else if (!strncmp(in, "&deg;", strlen("&deg;")))
		{
			*out++='\xC2';
			*out++='\xB0';
			in+=strlen("&deg;");
		}
		else *out++=*in++;
	}
	*out=0;
	return str;
}

static inline const char * htmlencode(const char * text, bool allowhtml)
{
	static char * ptrtmp=NULL;//weird trick to disarm destructors.
	free(ptrtmp);
	string out;
	out="";
	const char * in=text;
	bool b=false;
	bool u=false;
	int fc=-1;
	int bc=-1;
	bool formatted=false;
	bool sp=false;
	char urlend=0;
#define reformat() \
			if (*in!='\x02' && *in!='\x03' && *in!='\x0F' && *in!='\x1F' && allowhtml) \
			{                                                                          \
				if (formatted) out+="</span>";                                           \
				if (b || u || fc!=-1 || bc!=-1)                                          \
				{                                                                        \
					formatted=true;                                                        \
					const char * sp="";                                                    \
					out+="<span class=\"";                                                 \
					if (b) { out+=sp; out+="b"; sp=" "; }                                  \
					if (u) { out+=sp; out+="u"; sp=" "; }                                  \
					if (fc!=-1) { out+=sp; out+=S"fc"+dec(fc); sp=" "; }                   \
					if (bc!=-1) { out+=sp; out+=S"bc"+dec(bc); sp=" "; }                   \
					out+="\">";                                                            \
				}                                                                        \
				else formatted=false;                                                    \
			}
	while (*in)
	{
		if ((sp || in==text) && (!strncmp(in, "http://", 7) || !strncmp(in, "https://", 8)))
		{
			string url;
			while (true)
			{
				if ((*in=='.' || *in==',') && (in[1]==' ' || in[1]=='\0')) break;
				if (*in==' ' || *in=='\0') break;
				if (*in<0x20) url+="\xEF\xBF\xBD";//no formatting allowed in URLs
				else if (*in=='<') url+="&lt;";
				else if (*in=='>') url+="&gt;";
				else if (*in=='&') url+="&amp;";
				else if (*in=='"') url+="&quot;";
				else if (*in==')' && (in[1]==' ' || in[1]=='\0') && !strchr(url, '(')) break;
				else if (*in==urlend) break;//crappy way to handle file_get_contents("http://example.com/");
				else url+=*in;
				in++;
			}
			out+="<a href=\"";
			out+=url;
			out+="\">";
			out+=url;
			out+="</a>";
			continue;
		}
		char c=*in;
		in++;
		if (c=='\x02')//bold
		{
			b=!b;
			reformat();
		}
		else if (c=='\x03')//color
		{
			if (isdigit(*in))
			{
				fc=*in++-'0';
				if (isdigit(*in)) fc=(fc*10)+(*in++-'0');
				fc&=0x0F;
				if (*in==',')
				{
					in++;
					bc=*in++-'0';
					if (isdigit(*in)) bc=(bc*10)+(*in++-'0');
					bc&=0x0F;
				}
				reformat();
			}
		}
		else if (c=='\x09')//tab
		{
			out+='\x09';
		}
		else if (c=='\x0A')//linebreak
		{
			out+='\n';
		}
		else if (c=='\x0F')//restore
		{
			b=false;
			u=false;
			fc=-1;
			bc=-1;
			reformat();
		}
		else if (c=='\x1F')//underline
		{
			u=!u;
			reformat();
		}
		else if (c=='\x16')//reverse colors
		{
			int t=fc;
			fc=bc;
			bc=t;
			reformat();
		}
		else if ((unsigned char)c<0x20)
		{
			out+="\xEF\xBF\xBD";//use the replacement character for various junk
		}
		else
		{
			if(0);
			else if (c=='<') out+="&lt;";
			else if (c=='>') out+="&gt;";
			else if (c=='&') out+="&amp;";
			else if (c=='"') out+="&quot;";
			else out+=c;
			sp=!isalnum(c);
			urlend=c;
		}
	}
#undef reformat
	if (formatted) out+="</span>";
	if (strstr(text, "\xE2\x80\xAE")) out+="\xE2\x80\xAD";//right-to-left mark
	ptrtmp=strdup(out);
	return ptrtmp;
}

const char * tohtml(const char * text)
{
	return htmlencode(text, true);
}

const char * htmlentities(const char * text)
{
	return htmlencode(text, false);
}

