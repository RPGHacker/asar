#include "asar.h"
#include "errors.h"
#include "assembleblock.h"
#include "asar_math.h"

#include "arch-shared.h"

#define write1 write1_pick

void asinit_superfx()
{
}

void asend_superfx()
{
}

static int64_t getnum_ck(const char* math)
{
	return pass == 2 ? getnum(math) : 0;
}

static void range(int min, int mid, int max)
{
	if (mid<min || mid>max) asar_throw_error(0, error_type_block, error_id_superfx_invalid_register, min, max);
}

enum reg_t {
	reg_parr,
	reg_r,
	reg_hash,
};
static bool getreg(const char * par, int * reg, reg_t type)
{
	int ret;
	*reg=-1;
	if (type==reg_parr && *par++!='(') return false;
	if (type==reg_parr && to_lower(*par++)!='r') return false;
	if (type==reg_r && to_lower(*par++)!='r') return false;
	if (type==reg_hash && *par++!='#') return false;
	if (!is_digit(par[0])) return false;
	if (is_digit(par[1]))
	{
		if (par[0]!='1' || par[1]>'5') return false;
		ret=par[1]-'0'+10;
		par+=2;
	}
	else
	{
		ret=par[0]-'0';
		par+=1;
	}
	if (type==reg_parr && *par++!=')') return false;
	if (*par) return false;
	*reg=ret;
	return true;
}

//for LMS and SMS short addressing forms, check range & evenness
static bool check_short_addr(int num) {
	if (num % 2 > 0 || num < 0 || num > 0x1FE) {
		asar_throw_error(0, error_type_block, error_id_superfx_invalid_short_address, hex((unsigned int)num).data());
		return false;
	}
	return true;
}

