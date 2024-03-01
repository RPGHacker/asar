#include "asar.h"
#include "warnings.h"
#include "errors.h"
#include "assembleblock.h"
#include "asar_math.h"

#include "arch-shared.h"

#define write1 write1_pick

static int writesizeto=-1;
static int inlinestartpos=0;

static int64_t getnum_ck(const char* math)
{
	return pass == 2 ? getnum(math) : 0;
}

static void inline_finalizeorg()
{
	if (writesizeto>=0 && pass==2)
	{
		int pcpos=snestopc(writesizeto&0xFFFFFF);
		if (pcpos<0) asar_throw_error(2, error_type_block, error_id_snes_address_doesnt_map_to_rom, hex6((unsigned int)realsnespos).data());
		int num=snespos-startpos;
		writeromdata_byte(pcpos, (unsigned char)num);
		writeromdata_byte(pcpos+1, (unsigned char)(num >> 8));
	}
	writesizeto=-1;
}

static void inline_org(unsigned int num)
{
	inline_finalizeorg();
	if (num&~0xFFFF) asar_throw_error(0, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).data());
	writesizeto=realsnespos;
	write2(0x0000);
	write2(num);
	snespos=(int)num;
	startpos=(int)num;
}

static void inline_leavearch()
{
	inline_finalizeorg();
	write2(0x0000);
	write2((unsigned int)inlinestartpos);
}

void asinit_spc700()
{
}

void asend_spc700()
{
	if (arch==arch_spc700_inline) inline_leavearch();
}

static bool matchandwrite(const char * str, const char * left, const char * right, string& remainder)
{
	for (int i=0;left[i];i++)
	{
		if (to_lower(*str)!=left[i]) return false;
		str++;
	}
	int mainlen=(int)(strlen(str)-strlen(right));
	if(mainlen < 0) return false;
	for (int i=0;right[i];i++)
	{
		if (to_lower(str[mainlen+i])!=right[i]) return false;
	}
	remainder=substr(str, mainlen);
	return true;
}

static bool bitmatch(const char * opnamein, string& opnameout, const char * str, string& math, int& bit)
{
	const char * opnameend=strchr(opnamein, '\0');
	const char * dot=strqrchr(str, '.');
	if (dot && is_digit(dot[1]) && !dot[2])
	{
		bit=atoi(dot+1);
		if (bit>=8) return false;
		math=substr(str, (int)(dot-str));
		if (opnameend[-1]=='1') opnameout=substr(opnamein, (int)(opnameend-opnamein-1));
		else opnameout=opnamein;
		return true;
	}
	if (opnameend[-1]>='0' && opnameend[-1]<='7')
	{
		math=str;
		bit=opnameend[-1]-'0';
		opnameout=substr(opnamein, (int)(opnameend-opnamein-1));
		return true;
	}
	return false;
}

#define isop(test) (!stricmp(op, test))
static bool assinglebitwithc(const char * op, const char * math, int bits)
{
	unsigned int num;
	if (math[0]=='!')
	{
		if(0);
		else if (isop("or")) write1(0x2A);
		else if (isop("and")) write1(0x6A);
		else return false;
		num=getnum_ck(math+1);
	}
	else
	{
		if(0);
		else if (isop("or")) write1(0x0A);
		else if (isop("and")) write1(0x4A);
		else if (isop("eor")) write1(0x8A);
		else if (isop("mov")) write1(0xAA);
		else if (isop("not")) write1(0xEA);
		else return false;
		num=getnum_ck(math);
	}
	if (num>=0x2000) asar_throw_error(2, error_type_block, error_id_spc700_addr_out_of_range, hex4(num).data());
	write2(((unsigned int)bits<<13)|num);
	return true;
}
#undef isop

