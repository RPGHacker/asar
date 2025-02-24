#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include <cassert>
#include <initializer_list>

// A bit of terminology i just invented:
// "mnemonic" refers to the name of an instruction, e.g. LDA or JMP.
// "address mode" is the form of an instruction's argument, with a specific
//   width, e.g. [$12] or $1234,x. determining the exact mode requires knowing
//   the width of the instruction.
// "address kind" is like an address mode, but without a specific width. it
//   is determined purely from the syntax of the argument.
// "modifier" is the length suffix (the .w in LDA.w), but it's called modifier
//   because it's a bit more general (also sets branch targeting mode).

enum class addr_kind {
	abs,   // $00
	x,     // $00,x
	y,     // $00,y
	ind,   // ($00)  "indirect"
	lind,  // [$00]  "long indirect"
	xind,  // ($00,x)
	indy,  // ($00),y
	lindy, // [$00],y
	s,     // $00,s
	sy,    // ($00,s),y
	imp,   // implied (no argument)
	a,     // 'A' as argument
	imm,   // #$00  "immediate"
	mvn,   // special for mvn/mvp
	num_addr_kinds,
};
// used as bitmasks
static_assert((int)addr_kind::num_addr_kinds < 32);

// various flags that affect parsing of specific instructions.
// these are stored per mnemonic+kind, which is required for some of them, and
// the rest usually have only 1 kind/width allowed anyways so it doesn't hurt.
// this is not an `enum class` because those are annoying to use as bitmasks.
enum mnemonic_flag {
	// for brk,cop,wdm: implied mode = immediate with arg 0
	flag_imm_implied_0   = 1 << 0,
	// imm should be size 1 or 2 depending on accum size.
	// currently all this does is allow both 1- and 2-wide immediates,
	// but could be useful for tracking rep/sep state in asar in the future.
	flag_imm_width_a     = 1 << 1,
	// imm should be size 1 or 2 depending on x/y size
	flag_imm_width_xy    = 1 << 2,
	// branch opcodes, have weird target computation rules
	flag_branch          = 1 << 3,
	// for jmp/jsr, sets optimizeforbank to K
	flag_bank_k          = 1 << 4,
	// sets optimizeforbank to 0 (as some jumps always read from bank 0)
	flag_bank_0          = 1 << 5,
	// additionally allow `imm` addressing which is interpreted as repetition
	flag_implied_rep     = 1 << 6,
};

struct mnem_kind_info {
	uint8_t allowed_widths; // bitmask, low 4 bits represent widths 0 to 3
	uint8_t opcode_for_width[4];
	uint32_t flags; // combination of `mnemonic_flag`s
};
struct mnemonicinfo {
	// kind of a waste to have this array which most of the time only has 1 entry filled
	mnem_kind_info types[(int)addr_kind::num_addr_kinds];
	uint32_t allowed_kinds_mask;
};

struct mnemonic_lookup_t {
	std::unordered_map<string, mnemonicinfo> the_map;

	// helper struct for the initializer_list
	struct opcode {
		uint8_t byte;
		const char *mnem;
		addr_kind akind;
		int width; // width of argument, not entire insn
		uint32_t flags;
		opcode(uint8_t byte_, const char *mnem_, addr_kind akind_, int width_, uint32_t flags_ = 0)
			: byte(byte_), mnem(mnem_), akind(akind_), width(width_),
			flags(flags_) {}
	};

	static void add_one(mnemonicinfo& mnem, addr_kind kind, int width, uint8_t byte, uint32_t flags) {
		mnem.allowed_kinds_mask |= 1 << (int)kind;
		auto& this_part = mnem.types[(int)kind];
		assert((this_part.allowed_widths & (1 << width)) == 0);
		this_part.allowed_widths |= 1 << width;
		this_part.opcode_for_width[width] = byte;
		this_part.flags |= flags;
	}