bool asblock_superfx(char** word, int numwords)
{
#define is(test) (!stricmp(word[0], test))
	char * par= nullptr;
	if (word[1]) par= duplicate_string(word[1]);
	autoptr<char*> parptr=par;
	if(0);
	else if (assemblemapper(word, numwords)) return true;
	else if (numwords==1)
	{
#define op(from, to) if (is(from)) { write1(to); return true; }
#define op3d(from, to) if (is(from)) { write1(0x3D); write1(to); return true; }
#define op3e(from, to) if (is(from)) { write1(0x3E); write1(to); return true; }
#define op3f(from, to) if (is(from)) { write1(0x3F); write1(to); return true; }
		op("STOP", 0x00);
		op("NOP", 0x01);
		op("CACHE", 0x02);
		op("LSR", 0x03);
		op("ROL", 0x04);
		op("LOOP", 0x3C);
		op("ALT1", 0x3D);
		op("ALT2", 0x3E);
		op("ALT3", 0x3F);
		op("PLOT", 0x4C);
		op("SWAP", 0x4D);
		op("COLOR", 0x4E);
		op("NOT", 0x4F);
		op("MERGE", 0x70);
		op("SBK", 0x90);
		op("SEX", 0x95);
		op("ASR", 0x96);
		op("ROR", 0x97);
		op("LOB", 0x9E);
		op("FMULT", 0x9F);
		op("HIB", 0xC0);
		op("GETC", 0xDF);
		op("GETB", 0xEF);
		op3d("RPIX", 0x4C);
		op3d("CMODE", 0x4E);
		op3d("DIV2", 0x96);
		op3d("LMULT", 0x9F);
		op3d("GETBH", 0xEF);
		op3e("RAMB", 0xDF);
		op3e("GETBL", 0xEF);
		op3f("ROMB", 0xDF);
		op3f("GETBS", 0xEF);
#undef op
#undef op3d
#undef op3e
#undef op3f
		return false;
	}
	else if (numwords==2)
	{
		string tmp=par;
		int numwordsinner;
		autoptr<char*> parcpy= duplicate_string(par);
		autoptr<char**> arg=qpsplit(parcpy, ",", &numwordsinner);
		bool ret=false;
#define ok() ret=true
#define op(op) if (is(op)) ok()
#define w3d(val) ,write1(0x3D) w(val)
#define w3e(val) ,write1(0x3E) w(val)
#define w3f(val) ,write1(0x3F) w(val)
		if (numwordsinner ==1)
		{
#define w(val) ,write1((unsigned int)(val+reg))
#define reg_range(min, max) ,range(min, reg, max)
			int reg;
			if (getreg(par, &reg, reg_r))
			{
				op("TO")                    w(0x10);
				op("WITH")                  w(0x20);
				op("ADD")                   w(0x50);
				op("SUB")                   w(0x60);
				op("AND")  reg_range(1, 15) w(0x70);
				op("MULT")                  w(0x80);
				op("JMP")  reg_range(8, 13) w(0x90);
				op("FROM")                  w(0xB0);
				op("OR")   reg_range(1, 15) w(0xC0);
				op("INC")  reg_range(0, 14) w(0xD0);
				op("DEC")  reg_range(0, 14) w(0xE0);

				op("ADC")                    w3d(0x50);
				op("SBC")                    w3d(0x60);
				op("BIC")   reg_range(1, 15) w3d(0x70);
				op("UMULT")                  w3d(0x80);
				op("LJMP")  reg_range(8, 13) w3d(0x90);
				op("XOR")   reg_range(1, 15) w3d(0xC0);

				op("CMP") w3f(0x60);
			}
			if (getreg(par, &reg, reg_hash))
			{
				op("LINK") reg_range(1, 4) w(0x90);

				op("ADD")                   w3e(0x50);
				op("SUB")                   w3e(0x60);
				op("AND")  reg_range(1, 15) w3e(0x70);
				op("MULT")                  w3e(0x80);
				op("OR")   reg_range(1, 15) w3e(0xC0);

				op("ADC")                    w3f(0x50);
				op("BIC")   reg_range(1, 15) w3f(0x70);
				op("UMULT")                  w3f(0x80);
				op("XOR")   reg_range(1, 15) w3f(0xC0);
			}
			if (getreg(par, &reg, reg_parr))
			{
				op("STW") reg_range(0, 11) w(0x30);
				op("LDW") reg_range(0, 11) w(0x40);
				op("STB") reg_range(0, 11) w3d(0x30);
				op("LDB") reg_range(0, 11) w3d(0x40);
			}
#undef w
#undef reg_range
			int byte=-1;
#define br(name, val) if (is(name)) byte=val;
			br("BRA", 0x05);
			br("BGE", 0x06);
			br("BLT", 0x07);
			br("BNE", 0x08);
			br("BEQ", 0x09);
			br("BPL", 0x0A);
			br("BMI", 0x0B);
			br("BCC", 0x0C);
			br("BCS", 0x0D);
			br("BVC", 0x0E);
			br("BVS", 0x0F);
#undef br
			if (byte!=-1)
			{
				ret=true;
				int len=getlen(par);
				unsigned int num=getnum_ck(par);
				if (len==1)
				{
					write1((unsigned int)byte); write1(num);
				}
				else
				{
					int pos=(int)getnum_ck(par)-((snespos&0xFFFFFF)+2);
					write1((unsigned int)byte); write1((unsigned int)pos);
					if (pass==2 && (pos<-128 || pos>127))
					{
						asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(pos).data());
					}
				}
			}
		}
		if (numwordsinner==2)
		{
#define w(val) ,write1((unsigned int)(val))
			int reg1; bool isreg1=getreg(arg[0], &reg1, reg_r);
			int reg2; bool isreg2=getreg(arg[1], &reg2, reg_r);
			if (isreg1)
			{
				if (isreg2)
				{
					op("MOVE") w(0x20+reg2) w(0x10+reg1);
					op("MOVES") w(0x20+reg1) w(0xB0+reg2);
				}
				if (arg[1][0]=='#')
				{
					unsigned int num=getnum_ck(arg[1]+1);
					num&=0xFFFF;
					op("IBT") w(0xA0+reg1) w(num);
					op("IWT") w(0xF0+reg1) w(num) w(num>>8);
					if (num<0x80 || num>=0xFF80)
					{
						op("MOVE") w(0xA0+reg1) w(num);
					}
					else
					{
						op("MOVE") w(0xF0+reg1) w(num) w(num>>8);
					}
				}
				if (getreg(arg[1], &reg2, reg_parr))
				{
					if (reg1==0)
					{
						op("MOVEB") w(0x3D) w(0x40+reg2);
						op("MOVEW") w(0x40+reg2);
					}
					else
					{
						op("MOVEB") w(0x10+reg1) w(0x3D) w(0x40+reg2);
						op("MOVEW") w(0x10+reg1) w(0x40+reg2);
					}
				}
				else if (arg[1][0]=='(')
				{
					char * endpar=strchr(arg[1], ')');
					if (!endpar || endpar[1]) return false;
					unsigned int num=getnum_ck(arg[1]);
					op("LM") w(0x3D) w(0xF0+reg1) w(num) w(num>>8);

					if (is("LMS")) {
						ok();
						if (check_short_addr((int)num))
						{
							ok() w(0x3D) w(0xA0+reg1) w(num>>1);
						}
					}

					if (num&1 || num>=0x200)
					{
						op("MOVE") w(0x3D) w(0xF0+reg1) w(num) w(num>>8);
					}
					else
					{
						op("MOVE") w(0x3D) w(0xA0+reg1) w(num);
					}
				}
				if (is("LEA"))
				{
					unsigned int num=getnum_ck(arg[1]);
					ok() w(0xF0+reg1) w(num) w(num>>8);
				}
			}
			else if (isreg2)
			{
				if (getreg(arg[0], &reg1, reg_parr))
				{
					if (reg1==0)
					{
						op("MOVEB") w(0x3D) w(0x30+reg2);
						op("MOVEW") w(0x30+reg2);
					}
					else
					{
						op("MOVEB") w(0xB0+reg1) w(0x3D) w(0x30+reg2);
						op("MOVEW") w(0xB0+reg1) w(0x30+reg2);
					}
				}
				else if (arg[0][0]=='(')
				{
					char * endpar=strchr(arg[0], ')');
					if (!endpar || endpar[1]) return false;
					unsigned int num=getnum(arg[0]);
					op("SM") w(0x3E) w(0xF0+reg2) w(num) w(num>>8);

					if (is("SMS"))
					{
						ok();
						if (check_short_addr((int)num))
						{
							ok() w(0x3E) w(0xA0+reg2) w(num>>1);
						}
					}

					if (num&1 || num>=0x200)
					{
						op("MOVE") w(0x3E) w(0xF0+reg2) w(num) w(num>>8);
					}
					else
					{
						op("MOVE") w(0x3E) w(0xA0+reg2) w(num);
					}
				}
			}
		}
#undef ok
#undef op
#undef w3d
#undef w3e
#undef w3f
		return ret;
	}
	return false;
}