bool asblock_spc700(char** word, int numwords)
{
#define is(test) (!stricmp(word[0], test))
#define is1(test) (!stricmp(word[0], test) && numwords==2)
#define par word[1]
	if(0);
	else if (arch==arch_spc700_inline && is1("org"))
	{
		unsigned int num=getnum(par);
		if (foundlabel) asar_throw_error(0, error_type_block, error_id_org_label_invalid);
		inline_org(num);
	}
	else if (arch==arch_spc700_inline && is1("arch"))
	{
		inline_leavearch();
		return false;
	}
	else if (arch==arch_spc700_inline && is1("skip"))
	{
		int num=snespos+(int)getnum(par);
		if (foundlabel) asar_throw_error(0, error_type_block, error_id_skip_label_invalid);
		inline_org(num);
	}
	else if (arch==arch_spc700_inline && is1("base"))
	{
		asar_throw_error(0, error_type_block, error_id_spc700_inline_no_base);
	}
	else if (arch==arch_spc700_inline && is1("startpos"))
	{
		inlinestartpos=(int)getnum_ck(par);
	}
	else if (numwords==1)
	{
#define op(name, val) else if (is(name)) do { write1(val); } while(0)
		if(0);
		op("nop", 0x00);
		op("brk", 0x0F);
		op("clrp", 0x20);
		op("setp", 0x40);
		op("clrc", 0x60);
		op("ret", 0x6F);
		op("reti", 0x7F);
		op("setc", 0x80);
		op("ei", 0xA0);
		op("di", 0xC0);
		op("clrv", 0xE0);
		op("notc", 0xED);
		op("sleep", 0xEF);
		op("stop", 0xFF);
		op("xcn", 0x9F);
		else return false;
#undef op
	}
	else if (numwords==2)
	{
		int numwordsinner;
		//Detect opcode length before continuing
		int opLen=0; //In case of .b or .w, this overwrites auto-detection of opcode length
		unsigned int periodLocCount=0;
		do {
			if (word[0][periodLocCount] == '.') {
				opLen=getlenfromchar(word[0][periodLocCount+1]);
				word[0][periodLocCount]='\0';
			}
			periodLocCount++;
		} while ((opLen == 0) && (periodLocCount < strlen(word[0])));
		if (opLen > 2) { asar_throw_error(0, error_type_block, error_id_opcode_length_too_long); }
		autoptr<char*> parcpy= duplicate_string(par);
		autoptr<char**> arg=qpsplit(parcpy, ",", &numwordsinner);
		if (numwordsinner ==1)
		{
			string op;
			string math;
			int bits;
#define isop(str) (!stricmp(word[0], str))
#define isam(str) (!stricmp(arg[0], str))
#define ismatch(left, right) (matchandwrite(arg[0], left, right, math))
#define eq(str) if (isam(str))
#define w0(hex) do { write1((unsigned int)hex); return true; } while(0)
#define w1(hex) do { write1((unsigned int)hex); write1(getnum_ck(math)); return true; } while(0)
#define w2(hex) do { write1((unsigned int)hex); write2(getnum_ck(math)); return true; } while(0)
#define wv(hex1, hex2) do { if ((opLen == 1) || (opLen == 0 && getlen(math) == 1)) { write1((unsigned int)hex1); write1(getnum_ck(math)); } else { write1((unsigned int)hex2); write2(getnum_ck(math)); } return true; } while(0)
#define wr(hex) do { int len=getlen(math); int num=(int)getnum_ck(math); int pos=(len==1)?num:num-((snespos&0xFFFFFF)+2); write1((unsigned int)hex); write1((unsigned int)pos); \
								if (pass==2 && foundlabel && (pos<-128 || pos>127)) asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(pos).data()); \
								return true; } while(0)
#define op0(str, hex) if (isop(str)) w0(hex)
#define op1(str, hex) if (isop(str)) w1(hex)
#define op2(str, hex) if (isop(str)) w2(hex)
#define opv(str, hex1, hex2) if (isop(str)) wv(hex1, hex2)
#define opr(str, hex) if (isop(str)) wr(hex)
#define match(left, right) if (ismatch(left, right))
			eq("a")
			{
				op0("asl", 0x1C);
				op0("das", 0xBE);
				op0("daa", 0xDF);
				op0("dec", 0x9C);
				op0("inc", 0xBC);
				op0("lsr", 0x5C);
				op0("pop", 0xAE);
				op0("push", 0x2D);
				op0("rol", 0x3C);
				op0("ror", 0x7C);
				op0("xcn", 0x9F);
			}
			eq("x")
			{
				op0("dec", 0x1D);
				op0("inc", 0x3D);
				op0("pop", 0xCE);
				op0("push", 0x4D);
			}
			eq("y")
			{
				op0("dec", 0xDC);
				op0("inc", 0xFC);
				op0("pop", 0xEE);
				op0("push", 0x6D);
			}
			eq("p")
			{
				op0("pop", 0x8E);
				op0("push", 0x0D);
			}
			if (isop("mul") && isam("ya")) w0(0xCF);
			if (isop("jmp") && ismatch("(", "+x)")) w2(0x1F);
			match("", "+x")
			{
				op1("asl", 0x1B);
				op1("dec", 0x9B);
				op1("inc", 0xBB);
				op1("lsr", 0x5B);
				op1("rol", 0x3B);
				op1("ror", 0x7B);
			}
			if (bitmatch(word[0], op, arg[0], math, bits))
			{
				if (assinglebitwithc(op, math, bits)) return true;
				else if (!stricmp(op, "set")) write1((unsigned int)(0x02|(bits<<5)));
				else if (!stricmp(op, "clr")) write1((unsigned int)(0x12|(bits<<5)));
				else return false;
				unsigned int num=getnum_ck(math);
				if (num>=0x100) asar_throw_error(2, error_type_block, error_id_snes_address_out_of_bounds, hex6(num).data());
				write1(num);
				return true;
			}
			if (true)
			{
				math=arg[0];
				if (isop("tcall"))
				{
					unsigned int num = getnum_ck(math);
					if (num >= 16) asar_throw_error(2, error_type_block, error_id_invalid_tcall);
					write1(((num<<4)|1));
					return true;
				}
				opv("asl", 0x0B, 0x0C);
				opv("dec", 0x8B, 0x8C);
				opv("inc", 0xAB, 0xAC);
				opv("lsr", 0x4B, 0x4C);
				opv("rol", 0x2B, 0x2C);
				opv("ror", 0x6B, 0x6C);
				op2("jmp", 0x5F);
				op2("call", 0x3F);
				op1("decw", 0x1A);
				op1("incw", 0x3A);
				op1("pcall", 0x4F);
				opr("bpl", 0x10);
				opr("bra", 0x2F);
				opr("bmi", 0x30);
				opr("bvc", 0x50);
				opr("bvs", 0x70);
				opr("bcc", 0x90);
				opr("bcs", 0xB0);
				opr("bne", 0xD0);
				opr("beq", 0xF0);
			}
#undef isop
#undef isam
#undef eq
#undef w0
#undef w1
#undef w2
#undef wv
#undef wr
#undef op0
#undef op1
#undef op2
#undef opv
#undef opr
#undef match
			return false;
		}
		if (numwordsinner==2)
		{
#define iscc(str1, str2) (!stricmp(arg[0], str1) && !stricmp(arg[1], str2))
#define iscv(str1, left2, right2) (!stricmp(arg[0], str1) && matchandwrite(arg[1], left2, right2, s2))
#define isvc(left1, right1, str2) (matchandwrite(arg[0], left1, right1, s1) && !stricmp(arg[1], str2))
#define isvv(left1, right1, left2, right2) (matchandwrite(arg[0], left1, right1, s1) && matchandwrite(arg[1], left2, right2, s2))
#define cc(str1, str2) if (iscc(str1, str2))
#define cv(str1, left2, right2) if (iscv(str1, left2, right2))
#define vc(left1, right1, str2) if (isvc(left1, right1, str2))
#define vv(left1, right1, left2, right2) if (isvv(left1, right1, left2, right2))
#define w0(opcode) do { write1((unsigned int)opcode); return true; } while(0)
#define w1(opcode, math) \
	do { \
		write1((unsigned int)opcode); \
		unsigned int val=getnum_ck(math); \
		if ((((val&0xFF00) && (val&0x80000000) == 0) || (((val&0xFF00) != 0xFF00) && (val&0x80000000))) && opLen != 1) \
			asar_throw_warning(2, warning_id_spc700_assuming_8_bit); \
		write1(val);\
		return true; \
	} while(0)
#define w2(opcode, math) do { write1((unsigned int)opcode); write2(getnum_ck(math)); return true; } while(0)
#define wv(opcode1, opcode2, math) do { if ((opLen == 1) || (opLen == 0 && getlen(math)==1)) { write1((unsigned int)opcode1); write1(getnum_ck(math)); } \
																	 else { write1((unsigned int)opcode2); write2(getnum_ck(math)); } return true; } while(0)