	mnemonic_lookup_t(std::initializer_list<opcode> l) {
		for(const auto& x : l) {
			auto& this_mnem = the_map[x.mnem];
			add_one(this_mnem, x.akind, x.width, x.byte, x.flags);
			if(x.flags & flag_imm_implied_0) {
				assert(x.akind == addr_kind::imm && x.width == 1);
				add_one(this_mnem, addr_kind::imp, 0, x.byte, x.flags);
			}
			if(x.akind == addr_kind::a) {
				add_one(this_mnem, addr_kind::imp, 0, x.byte, x.flags);
			}
			if(x.flags & flag_implied_rep) {
				add_one(this_mnem, addr_kind::imm, 0, x.byte, x.flags);
			}
			if(x.flags & (flag_imm_width_a | flag_imm_width_xy)) {
				assert(x.akind == addr_kind::imm && x.width == 1);
				add_one(this_mnem, x.akind, 2, x.byte, x.flags);
			}
		}
	}
};

mnemonic_lookup_t mnemonic_lookup = {
	{ 0x00, "brk", addr_kind::imm    , 1, flag_imm_implied_0 },
	{ 0x01, "ora", addr_kind::xind   , 1 },
	{ 0x02, "cop", addr_kind::imm    , 1, flag_imm_implied_0 },
	{ 0x03, "ora", addr_kind::s      , 1 },
	{ 0x04, "tsb", addr_kind::abs    , 1 },
	{ 0x05, "ora", addr_kind::abs    , 1 },
	{ 0x06, "asl", addr_kind::abs    , 1 },
	{ 0x07, "ora", addr_kind::lind   , 1 },
	{ 0x08, "php", addr_kind::imp    , 0 },
	{ 0x09, "ora", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0x0a, "asl", addr_kind::a      , 0, flag_implied_rep },
	{ 0x0b, "phd", addr_kind::imp    , 0 },
	{ 0x0c, "tsb", addr_kind::abs    , 2 },
	{ 0x0d, "ora", addr_kind::abs    , 2 },
	{ 0x0e, "asl", addr_kind::abs    , 2 },
	{ 0x0f, "ora", addr_kind::abs    , 3 },
	{ 0x10, "bpl", addr_kind::abs    , 1, flag_branch },
	{ 0x11, "ora", addr_kind::indy   , 1 },
	{ 0x12, "ora", addr_kind::ind    , 1 },
	{ 0x13, "ora", addr_kind::sy     , 1 },
	{ 0x14, "trb", addr_kind::abs    , 1 },
	{ 0x15, "ora", addr_kind::x      , 1 },
	{ 0x16, "asl", addr_kind::x      , 1 },
	{ 0x17, "ora", addr_kind::lindy  , 1 },
	{ 0x18, "clc", addr_kind::imp    , 0 },
	{ 0x19, "ora", addr_kind::y      , 2 },
	{ 0x1a, "inc", addr_kind::a      , 0, flag_implied_rep },
	{ 0x1b, "tcs", addr_kind::imp    , 0 },
	{ 0x1c, "trb", addr_kind::abs    , 2 },
	{ 0x1d, "ora", addr_kind::x      , 2 },
	{ 0x1e, "asl", addr_kind::x      , 2 },
	{ 0x1f, "ora", addr_kind::x      , 3 },
	{ 0x20, "jsr", addr_kind::abs    , 2, flag_bank_k },
	{ 0x21, "and", addr_kind::xind   , 1 },
	{ 0x22, "jsl", addr_kind::abs    , 3 },
	{ 0x23, "and", addr_kind::s      , 1 },
	{ 0x24, "bit", addr_kind::abs    , 1 },
	{ 0x25, "and", addr_kind::abs    , 1 },
	{ 0x26, "rol", addr_kind::abs    , 1 },
	{ 0x27, "and", addr_kind::lind   , 1 },
	{ 0x28, "plp", addr_kind::imp    , 0 },
	{ 0x29, "and", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0x2a, "rol", addr_kind::a      , 0, flag_implied_rep },
	{ 0x2b, "pld", addr_kind::imp    , 0 },
	{ 0x2c, "bit", addr_kind::abs    , 2 },
	{ 0x2d, "and", addr_kind::abs    , 2 },
	{ 0x2e, "rol", addr_kind::abs    , 2 },
	{ 0x2f, "and", addr_kind::abs    , 3 },
	{ 0x30, "bmi", addr_kind::abs    , 1, flag_branch },
	{ 0x31, "and", addr_kind::indy   , 1 },
	{ 0x32, "and", addr_kind::ind    , 1 },
	{ 0x33, "and", addr_kind::sy     , 1 },
	{ 0x34, "bit", addr_kind::x      , 1 },
	{ 0x35, "and", addr_kind::x      , 1 },
	{ 0x36, "rol", addr_kind::x      , 1 },
	{ 0x37, "and", addr_kind::lindy  , 1 },
	{ 0x38, "sec", addr_kind::imp    , 0 },
	{ 0x39, "and", addr_kind::y      , 2 },
	{ 0x3a, "dec", addr_kind::a      , 0, flag_implied_rep },
	{ 0x3b, "tsc", addr_kind::imp    , 0 },
	{ 0x3c, "bit", addr_kind::x      , 2 },
	{ 0x3d, "and", addr_kind::x      , 2 },
	{ 0x3e, "rol", addr_kind::x      , 2 },
	{ 0x3f, "and", addr_kind::x      , 3 },
	{ 0x40, "rti", addr_kind::imp    , 0 },
	{ 0x41, "eor", addr_kind::xind   , 1 },
	{ 0x42, "wdm", addr_kind::imm    , 1, flag_imm_implied_0 },
	{ 0x43, "eor", addr_kind::s      , 1 },
	{ 0x44, "mvp", addr_kind::mvn    , 2 },
	{ 0x45, "eor", addr_kind::abs    , 1 },
	{ 0x46, "lsr", addr_kind::abs    , 1 },
	{ 0x47, "eor", addr_kind::lind   , 1 },
	{ 0x48, "pha", addr_kind::imp    , 0 },
	{ 0x49, "eor", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0x4a, "lsr", addr_kind::a      , 0, flag_implied_rep },
	{ 0x4b, "phk", addr_kind::imp    , 0 },
	{ 0x4c, "jmp", addr_kind::abs    , 2, flag_bank_k },
	{ 0x4d, "eor", addr_kind::abs    , 2 },
	{ 0x4e, "lsr", addr_kind::abs    , 2 },
	{ 0x4f, "eor", addr_kind::abs    , 3 },
	{ 0x50, "bvc", addr_kind::abs    , 1, flag_branch },
	{ 0x51, "eor", addr_kind::indy   , 1 },
	{ 0x52, "eor", addr_kind::ind    , 1 },
	{ 0x53, "eor", addr_kind::sy     , 1 },
	{ 0x54, "mvn", addr_kind::mvn    , 2 },
	{ 0x55, "eor", addr_kind::x      , 1 },
	{ 0x56, "lsr", addr_kind::x      , 1 },
	{ 0x57, "eor", addr_kind::lindy  , 1 },
	{ 0x58, "cli", addr_kind::imp    , 0 },
	{ 0x59, "eor", addr_kind::y      , 2 },
	{ 0x5a, "phy", addr_kind::imp    , 0 },
	{ 0x5b, "tcd", addr_kind::imp    , 0 },
	{ 0x5c, "jml", addr_kind::abs    , 3 },
	{ 0x5d, "eor", addr_kind::x      , 2 },
	{ 0x5e, "lsr", addr_kind::x      , 2 },
	{ 0x5f, "eor", addr_kind::x      , 3 },
	{ 0x60, "rts", addr_kind::imp    , 0 },
	{ 0x61, "adc", addr_kind::xind   , 1 },
	{ 0x62, "per", addr_kind::abs    , 2, flag_branch },
	{ 0x63, "adc", addr_kind::s      , 1 },
	{ 0x64, "stz", addr_kind::abs    , 1 },
	{ 0x65, "adc", addr_kind::abs    , 1 },
	{ 0x66, "ror", addr_kind::abs    , 1 },
	{ 0x67, "adc", addr_kind::lind   , 1 },
	{ 0x68, "pla", addr_kind::imp    , 0 },
	{ 0x69, "adc", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0x6a, "ror", addr_kind::a      , 0, flag_implied_rep },
	{ 0x6b, "rtl", addr_kind::imp    , 0 },
	{ 0x6c, "jmp", addr_kind::ind    , 2, flag_bank_0 },
	{ 0x6d, "adc", addr_kind::abs    , 2 },
	{ 0x6e, "ror", addr_kind::abs    , 2 },
	{ 0x6f, "adc", addr_kind::abs    , 3 },
	{ 0x70, "bvs", addr_kind::abs    , 1, flag_branch },
	{ 0x71, "adc", addr_kind::indy   , 1 },
	{ 0x72, "adc", addr_kind::ind    , 1 },
	{ 0x73, "adc", addr_kind::sy     , 1 },
	{ 0x74, "stz", addr_kind::x      , 1 },
	{ 0x75, "adc", addr_kind::x      , 1 },
	{ 0x76, "ror", addr_kind::x      , 1 },
	{ 0x77, "adc", addr_kind::lindy  , 1 },
	{ 0x78, "sei", addr_kind::imp    , 0 },
	{ 0x79, "adc", addr_kind::y      , 2 },
	{ 0x7a, "ply", addr_kind::imp    , 0 },
	{ 0x7b, "tdc", addr_kind::imp    , 0 },
	{ 0x7c, "jmp", addr_kind::xind   , 2, flag_bank_k },
	{ 0x7d, "adc", addr_kind::x      , 2 },
	{ 0x7e, "ror", addr_kind::x      , 2 },
	{ 0x7f, "adc", addr_kind::x      , 3 },
	{ 0x80, "bra", addr_kind::abs    , 1, flag_branch },
	{ 0x81, "sta", addr_kind::xind   , 1 },
	{ 0x82, "brl", addr_kind::abs    , 2, flag_branch },
	{ 0x83, "sta", addr_kind::s      , 1 },
	{ 0x84, "sty", addr_kind::abs    , 1 },
	{ 0x85, "sta", addr_kind::abs    , 1 },
	{ 0x86, "stx", addr_kind::abs    , 1 },
	{ 0x87, "sta", addr_kind::lind   , 1 },
	{ 0x88, "dey", addr_kind::imp    , 0, flag_implied_rep },
	{ 0x89, "bit", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0x8a, "txa", addr_kind::imp    , 0 },
	{ 0x8b, "phb", addr_kind::imp    , 0 },
	{ 0x8c, "sty", addr_kind::abs    , 2 },
	{ 0x8d, "sta", addr_kind::abs    , 2 },
	{ 0x8e, "stx", addr_kind::abs    , 2 },
	{ 0x8f, "sta", addr_kind::abs    , 3 },
	{ 0x90, "bcc", addr_kind::abs    , 1, flag_branch },
	{ 0x91, "sta", addr_kind::indy   , 1 },
	{ 0x92, "sta", addr_kind::ind    , 1 },
	{ 0x93, "sta", addr_kind::sy     , 1 },
	{ 0x94, "sty", addr_kind::x      , 1 },
	{ 0x95, "sta", addr_kind::x      , 1 },
	{ 0x96, "stx", addr_kind::y      , 1 },
	{ 0x97, "sta", addr_kind::lindy  , 1 },
	{ 0x98, "tya", addr_kind::imp    , 0 },
	{ 0x99, "sta", addr_kind::y      , 2 },
	{ 0x9a, "txs", addr_kind::imp    , 0 },
	{ 0x9b, "txy", addr_kind::imp    , 0 },
	{ 0x9c, "stz", addr_kind::abs    , 2 },
	{ 0x9d, "sta", addr_kind::x      , 2 },
	{ 0x9e, "stz", addr_kind::x      , 2 },
	{ 0x9f, "sta", addr_kind::x      , 3 },
	{ 0xa0, "ldy", addr_kind::imm    , 1, flag_imm_width_xy },
	{ 0xa1, "lda", addr_kind::xind   , 1 },
	{ 0xa2, "ldx", addr_kind::imm    , 1, flag_imm_width_xy },
	{ 0xa3, "lda", addr_kind::s      , 1 },
	{ 0xa4, "ldy", addr_kind::abs    , 1 },
	{ 0xa5, "lda", addr_kind::abs    , 1 },
	{ 0xa6, "ldx", addr_kind::abs    , 1 },
	{ 0xa7, "lda", addr_kind::lind   , 1 },
	{ 0xa8, "tay", addr_kind::imp    , 0 },
	{ 0xa9, "lda", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0xaa, "tax", addr_kind::imp    , 0 },
	{ 0xab, "plb", addr_kind::imp    , 0 },
	{ 0xac, "ldy", addr_kind::abs    , 2 },
	{ 0xad, "lda", addr_kind::abs    , 2 },
	{ 0xae, "ldx", addr_kind::abs    , 2 },
	{ 0xaf, "lda", addr_kind::abs    , 3 },
	{ 0xb0, "bcs", addr_kind::abs    , 1, flag_branch },
	{ 0xb1, "lda", addr_kind::indy   , 1 },
	{ 0xb2, "lda", addr_kind::ind    , 1 },
	{ 0xb3, "lda", addr_kind::sy     , 1 },
	{ 0xb4, "ldy", addr_kind::x      , 1 },
	{ 0xb5, "lda", addr_kind::x      , 1 },
	{ 0xb6, "ldx", addr_kind::y      , 1 },
	{ 0xb7, "lda", addr_kind::lindy  , 1 },
	{ 0xb8, "clv", addr_kind::imp    , 0 },
	{ 0xb9, "lda", addr_kind::y      , 2 },
	{ 0xba, "tsx", addr_kind::imp    , 0 },
	{ 0xbb, "tyx", addr_kind::imp    , 0 },
	{ 0xbc, "ldy", addr_kind::x      , 2 },
	{ 0xbd, "lda", addr_kind::x      , 2 },
	{ 0xbe, "ldx", addr_kind::y      , 2 },
	{ 0xbf, "lda", addr_kind::x      , 3 },
	{ 0xc0, "cpy", addr_kind::imm    , 1, flag_imm_width_xy },
	{ 0xc1, "cmp", addr_kind::xind   , 1 },
	{ 0xc2, "rep", addr_kind::imm    , 1 },
	{ 0xc3, "cmp", addr_kind::s      , 1 },
	{ 0xc4, "cpy", addr_kind::abs    , 1 },
	{ 0xc5, "cmp", addr_kind::abs    , 1 },
	{ 0xc6, "dec", addr_kind::abs    , 1 },
	{ 0xc7, "cmp", addr_kind::lind   , 1 },
	{ 0xc8, "iny", addr_kind::imp    , 0, flag_implied_rep },
	{ 0xc9, "cmp", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0xca, "dex", addr_kind::imp    , 0, flag_implied_rep },
	{ 0xcb, "wai", addr_kind::imp    , 0 },
	{ 0xcc, "cpy", addr_kind::abs    , 2 },
	{ 0xcd, "cmp", addr_kind::abs    , 2 },
	{ 0xce, "dec", addr_kind::abs    , 2 },
	{ 0xcf, "cmp", addr_kind::abs    , 3 },
	{ 0xd0, "bne", addr_kind::abs    , 1, flag_branch },
	{ 0xd1, "cmp", addr_kind::indy   , 1 },
	{ 0xd2, "cmp", addr_kind::ind    , 1 },
	{ 0xd3, "cmp", addr_kind::sy     , 1 },
	{ 0xd4, "pei", addr_kind::ind    , 1 },
	{ 0xd5, "cmp", addr_kind::x      , 1 },
	{ 0xd6, "dec", addr_kind::x      , 1 },
	{ 0xd7, "cmp", addr_kind::lindy  , 1 },
	{ 0xd8, "cld", addr_kind::imp    , 0 },
	{ 0xd9, "cmp", addr_kind::y      , 2 },
	{ 0xda, "phx", addr_kind::imp    , 0 },
	{ 0xdb, "stp", addr_kind::imp    , 0 },
	{ 0xdc, "jmp", addr_kind::lind   , 2, flag_bank_0 },
	{ 0xdc, "jml", addr_kind::lind   , 2, flag_bank_0 },
	{ 0xdd, "cmp", addr_kind::x      , 2 },
	{ 0xde, "dec", addr_kind::x      , 2 },
	{ 0xdf, "cmp", addr_kind::x      , 3 },
	{ 0xe0, "cpx", addr_kind::imm    , 1, flag_imm_width_xy },
	{ 0xe1, "sbc", addr_kind::xind   , 1 },
	{ 0xe2, "sep", addr_kind::imm    , 1 },
	{ 0xe3, "sbc", addr_kind::s      , 1 },
	{ 0xe4, "cpx", addr_kind::abs    , 1 },
	{ 0xe5, "sbc", addr_kind::abs    , 1 },
	{ 0xe6, "inc", addr_kind::abs    , 1 },
	{ 0xe7, "sbc", addr_kind::lind   , 1 },
	{ 0xe8, "inx", addr_kind::imp    , 0, flag_implied_rep },
	{ 0xe9, "sbc", addr_kind::imm    , 1, flag_imm_width_a },
	{ 0xea, "nop", addr_kind::imp    , 0, flag_implied_rep },
	{ 0xeb, "xba", addr_kind::imp    , 0 },
	{ 0xec, "cpx", addr_kind::abs    , 2 },
	{ 0xed, "sbc", addr_kind::abs    , 2 },
	{ 0xee, "inc", addr_kind::abs    , 2 },
	{ 0xef, "sbc", addr_kind::abs    , 3 },
	{ 0xf0, "beq", addr_kind::abs    , 1, flag_branch },
	{ 0xf1, "sbc", addr_kind::indy   , 1 },
	{ 0xf2, "sbc", addr_kind::ind    , 1 },
	{ 0xf3, "sbc", addr_kind::sy     , 1 },
	{ 0xf4, "pea", addr_kind::abs    , 2 },
	{ 0xf5, "sbc", addr_kind::x      , 1 },
	{ 0xf6, "inc", addr_kind::x      , 1 },
	{ 0xf7, "sbc", addr_kind::lindy  , 1 },
	{ 0xf8, "sed", addr_kind::imp    , 0 },
	{ 0xf9, "sbc", addr_kind::y      , 2 },
	{ 0xfa, "plx", addr_kind::imp    , 0 },
	{ 0xfb, "xce", addr_kind::imp    , 0 },
	{ 0xfc, "jsr", addr_kind::xind   , 2, flag_bank_k },
	{ 0xfd, "sbc", addr_kind::x      , 2 },
	{ 0xfe, "inc", addr_kind::x      , 2 },
	{ 0xff, "sbc", addr_kind::x      , 3 },
};

