#include "asar.h"

#define error error<errblock>
#define write1 write1_pick

void asinit_65816()
{
}

void asend_65816()
{
}

extern int optimizeforbank;
extern bool fastrom;
extern bool emulatexkas;

bool asblock_65816(char** word, int numwords)
{
#define is(test) (!stricmp(word[0], test))
#define is0(test) (!stricmp(word[0], test) && numwords==1)
#define is1(test) (!stricmp(word[0], test) && numwords==2)
#define is2(test) (!stricmp(word[0], test) && numwords==3)
#define is3(test) (!stricmp(word[0], test) && numwords==4)
#define is4(test) (!stricmp(word[0], test) && numwords==5)
#define is5(test) (!stricmp(word[0], test) && numwords==6)
//#define par word[1]
	char * par=NULL;
	if (word[1]) par=strdup(word[1]);
	autoptr<char*> parptr=par;
	unsigned int num;
	int len=0;//declared here for A->generic fallback
	if(0);
	else if (assemblemapper(word, numwords)) {}
#define getvars(optbank) num=(pass!=0)?getnum(par):0; if (word[0][3]=='.') { len=getlenfromchar(word[0][4]); word[0][3]='\0'; } else len=getlen(par, optbank)
#define match(left, right) (word[1] && stribegin(par, left) && striend(par, right))
#define init(left, right) itrim(par, left, right); getvars(false)
#define bankoptinit(left, right) itrim(par, left, right); getvars(true)
#define blankinit() len=1; num=0
#define end() return false
#define as0(    op, byte) if (is(op)          ) { write1((unsigned int)byte);              return true; }
#define as1(    op, byte) if (is(op) && len==1) { write1((unsigned int)byte); write1(num); return true; }
#define as2(    op, byte) if (is(op) && len==2) { write1((unsigned int)byte); write2(num); return true; } \
													/*if (is(op) && len==3 && emulate) { write1(byte); write2(num); return true; }*/
#define as3(    op, byte) if (is(op) && len==3) { write1((unsigned int)byte); write3(num); return true; }
//#define as23(   op, byte) if (is(op) && (len==2 || len==3)) { write1(byte); write2(num); return true; }
#define as32(   op, byte) if (is(op) && (len==2 || len==3)) { write1((unsigned int)byte); write3(num); return true; }
#define as_a(   op, byte) if (is(op)) { if (len==1) { write1((unsigned int)byte); write1(num); } \
																					 else { write1((unsigned int)byte); write2(num); } return true; }
#define as_xy(  op, byte) if (is(op)) { if (len==1) { write1((unsigned int)byte); write1(num); } \
																					 else {  write1((unsigned int)byte); write2(num); } return true; }
#define as_rep( op, byte) if (is(op)) { if (pass==0) num=getnum(par); for (unsigned int i=0;i<num;i++) { write1((unsigned int)byte); } return true; }
#define as_rel1(op, byte) if (is(op)) { int pos=(!foundlabel)?(int)num:(int)num-((snespos&0xFFFFFF)+2); write1((unsigned int)byte); write1((unsigned int)pos); \
													if (pass==2 && foundlabel && (pos<-128 || pos>127)) error(2, S"Relative branch out of bounds (distance is "+dec(pos)+")"); \
													return true; }
#define as_rel2(op, byte) if (is(op)) { int pos=(!foundlabel)?(int)num:(int)num-((snespos&0xFFFFFF)+3); write1((unsigned int)byte); write2((unsigned int)pos);\
											if (pass==2 && foundlabel && (pos<-32768 || pos>32767)) error(2, S"Relative branch out of bounds (distance is "+dec(pos)+")"); \
											return true; }
#define the8(offset, len) as##len("ORA", offset+0x00); as##len("AND", offset+0x20); as##len("EOR", offset+0x40); as##len("ADC", offset+0x60); \
													as##len("STA", offset+0x80); as##len("LDA", offset+0xA0); as##len("CMP", offset+0xC0); as##len("SBC", offset+0xE0)
#define thenext8(offset, len) as##len("ASL", offset+0x00); as##len("BIT", offset+0x1E); as##len("ROL", offset+0x20); as##len("LSR", offset+0x40); \
															as##len("ROR", offset+0x60); as##len("LDY", offset+0x9E); as##len("DEC", offset+0xC0); as##len("INC", offset+0xE0)