#define w11(opcode, math1, math2) do { write1((unsigned int)opcode); write1(getnum_ck(math1)); write1(getnum_ck(math2)); return true; } while(0)
#define wr(opcode, math) do { int len=getlen(math); int num=(int)getnum_ck(math); int pos=(len==1)?num:num-(snespos+2); \
								if (pass && foundlabel && (pos<-128 || pos>127)) asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(pos).data()); \
								write1((unsigned int)opcode); write1((unsigned int)pos); return true; } while(0)
#define w1r(opcode, math1, math2) do { int len=getlen(math2); int num=(int)getnum_ck(math2); int pos=(len==1)?num:num-(snespos+3); \
								if (pass && foundlabel && (pos<-128 || pos>127)) asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(pos).data()); \
								write1((unsigned int)opcode); write1(getnum_ck(math1)); write1((unsigned int)pos); return true; } while(0)
			string s1;
			string s2;
			string op;
			string math;
			int bits;
#define isop(test) (!stricmp(op, test))
			if (!stricmp(arg[0], "c") && bitmatch(word[0], op, arg[1], math, bits))
			{
				if (assinglebitwithc(op, math, bits)) return true;
			}
			if (bitmatch(word[0], op, arg[0], s1, bits))
			{
				if (isop("mov") && !stricmp(arg[1], "c"))
				{
					unsigned int num=getnum_ck(s1);
					if (num>=0x2000) asar_throw_error(2, error_type_block, error_id_snes_address_out_of_bounds, hex6((unsigned int)num).data());
					write1(0xCA);
					write2(((unsigned int)bits<<13)|num);
					return true;
				}
				int pos = (getnum_ck(arg[1])- (unsigned int)(snespos)-3);
				if (pass==2 && (pos<-128 || pos>127)) {
					asar_throw_error(1, error_type_block, error_id_relative_branch_out_of_bounds, dec(pos).data());
				}
				if(0);
				else if (isop("bbs")) write1((unsigned int)(0x03|(bits<<5)));
				else if (isop("bbc")) write1((unsigned int)(0x13|(bits<<5)));
				else return false;
				unsigned int num=getnum_ck(s1);
				if (num>=0x100) asar_throw_error(2, error_type_block, error_id_snes_address_out_of_bounds, hex6(num).data());
				write1(num);
				write1(pos);
				return true;
			}