struct parse_result {
	addr_kind kind;
	string arg;
};

// checks for matching characters at the start of haystack, ignoring spaces.
// returns index into haystack right after the match
template<char... chars>
int64_t startmatch(const string& haystack) {
	static const char needle[] = {chars...};
	size_t haystack_i = 0;
	for(size_t i = 0; i < sizeof...(chars); i++) {
		while(haystack[haystack_i] == ' ') haystack_i++;
		if(needle[i] != to_lower(haystack[haystack_i++])) return -1;
	}
	return haystack_i;
}

// checks for matching characters at the end of haystack, ignoring spaces.
// returns index into haystack right before the match
template<char... chars>
int64_t endmatch(const string& haystack) {
	static const char needle[] = {chars...};
	int64_t haystack_i = haystack.length()-1;
	for(int64_t i = sizeof...(chars)-1; i >= 0; i--) {
		while(haystack_i >= 0 && haystack[haystack_i] == ' ') haystack_i--;
		if(haystack_i < 0 || needle[i] != to_lower(haystack[haystack_i--])) return -1;
	}
	return haystack_i+1;
}

/*
* Parses the address kind from an argument, given a list of allowed address kinds.
* Throws "invalid address mode" if the argument does not match any kinds.
*/
// i still don't like this function...
parse_result parse_addr_kind(const string& arg, uint32_t allowed_kinds_mask) {
	int64_t start_i = 0, end_i = arg.length();
// If this addressing kind is allowed, return it, along with a trimmed version of the string.
#define RETURN_IF_ALLOWED(kind) \
		if (allowed_kinds_mask & 1<<(uint32_t)addr_kind::kind) { \
			string out(arg.data() + start_i, end_i - start_i); \
			return parse_result{addr_kind::kind, out}; \
		}
	if((start_i = startmatch<'#'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(imm);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((start_i = startmatch<'('>(arg)) >= 0) {
		// TODO check if the end of this paren is where it should be
		if((end_i = endmatch<',', 's', ')', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(sy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(indy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<',', 'x', ')'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(xind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(ind);
			asar_throw_warning(1, warning_id_assuming_address_mode, "($00)", "$00", " (if this was intentional, add a +0 after the parentheses.)");
		}
	}
	if((start_i = startmatch<'['>(arg)) >= 0) {
		if((end_i = endmatch<']', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(lindy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<']'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(lind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
	}
	start_i = 0;
	if((end_i = endmatch<',', 'x'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(x);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 'y'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(y);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 's'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(s);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	end_i = arg.length();
	// hoping that by now, something would have stripped whitespace lol
	if(arg.length() == 0) {
		RETURN_IF_ALLOWED(imp);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if(arg == "a" || arg == "A") {
		RETURN_IF_ALLOWED(a);
		// todo: some hint for "don't name your label "a" "?
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	RETURN_IF_ALLOWED(abs);
	asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
#undef RETURN_IF_ALLOWED
}

const char* format_valid_widths(int min, int max) {
	if(min == 1) {
		if(max == 1) return "only 8-bit";
		if(max == 2) return "only 8-bit or 16-bit";
		if(max == 3) return "any width"; // shouldn't actually happen lol
	} else if(min == 2) {
		if(max == 2) return "only 16-bit";
		if(max == 3) return "only 16-bit or 24-bit";
	} else if(min == 3) {
		return "only 24-bit";
	}
	return "???";
}

int get_real_len(int min_len, int max_len, char modifier, const parse_result& parsed) {
	// we can theoretically give min_len to getlen now :o
	int arg_min_len = getlen(parsed.arg, parsed.kind == addr_kind::imm);
	int out_len;
	if(modifier != 0) {
		out_len = getlenfromchar(modifier);
		if(out_len < min_len || out_len > max_len)
			asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(min_len, max_len), out_len*8);
	} else {
		if(parsed.kind == addr_kind::imm) {
			if(!is_hex_constant(parsed.arg.data()))
				asar_throw_warning(2, warning_id_implicitly_sized_immediate);
			if(arg_min_len == 3 && max_len == 2) {
				// lda #label
				// todo: throw pedantic warning
				return max_len;
			}
		}
		if(arg_min_len > max_len) {
			// we might get here on pass 0 if getlen is wrong about the width,
			// which can happen with forward labels and namespaces.
			// in that case return some valid width to silence the error.
			if(pass == 0) return max_len;

			asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(min_len, max_len), arg_min_len*8);
		}
		// todo warn about widening when dpbase != 0
		out_len = std::max(arg_min_len, min_len);
	}
	return out_len;
}

int64_t get_branch_value(parse_result& parsed, char modifier, int width) {
	int64_t num = 0;
	num = getnum(parsed.arg);
	bool target_is_abs = foundlabel;
	if(modifier != 0) {
		if(to_lower(modifier) == 'a') target_is_abs = true;
		else if(to_lower(modifier) == 'r') target_is_abs = false;
		// TODO: better error message
		else asar_throw_error(2, error_type_block, error_id_invalid_opcode_length);
	}
	if(target_is_abs) {
		// cast delta to signed 16-bit, this makes it possible to handle bank-border-wrapping automatically
		int16_t delta = num - (snespos + width + 1);
		if((num & ~0xffff) != (snespos & ~0xffff)) {
			// todo: should throw error "can't branch to different bank"
			asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(delta).data());
		}
		if(width==1 && (delta < -128 || delta > 127)) {
			asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(delta).data());
		}
		return delta;
	} else {
		if(num & ~(width==2 ? 0xffff : 0xff)) {
			asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(num).data());
		}
		return (int16_t)(width==2 ? num : (int8_t)num);
	}
}

bool asblock_65816(char** word, int numwords)
{
	// first find the mnemonic from the first word
	int word_i = 0;
	bool autoclean = false;
	if(!stricmpwithlower(word[0], "autoclean")) {
		word_i++;
		autoclean = true;
	}
	string mnem = word[word_i++];
	for(int i = 0; i < mnem.length(); i++) mnem.raw()[i] = to_lower(mnem[i]);

	char modifier = 0;
	if(mnem.length() >= 2 && mnem[mnem.length()-2] == '.') {
		modifier = mnem[mnem.length()-1];
		mnem.truncate(mnem.length()-2);
	}

	auto it = mnemonic_lookup.the_map.find(mnem);
	if(it == mnemonic_lookup.the_map.end()) return false;
	mnemonicinfo& mnem_info = it->second;

	// join together all the other arguments
	string par;
	for(int i = word_i; i < numwords; i++){
		if(i > word_i) par += " ";
		par += word[i];
	}

	// check if we're doing mvn/mvp, and if yes, parse completely differently
	// this is a little hacky, sorry...
	if(mnem_info.allowed_kinds_mask & 1<<(uint32_t)addr_kind::mvn) {
		int count = 0;
		autoptr<char**> parts = qpsplit(par.raw(), ',', &count);
		if(count != 2) asar_throw_error(2, error_type_block, error_id_bad_addr_mode);
		uint8_t opcode = mnem_info.types[(int)addr_kind::mvn].opcode_for_width[2];
		write1(opcode);
		int64_t num1 = 0, num2 = 0;
		if(pass == 2) {
			num1 = getnum(parts[0]);
			num2 = getnum(parts[1]);
		}
		if(num1 < 0 || num1 > 255) asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(1, 1), num1 > 65535 ? 24 : 16);
		if(num2 < 0 || num2 > 255) asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(1, 1), num2 > 65535 ? 24 : 16);
		write1(num1);
		write1(num2);
		// a bit hacky to check this here, but we kinda do need to early-return here
		if(autoclean) asar_throw_error(2, error_type_block, error_id_broken_autoclean);
		return true;
	}
	parse_result parse_res = parse_addr_kind(par, mnem_info.allowed_kinds_mask);
	mnem_kind_info& kind_info = mnem_info.types[(uint32_t)parse_res.kind];

	// figure out the minimum/maximum argument width for this mnemonic+addr_kind combo
	// todo this is mildly jank
	// we also kinda implicitly assume that no insn has "gaps" in its allowed widths
	int min_w = 99;
	int max_w = 0;
	for(int i = 0; i < 8; i++) {
		if(kind_info.allowed_widths & 1<<i) {
			min_w = std::min(min_w, i);
			max_w = std::max(max_w, i);
		}
	}

	// compute argument value
	bool is_implied_rep = parse_res.kind == addr_kind::imm && (kind_info.flags & flag_implied_rep);
	int arg_len;
	int64_t arg_value;
	int64_t rep_count = 0;
	if(kind_info.flags & flag_branch) {
		assert(min_w == max_w); // can't call get_real_len, but there should be only one width anyways
		arg_len = min_w;
		arg_value = pass == 2 ? get_branch_value(parse_res, modifier, arg_len) : 0;
	} else if(is_implied_rep) {
		arg_len = 0;
		rep_count = getnum(parse_res.arg);
		if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
	} else if(parse_res.kind == addr_kind::imp || parse_res.kind == addr_kind::a) {
		arg_len = 0;
	} else {
		int old_optimize = optimizeforbank;
		if(kind_info.flags & flag_bank_0) optimizeforbank = 0;
		else if(kind_info.flags & flag_bank_k) optimizeforbank = -1;
		arg_len = get_real_len(min_w, max_w, modifier, parse_res);
		optimizeforbank = old_optimize;
		arg_value = pass == 2 ? getnum(parse_res.arg) : 0;
	}

	assert((kind_info.allowed_widths & 1<<arg_len) != 0);
	uint8_t opcode = kind_info.opcode_for_width[arg_len];

	if(arg_len == 0 && (kind_info.flags & flag_imm_implied_0)) {
		arg_len = 1;
		arg_value = 0;
	}

	if(is_implied_rep) {
		for(int64_t i = 0; i < rep_count; i++) write1(opcode);
	} else {
		write1(opcode);
		if(arg_len == 0);
		else if(arg_len == 1) write1(arg_value);
		else if(arg_len == 2) write2(arg_value);
		else if(arg_len == 3) write3(arg_value);
	}

	if(autoclean && pass > 0) {
		// should be changed to "can't use autoclean on this instruction"?
		if(arg_len != 3) asar_throw_error(2, error_type_block, error_id_broken_autoclean);
		handle_autoclean(parse_res.arg, opcode, snespos - 4);
	}
	return true;
}