#define thefinal7(offset, len) as##len("TSB", offset+0x00); as##len("TRB", offset+0x10); as##len("STY", offset+0x80); as##len("STX", offset+0x82); \
															 as##len("LDX", offset+0xA2); as##len("CPY", offset+0xC0); as##len("CPX", offset+0xE0)
#define onlythe8(left, right, offset) else if (match(left, right)) do { init(left, right); the8(offset, 1); end(); } while(0)
	else if ((strlen(word[0])!=3 && (strlen(word[0])!=5 || word[0][3]!='.')) || (word[1] && word[2])) return false;
	else if (!word[1])
	{
		blankinit();
		as0("PHP", 0x08); as0("ASL", 0x0A); as0("PHD", 0x0B); as0("CLC", 0x18);
		as0("INC", 0x1A); as0("TCS", 0x1B); as0("PLP", 0x28); as0("ROL", 0x2A);
		as0("PLD", 0x2B); as0("SEC", 0x38); as0("DEC", 0x3A); as0("TSC", 0x3B);
		as0("RTI", 0x40); as0("PHA", 0x48); as0("LSR", 0x4A); as0("PHK", 0x4B);
		as0("CLI", 0x58); as0("PHY", 0x5A); as0("TCD", 0x5B); as0("RTS", 0x60);
		as0("PLA", 0x68); as0("ROR", 0x6A); as0("RTL", 0x6B); as0("SEI", 0x78);
		as0("PLY", 0x7A); as0("TDC", 0x7B); as0("DEY", 0x88); as0("TXA", 0x8A);//these tables are blatantly stolen from xkas
		as0("PHB", 0x8B); as0("TYA", 0x98); as0("TXS", 0x9A); as0("TXY", 0x9B);
		as0("TAY", 0xA8); as0("TAX", 0xAA); as0("PLB", 0xAB); as0("CLV", 0xB8);
		as0("TSX", 0xBA); as0("TYX", 0xBB); as0("INY", 0xC8); as0("DEX", 0xCA);
		as0("WAI", 0xCB); as0("CLD", 0xD8); as0("PHX", 0xDA); as0("STP", 0xDB);
		as0("INX", 0xE8); as0("NOP", 0xEA); as0("XBA", 0xEB); as0("SED", 0xF8);
		as0("PLX", 0xFA); as0("XCE", 0xFB);
		as1("BRK", 0x00); as1("COP", 0x02); as1("WDM", 0x42);
		//as0("DEA", 0x3A); as0("INA", 0x1A); as0("TAD", 0x5B); as0("TDA", 0x7B);//nobody cares about these, but keeping them does no harm
		//as0("TAS", 0x1B); as0("TSA", 0x3B); as0("SWA", 0xEB);                  //actually, it does: it may make some users think it's correct.
		end();
	}
	else if (!stricmp(word[1], "A"))
	{
		blankinit();
		as0("ASL", 0x0A); as0("LSR", 0x4A); as0("ROL", 0x2A); as0("ROR", 0x6A);
		as0("INC", 0x1A); as0("DEC", 0x3A);
		goto opAFallback;//yay goto
	}
	else if (match("#", ""))
	{
		bankoptinit("#", "");
		as_a("ORA", 0x09); as_a("AND", 0x29); as_a("EOR", 0x49); as_a("ADC", 0x69);
		as_a("BIT", 0x89); as_a("LDA", 0xA9); as_a("CMP", 0xC9); as_a("SBC", 0xE9);
		as_xy("CPX", 0xE0); as_xy("CPY", 0xC0); as_xy("LDX", 0xA2); as_xy("LDY", 0xA0);
		as_rep("ASL", 0x0A); as_rep("LSR", 0x4A); as_rep("ROL", 0x2A); as_rep("ROR", 0x6A);
		as_rep("INC", 0x1A); as_rep("DEC", 0x3A); as_rep("INX", 0xE8); as_rep("DEX", 0xCA);
		as_rep("INY", 0xC8); as_rep("DEY", 0x88); as_rep("NOP", 0xEA);
		as1("REP", 0xC2); as1("SEP", 0xE2);
		as1("BRK", 0x00); as1("COP", 0x02); as1("WDM", 0x42);
		end();
	}
	onlythe8("(", ",s),y", 0x13);
	onlythe8("[", "],y", 0x17);
	onlythe8("(", "),y", 0x11);
	onlythe8("", ",s", 0x03);
	else if (match("[", "]"))
	{
		init("[", "]");
		the8(0x07, 1);
		as2("JMP", 0xDC); as2("JML", 0xDC);
		end();
	}
	else if (match("(", ",x)"))
	{
		init("(", ",x)");
		the8(0x01, 1);
		as2("JMP", 0x7C); as2("JSR", 0xFC);
		end();
	}
	else if (match("(", ")") && confirmqpar(substr(word[1]+1, (int)(strlen(word[1]+1)-1))))
	{
		init("(", ")");
		the8(0x12, 1);
		as1("PEI", 0xD4);
		as2("JMP", 0x6C);
		end();
	}
	else if (match("", ",x"))
	{
		init("", ",x");
		if (match("(", ")") && confirmqpar(substr(word[1]+1, (int)(strlen(word[1]+1)-2-1)))) warn0("($yy),x does not exist, assuming $yy,x");
		the8(0x1F, 3);
		the8(0x1D, 2);
		the8(0x15, 1);
		thenext8(0x16, 1);
		thenext8(0x1E, 2);
		as1("STZ", 0x74);
		as1("STY", 0x94);
		as2("STZ", 0x9E);
		end();
	}
	else if (match("", ",y"))
	{
		init("", ",y");
		if (len==3 && emulatexkas) len=2;
		as1("LDX", 0xB6);
		as1("STX", 0x96);
		as2("LDX", 0xBE);
		if (len==1 && (is("ORA") || is("AND") || is("EOR") || is("ADC") || is("STA") || is("LDA") || is("CMP") || is("SBC")))
		{
			warn0(S word[0]+" $xx,y is not valid with 8-bit parameters, assuming 16-bit");
			len=2;
		}
		the8(0x19, 2);
		end();
	}
	else
	{
		if ((is("MVN") || is("MVP")) && confirmqpar(par))
		{
			int numargs;
			autoptr<char**>param=qpsplit(par, ",", &numargs);
			if (numargs ==2)
			{
				write1(is("MVN")?(unsigned int)0x54:(unsigned int)0x44);
				write1(getnum(param[0]));
				write1(getnum(param[1]));
				return true;
			}
		}
		if (false)
		{
opAFallback:
			unsigned int tmp=0;
			if (pass && !labelval(par, &tmp)) return false;
			len=getlen(par);
			num=tmp;
		}
		if (is("JSR") || is("JMP"))
		{
			int tmp=optimizeforbank;
			optimizeforbank=-1;
			init("", "");
			optimizeforbank=tmp;
		}
		else
		{
			init("", "");
		}
		the8(0x0F, 3);
		the8(0x0D, 2);
		the8(0x05, 1);
		thenext8(0x06, 1);
		thenext8(0x0E, 2);
		thefinal7(0x04, 1);
		thefinal7(0x0C, 2);
		as1("STZ", 0x64);
		as2("STZ", 0x9C);
		as2("JMP", 0x4C);
		as2("JSR", 0x20);
		as32("JML", 0x5C);
		as32("JSL", 0x22);
		as2("MVN", 0x54);
		as2("MVP", 0x44);
		as2("PEA", 0xF4);
		if (emulatexkas)
		{
			as3("JMP", 0x5C);//all my hate
			//as3("JSR", 0x22);
		}
		as_rel1("BRA", 0x80);
		as_rel1("BCC", 0x90);
		as_rel1("BCS", 0xB0);
		as_rel1("BEQ", 0xF0);
		as_rel1("BNE", 0xD0);
		as_rel1("BMI", 0x30);
		as_rel1("BPL", 0x10);
		as_rel1("BVC", 0x50);
		as_rel1("BVS", 0x70);
		as_rel2("BRL", 0x82);
		as_rel2("PER", 0x62);
		end();
	}
	return true;
}