#undef isop
			if (is("mov"))
			{
				if (iscc("(x)+", "a")) asar_throw_error(0, error_type_block, error_id_use_xplus);
				cc("(x+)"   , "a"      ) w0(0xAF);
				cc("(x)"    , "a"      ) w0(0xC6);
				if (iscc("a", "(x)+")) asar_throw_error(0, error_type_block, error_id_use_xplus);
				cc("a"      , "(x+)"   ) w0(0xBF);
				cc("a"      , "(x)"    ) w0(0xE6);
				cc("a"      , "x"      ) w0(0x7D);
				cc("a"      , "y"      ) w0(0xDD);
				cc("x"      , "a"      ) w0(0x5D);
				cc("x"      , "sp"     ) w0(0x9D);
				cc("y"      , "a"      ) w0(0xFD);
				cc("sp"     , "x"      ) w0(0xBD);

				vc("(","+x)", "a"      ) w1(0xC7, s1);
				vc("(",")+y", "a"      ) w1(0xD7, s1);
				vc("","+x"  , "a"      ) wv(0xD4, 0xD5, s1);
				vc("","+y"  , "a"      ) w2(0xD6, s1);
				vc("",""    , "a"      ) wv(0xC4, 0xC5, s1);
				vc("","+x"  , "y"      ) w1(0xDB, s1);
				vc("","+y"  , "x"      ) w1(0xD9, s1);
				vc("",""    , "x"      ) wv(0xD8, 0xC9, s1);
				vc("",""    , "y"      ) wv(0xCB, 0xCC, s1);

				cv("a"      , "#",""   ) w1(0xE8, s2);
				cv("a"      , "(","+x)") w1(0xE7, s2);
				cv("a"      , "(",")+y") w1(0xF7, s2);
				cv("a"      , "","+x"  ) wv(0xF4, 0xF5, s2);
				cv("a"      , "","+y"  ) w2(0xF6, s2);
				cv("a"      , "",""    ) wv(0xE4, 0xE5, s2);
				cv("x"      , "#",""   ) w1(0xCD, s2);
				cv("x"      , "","+y"  ) w1(0xF9, s2);
				cv("x"      , "",""    ) wv(0xF8, 0xE9, s2);
				cv("y"      , "#",""   ) w1(0x8D, s2);
				cv("y"      , "","+x"  ) w1(0xFB, s2);
				cv("y"      , "",""    ) wv(0xEB, 0xEC, s2);

				vv("",""    , "#",""   ) w11(0x8F, s2, s1);
				vv("",""    , "",""    ) w11(0xFA, s2, s1);
			}
			if (is("cmp"))
			{
				cv("x", "#","") w1(0xC8, s2);
				cv("x", "","" ) wv(0x3E, 0x1E, s2);
				cv("y", "#","") w1(0xAD, s2);
				cv("y", "","" ) wv(0x7E, 0x5E, s2);
			}
			if (is("or") || is("and") || is("eor") || is("cmp") || is("adc") || is("sbc"))
			{
				int offset = 0;
				if (is("or" )) offset=0x00;
				if (is("and")) offset=0x20;
				if (is("eor")) offset=0x40;
				if (is("cmp")) offset=0x60;
				if (is("adc")) offset=0x80;
				if (is("sbc")) offset=0xA0;

				cc("a"  , "(x)"    ) w0(offset+0x06);
				cc("(x)", "(y)"    ) w0(offset+0x19);

				cv("a"  , "#",""   ) w1(offset+0x08, s2);
				cv("a"  , "(","+x)") w1(offset+0x07, s2);
				cv("a"  , "(",")+y") w1(offset+0x17, s2);
				cv("a"  , "","+x"  ) wv(offset+0x14, offset+0x15, s2);
				cv("a"  , "","+y"  ) w2(offset+0x16, s2);
				cv("a"  , "",""    ) wv(offset+0x04, offset+0x05, s2);

				vv("","", "#",""   ) w11(offset+0x18, s2, s1);
				vv("","", "",""    ) w11(offset+0x09, s2, s1);
			}
			vc("","", "a")
			{
				if (is("tset")) w2(0x0E, s1);
				if (is("tclr")) w2(0x4E, s1);
			}
			if (is("div") && iscc("ya", "x")) w0(0x9E);
			cv("ya", "","")
			{
				if (is("cmpw")) w1(0x5A, s2);
				if (is("addw")) w1(0x7A, s2);
				if (is("subw")) w1(0x9A, s2);
				if (is("movw")) w1(0xBA, s2);
			}
			if (is("movw") && isvc("","", "ya")) w1(0xDA, s1);
			if (is("cbne") && isvv("","+x", "","")) w1r(0xDE, s1, s2);
			if (is("dbnz") && iscv("y", "","")) wr(0xFE, s2);
			vv("","", "","")
			{
				if (is("dbnz")) w1r(0x6E, s1, s2);
				if (is("cbne")) w1r(0x2E, s1, s2);
			}
#undef iscc
#undef iscv
#undef isvc
#undef isvv
#undef cc
#undef cv
#undef vc
#undef vv
#undef w0
#undef w1
#undef w2
#undef wv
#undef w11
			return false;
		}
		return false;
	}
	else return false;
	return true;
}
