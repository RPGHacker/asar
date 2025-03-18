#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include <cassert>
#include <initializer_list>
#include "frozen/string.h"
#include "frozen/unordered_map.h"

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
	uint8_t allowed_widths = 0; // bitmask, low 4 bits represent widths 0 to 3
	uint8_t opcode_for_width[4] = {};
	uint32_t flags = 0; // combination of `mnemonic_flag`s
};
struct mnemonicinfo {
	// kind of a waste to have this array which most of the time only has 1 entry filled
	mnem_kind_info types[(int)addr_kind::num_addr_kinds];
	uint32_t allowed_kinds_mask = 0;
	// helper struct for the initializer_list
	struct opcode {
		uint8_t byte;
		addr_kind akind;
		int width; // width of argument, not entire insn
		uint32_t flags;
		constexpr opcode(uint8_t byte_, addr_kind akind_, int width_, uint32_t flags_ = 0)
			: byte(byte_), akind(akind_), width(width_),
			flags(flags_) {}
	};
	constexpr void add_one(addr_kind kind, int width, uint8_t byte, uint32_t flags) {
		allowed_kinds_mask |= 1 << (int)kind;
		auto& this_part = types[(int)kind];
		assert((this_part.allowed_widths & (1 << width)) == 0);
		this_part.allowed_widths |= 1 << width;
		this_part.opcode_for_width[width] = byte;
		this_part.flags |= flags;

	}
	constexpr mnemonicinfo(const std::initializer_list<opcode>& opcodes) {
		for(auto& x : opcodes) {
			add_one(x.akind, x.width, x.byte, x.flags);
			if(x.flags & flag_imm_implied_0) {
				assert(x.akind == addr_kind::imm && x.width == 1);
				add_one(addr_kind::imp, 0, x.byte, x.flags);
			}
			if(x.akind == addr_kind::a) {
				add_one(addr_kind::imp, 0, x.byte, x.flags);
			}
			if(x.flags & flag_implied_rep) {
				add_one(addr_kind::imm, 0, x.byte, x.flags);
			}
			if(x.flags & (flag_imm_width_a | flag_imm_width_xy)) {
				assert(x.akind == addr_kind::imm && x.width == 1);
				add_one(x.akind, 2, x.byte, x.flags);
			}
		}
	}
};

static constexpr auto mnemonic_lookup = frozen::make_unordered_map<frozen::string, mnemonicinfo>({
	{ "adc", { { 0x65, addr_kind::abs    , 1 },
	           { 0x6d, addr_kind::abs    , 2 },
	           { 0x6f, addr_kind::abs    , 3 },
	           { 0x69, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0x72, addr_kind::ind    , 1 },
	           { 0x71, addr_kind::indy   , 1 },
	           { 0x67, addr_kind::lind   , 1 },
	           { 0x77, addr_kind::lindy  , 1 },
	           { 0x63, addr_kind::s      , 1 },
	           { 0x73, addr_kind::sy     , 1 },
	           { 0x75, addr_kind::x      , 1 },
	           { 0x7d, addr_kind::x      , 2 },
	           { 0x7f, addr_kind::x      , 3 },
	           { 0x61, addr_kind::xind   , 1 },
	           { 0x79, addr_kind::y      , 2 } } },
	{ "and", { { 0x25, addr_kind::abs    , 1 },
	           { 0x2d, addr_kind::abs    , 2 },
	           { 0x2f, addr_kind::abs    , 3 },
	           { 0x29, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0x32, addr_kind::ind    , 1 },
	           { 0x31, addr_kind::indy   , 1 },
	           { 0x27, addr_kind::lind   , 1 },
	           { 0x37, addr_kind::lindy  , 1 },
	           { 0x23, addr_kind::s      , 1 },
	           { 0x33, addr_kind::sy     , 1 },
	           { 0x35, addr_kind::x      , 1 },
	           { 0x3d, addr_kind::x      , 2 },
	           { 0x3f, addr_kind::x      , 3 },
	           { 0x21, addr_kind::xind   , 1 },
	           { 0x39, addr_kind::y      , 2 } } },
	{ "asl", { { 0x0a, addr_kind::a      , 0, flag_implied_rep },
	           { 0x06, addr_kind::abs    , 1 },
	           { 0x0e, addr_kind::abs    , 2 },
	           { 0x16, addr_kind::x      , 1 },
	           { 0x1e, addr_kind::x      , 2 } } },
	{ "bcc", { { 0x90, addr_kind::abs    , 1, flag_branch } } },
	{ "bcs", { { 0xb0, addr_kind::abs    , 1, flag_branch } } },
	{ "beq", { { 0xf0, addr_kind::abs    , 1, flag_branch } } },
	{ "bit", { { 0x24, addr_kind::abs    , 1 },
	           { 0x2c, addr_kind::abs    , 2 },
	           { 0x89, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0x34, addr_kind::x      , 1 },
	           { 0x3c, addr_kind::x      , 2 } } },
	{ "bmi", { { 0x30, addr_kind::abs    , 1, flag_branch } } },
	{ "bne", { { 0xd0, addr_kind::abs    , 1, flag_branch } } },
	{ "bpl", { { 0x10, addr_kind::abs    , 1, flag_branch } } },
	{ "bra", { { 0x80, addr_kind::abs    , 1, flag_branch } } },
	{ "brk", { { 0x00, addr_kind::imm    , 1, flag_imm_implied_0 } } },
	{ "brl", { { 0x82, addr_kind::abs    , 2, flag_branch } } },
	{ "bvc", { { 0x50, addr_kind::abs    , 1, flag_branch } } },
	{ "bvs", { { 0x70, addr_kind::abs    , 1, flag_branch } } },
	{ "clc", { { 0x18, addr_kind::imp    , 0 } } },
	{ "cld", { { 0xd8, addr_kind::imp    , 0 } } },
	{ "cli", { { 0x58, addr_kind::imp    , 0 } } },
	{ "clv", { { 0xb8, addr_kind::imp    , 0 } } },
	{ "cmp", { { 0xc5, addr_kind::abs    , 1 },
	           { 0xcd, addr_kind::abs    , 2 },
	           { 0xcf, addr_kind::abs    , 3 },
	           { 0xc9, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0xd2, addr_kind::ind    , 1 },
	           { 0xd1, addr_kind::indy   , 1 },
	           { 0xc7, addr_kind::lind   , 1 },
	           { 0xd7, addr_kind::lindy  , 1 },
	           { 0xc3, addr_kind::s      , 1 },
	           { 0xd3, addr_kind::sy     , 1 },
	           { 0xd5, addr_kind::x      , 1 },
	           { 0xdd, addr_kind::x      , 2 },
	           { 0xdf, addr_kind::x      , 3 },
	           { 0xc1, addr_kind::xind   , 1 },
	           { 0xd9, addr_kind::y      , 2 } } },
	{ "cop", { { 0x02, addr_kind::imm    , 1, flag_imm_implied_0 } } },
	{ "cpx", { { 0xe4, addr_kind::abs    , 1 },
	           { 0xec, addr_kind::abs    , 2 },
	           { 0xe0, addr_kind::imm    , 1, flag_imm_width_xy } } },
	{ "cpy", { { 0xc4, addr_kind::abs    , 1 },
	           { 0xcc, addr_kind::abs    , 2 },
	           { 0xc0, addr_kind::imm    , 1, flag_imm_width_xy } } },
	{ "dec", { { 0x3a, addr_kind::a      , 0, flag_implied_rep },
	           { 0xc6, addr_kind::abs    , 1 },
	           { 0xce, addr_kind::abs    , 2 },
	           { 0xd6, addr_kind::x      , 1 },
	           { 0xde, addr_kind::x      , 2 } } },
	{ "dex", { { 0xca, addr_kind::imp    , 0, flag_implied_rep } } },
	{ "dey", { { 0x88, addr_kind::imp    , 0, flag_implied_rep } } },
	{ "eor", { { 0x45, addr_kind::abs    , 1 },
	           { 0x4d, addr_kind::abs    , 2 },
	           { 0x4f, addr_kind::abs    , 3 },
	           { 0x49, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0x52, addr_kind::ind    , 1 },
	           { 0x51, addr_kind::indy   , 1 },
	           { 0x47, addr_kind::lind   , 1 },
	           { 0x57, addr_kind::lindy  , 1 },
	           { 0x43, addr_kind::s      , 1 },
	           { 0x53, addr_kind::sy     , 1 },
	           { 0x55, addr_kind::x      , 1 },
	           { 0x5d, addr_kind::x      , 2 },
	           { 0x5f, addr_kind::x      , 3 },
	           { 0x41, addr_kind::xind   , 1 },
	           { 0x59, addr_kind::y      , 2 } } },
	{ "inc", { { 0x1a, addr_kind::a      , 0, flag_implied_rep },
	           { 0xe6, addr_kind::abs    , 1 },
	           { 0xee, addr_kind::abs    , 2 },
	           { 0xf6, addr_kind::x      , 1 },
	           { 0xfe, addr_kind::x      , 2 } } },
	{ "inx", { { 0xe8, addr_kind::imp    , 0, flag_implied_rep } } },
	{ "iny", { { 0xc8, addr_kind::imp    , 0, flag_implied_rep } } },
	{ "jml", { { 0x5c, addr_kind::abs    , 3 },
	           { 0xdc, addr_kind::lind   , 2, flag_bank_0 } } },
	{ "jmp", { { 0x4c, addr_kind::abs    , 2, flag_bank_k },
	           { 0x6c, addr_kind::ind    , 2, flag_bank_0 },
	           { 0xdc, addr_kind::lind   , 2, flag_bank_0 },
	           { 0x7c, addr_kind::xind   , 2, flag_bank_k } } },
	{ "jsl", { { 0x22, addr_kind::abs    , 3 } } },
	{ "jsr", { { 0x20, addr_kind::abs    , 2, flag_bank_k },
	           { 0xfc, addr_kind::xind   , 2, flag_bank_k } } },
	{ "lda", { { 0xa5, addr_kind::abs    , 1 },
	           { 0xad, addr_kind::abs    , 2 },
	           { 0xaf, addr_kind::abs    , 3 },
	           { 0xa9, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0xb2, addr_kind::ind    , 1 },
	           { 0xb1, addr_kind::indy   , 1 },
	           { 0xa7, addr_kind::lind   , 1 },
	           { 0xb7, addr_kind::lindy  , 1 },
	           { 0xa3, addr_kind::s      , 1 },
	           { 0xb3, addr_kind::sy     , 1 },
	           { 0xb5, addr_kind::x      , 1 },
	           { 0xbd, addr_kind::x      , 2 },
	           { 0xbf, addr_kind::x      , 3 },
	           { 0xa1, addr_kind::xind   , 1 },
	           { 0xb9, addr_kind::y      , 2 } } },
	{ "ldx", { { 0xa6, addr_kind::abs    , 1 },
	           { 0xae, addr_kind::abs    , 2 },
	           { 0xa2, addr_kind::imm    , 1, flag_imm_width_xy },
	           { 0xb6, addr_kind::y      , 1 },
	           { 0xbe, addr_kind::y      , 2 } } },
	{ "ldy", { { 0xa4, addr_kind::abs    , 1 },
	           { 0xac, addr_kind::abs    , 2 },
	           { 0xa0, addr_kind::imm    , 1, flag_imm_width_xy },
	           { 0xb4, addr_kind::x      , 1 },
	           { 0xbc, addr_kind::x      , 2 } } },
	{ "lsr", { { 0x4a, addr_kind::a      , 0, flag_implied_rep },
	           { 0x46, addr_kind::abs    , 1 },
	           { 0x4e, addr_kind::abs    , 2 },
	           { 0x56, addr_kind::x      , 1 },
	           { 0x5e, addr_kind::x      , 2 } } },
	{ "mvn", { { 0x54, addr_kind::mvn    , 2 } } },
	{ "mvp", { { 0x44, addr_kind::mvn    , 2 } } },
	{ "nop", { { 0xea, addr_kind::imp    , 0, flag_implied_rep } } },
	{ "ora", { { 0x05, addr_kind::abs    , 1 },
	           { 0x0d, addr_kind::abs    , 2 },
	           { 0x0f, addr_kind::abs    , 3 },
	           { 0x09, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0x12, addr_kind::ind    , 1 },
	           { 0x11, addr_kind::indy   , 1 },
	           { 0x07, addr_kind::lind   , 1 },
	           { 0x17, addr_kind::lindy  , 1 },
	           { 0x03, addr_kind::s      , 1 },
	           { 0x13, addr_kind::sy     , 1 },
	           { 0x15, addr_kind::x      , 1 },
	           { 0x1d, addr_kind::x      , 2 },
	           { 0x1f, addr_kind::x      , 3 },
	           { 0x01, addr_kind::xind   , 1 },
	           { 0x19, addr_kind::y      , 2 } } },
	{ "pea", { { 0xf4, addr_kind::abs    , 2 } } },
	{ "pei", { { 0xd4, addr_kind::ind    , 1 } } },
	{ "per", { { 0x62, addr_kind::abs    , 2, flag_branch } } },
	{ "pha", { { 0x48, addr_kind::imp    , 0 } } },
	{ "phb", { { 0x8b, addr_kind::imp    , 0 } } },
	{ "phd", { { 0x0b, addr_kind::imp    , 0 } } },
	{ "phk", { { 0x4b, addr_kind::imp    , 0 } } },
	{ "php", { { 0x08, addr_kind::imp    , 0 } } },
	{ "phx", { { 0xda, addr_kind::imp    , 0 } } },
	{ "phy", { { 0x5a, addr_kind::imp    , 0 } } },
	{ "pla", { { 0x68, addr_kind::imp    , 0 } } },
	{ "plb", { { 0xab, addr_kind::imp    , 0 } } },
	{ "pld", { { 0x2b, addr_kind::imp    , 0 } } },
	{ "plp", { { 0x28, addr_kind::imp    , 0 } } },
	{ "plx", { { 0xfa, addr_kind::imp    , 0 } } },
	{ "ply", { { 0x7a, addr_kind::imp    , 0 } } },
	{ "rep", { { 0xc2, addr_kind::imm    , 1 } } },
	{ "rol", { { 0x2a, addr_kind::a      , 0, flag_implied_rep },
	           { 0x26, addr_kind::abs    , 1 },
	           { 0x2e, addr_kind::abs    , 2 },
	           { 0x36, addr_kind::x      , 1 },
	           { 0x3e, addr_kind::x      , 2 } } },
	{ "ror", { { 0x6a, addr_kind::a      , 0, flag_implied_rep },
	           { 0x66, addr_kind::abs    , 1 },
	           { 0x6e, addr_kind::abs    , 2 },
	           { 0x76, addr_kind::x      , 1 },
	           { 0x7e, addr_kind::x      , 2 } } },
	{ "rti", { { 0x40, addr_kind::imp    , 0 } } },
	{ "rtl", { { 0x6b, addr_kind::imp    , 0 } } },
	{ "rts", { { 0x60, addr_kind::imp    , 0 } } },
	{ "sbc", { { 0xe5, addr_kind::abs    , 1 },
	           { 0xed, addr_kind::abs    , 2 },
	           { 0xef, addr_kind::abs    , 3 },
	           { 0xe9, addr_kind::imm    , 1, flag_imm_width_a },
	           { 0xf2, addr_kind::ind    , 1 },
	           { 0xf1, addr_kind::indy   , 1 },
	           { 0xe7, addr_kind::lind   , 1 },
	           { 0xf7, addr_kind::lindy  , 1 },
	           { 0xe3, addr_kind::s      , 1 },
	           { 0xf3, addr_kind::sy     , 1 },
	           { 0xf5, addr_kind::x      , 1 },
	           { 0xfd, addr_kind::x      , 2 },
	           { 0xff, addr_kind::x      , 3 },
	           { 0xe1, addr_kind::xind   , 1 },
	           { 0xf9, addr_kind::y      , 2 } } },
	{ "sec", { { 0x38, addr_kind::imp    , 0 } } },
	{ "sed", { { 0xf8, addr_kind::imp    , 0 } } },
	{ "sei", { { 0x78, addr_kind::imp    , 0 } } },
	{ "sep", { { 0xe2, addr_kind::imm    , 1 } } },
	{ "sta", { { 0x85, addr_kind::abs    , 1 },
	           { 0x8d, addr_kind::abs    , 2 },
	           { 0x8f, addr_kind::abs    , 3 },
	           { 0x92, addr_kind::ind    , 1 },
	           { 0x91, addr_kind::indy   , 1 },
	           { 0x87, addr_kind::lind   , 1 },
	           { 0x97, addr_kind::lindy  , 1 },
	           { 0x83, addr_kind::s      , 1 },
	           { 0x93, addr_kind::sy     , 1 },
	           { 0x95, addr_kind::x      , 1 },
	           { 0x9d, addr_kind::x      , 2 },
	           { 0x9f, addr_kind::x      , 3 },
	           { 0x81, addr_kind::xind   , 1 },
	           { 0x99, addr_kind::y      , 2 } } },
	{ "stp", { { 0xdb, addr_kind::imp    , 0 } } },
	{ "stx", { { 0x86, addr_kind::abs    , 1 },
	           { 0x8e, addr_kind::abs    , 2 },
	           { 0x96, addr_kind::y      , 1 } } },
	{ "sty", { { 0x84, addr_kind::abs    , 1 },
	           { 0x8c, addr_kind::abs    , 2 },
	           { 0x94, addr_kind::x      , 1 } } },
	{ "stz", { { 0x64, addr_kind::abs    , 1 },
	           { 0x9c, addr_kind::abs    , 2 },
	           { 0x74, addr_kind::x      , 1 },
	           { 0x9e, addr_kind::x      , 2 } } },
	{ "tax", { { 0xaa, addr_kind::imp    , 0 } } },
	{ "tay", { { 0xa8, addr_kind::imp    , 0 } } },
	{ "tcd", { { 0x5b, addr_kind::imp    , 0 } } },
	{ "tcs", { { 0x1b, addr_kind::imp    , 0 } } },
	{ "tdc", { { 0x7b, addr_kind::imp    , 0 } } },
	{ "trb", { { 0x14, addr_kind::abs    , 1 },
	           { 0x1c, addr_kind::abs    , 2 } } },
	{ "tsb", { { 0x04, addr_kind::abs    , 1 },
	           { 0x0c, addr_kind::abs    , 2 } } },
	{ "tsc", { { 0x3b, addr_kind::imp    , 0 } } },
	{ "tsx", { { 0xba, addr_kind::imp    , 0 } } },
	{ "txa", { { 0x8a, addr_kind::imp    , 0 } } },
	{ "txs", { { 0x9a, addr_kind::imp    , 0 } } },
	{ "txy", { { 0x9b, addr_kind::imp    , 0 } } },
	{ "tya", { { 0x98, addr_kind::imp    , 0 } } },
	{ "tyx", { { 0xbb, addr_kind::imp    , 0 } } },
	{ "wai", { { 0xcb, addr_kind::imp    , 0 } } },
	{ "wdm", { { 0x42, addr_kind::imm    , 1, flag_imm_implied_0 } } },
	{ "xba", { { 0xeb, addr_kind::imp    , 0 } } },
	{ "xce", { { 0xfb, addr_kind::imp    , 0 } } },
});

struct parse_result {
	addr_kind kind;
	string arg;
};

// checks for matching characters at the start of haystack, ignoring spaces.
// returns index into haystack right after the match
template<char... chars>
static int64_t startmatch(const string& haystack) {
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
static int64_t endmatch(const string& haystack) {
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
static parse_result parse_addr_kind(const string& arg, uint32_t allowed_kinds_mask) {
	int64_t start_i = 0, end_i = arg.length();
// If this addressing kind is allowed, return it, along with a trimmed version of the string.
#define RETURN_IF_ALLOWED(kind) \
		if (allowed_kinds_mask & 1<<(uint32_t)addr_kind::kind) { \
			string out(arg.data() + start_i, end_i - start_i); \
			return parse_result{addr_kind::kind, out}; \
		}
	if((start_i = startmatch<'#'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(imm);
		throw_err_block(1, err_bad_addr_mode);
	}
	if((start_i = startmatch<'('>(arg)) >= 0) {
		// TODO check if the end of this paren is where it should be
		if((end_i = endmatch<',', 's', ')', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(sy);
			throw_err_block(1, err_bad_addr_mode);
		}
		if((end_i = endmatch<')', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(indy);
			throw_err_block(1, err_bad_addr_mode);
		}
		if((end_i = endmatch<',', 'x', ')'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(xind);
			throw_err_block(1, err_bad_addr_mode);
		}
		if((end_i = endmatch<')'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(ind);
			throw_warning(1, warn_assuming_address_mode, "($00)", "$00", " (if this was intentional, add a +0 after the parentheses.)");
		}
	}
	if((start_i = startmatch<'['>(arg)) >= 0) {
		if((end_i = endmatch<']', ',', 'y'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(lindy);
			throw_err_block(1, err_bad_addr_mode);
		}
		if((end_i = endmatch<']'>(arg)) >= 0) {
			RETURN_IF_ALLOWED(lind);
			throw_err_block(1, err_bad_addr_mode);
		}
	}
	start_i = 0;
	if((end_i = endmatch<',', 'x'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(x);
		throw_err_block(1, err_bad_addr_mode);
	}
	if((end_i = endmatch<',', 'y'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(y);
		throw_err_block(1, err_bad_addr_mode);
	}
	if((end_i = endmatch<',', 's'>(arg)) >= 0) {
		RETURN_IF_ALLOWED(s);
		throw_err_block(1, err_bad_addr_mode);
	}
	end_i = arg.length();
	// hoping that by now, something would have stripped whitespace lol
	if(arg.length() == 0) {
		RETURN_IF_ALLOWED(imp);
		throw_err_block(1, err_bad_addr_mode);
	}
	if(arg == "a" || arg == "A") {
		RETURN_IF_ALLOWED(a);
		// todo: some hint for "don't name your label "a" "?
		throw_err_block(1, err_bad_addr_mode);
	}
	RETURN_IF_ALLOWED(abs);
	throw_err_block(1, err_bad_addr_mode);
#undef RETURN_IF_ALLOWED
}

static const char* format_valid_widths(int min, int max) {
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

static int get_real_len(int min_len, int max_len, int arg_min_len, char modifier, const parse_result& parsed) {
	int out_len;
	if(modifier != 0) {
		out_len = getlenfromchar(modifier);
		if(out_len < min_len || out_len > max_len)
			throw_err_block(2, err_bad_access_width, format_valid_widths(min_len, max_len), out_len*8);
	} else {
		if(parsed.kind == addr_kind::imm) {
			if(!is_hex_constant(parsed.arg.data()))
				throw_warning(2, warn_implicitly_sized_immediate);
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

			throw_err_block(2, err_bad_access_width, format_valid_widths(min_len, max_len), arg_min_len*8);
		}
		// todo warn about widening when dpbase != 0
		out_len = std::max(arg_min_len, min_len);
	}
	return out_len;
}

static int64_t get_branch_value(parse_result& parsed, char modifier, int width) {
	auto expr = parse_math_expr(parsed.arg);
	int64_t num = expr->evaluate().get_integer();
	bool target_is_abs = expr->has_label() > 0;
	if(modifier != 0) {
		if(to_lower(modifier) == 'a') target_is_abs = true;
		else if(to_lower(modifier) == 'r') target_is_abs = false;
		// ignore for backwards compat
		else if(to_lower(modifier) == (width == 2 ? 'w' : 'b')) {}
		// TODO: better error message
		else throw_err_block(2, err_invalid_opcode_length);
	}
	if(target_is_abs) {
		// cast delta to signed 16-bit, this makes it possible to handle bank-border-wrapping automatically
		int16_t delta = num - (snespos + width + 1);
		if((num & ~0xffff) != (snespos & ~0xffff)) {
			// todo: should throw error "can't branch to different bank"
			throw_err_block(2, err_relative_branch_out_of_bounds, dec(delta).data());
		}
		if(width==1 && (delta < -128 || delta > 127)) {
			throw_err_block(2, err_relative_branch_out_of_bounds, dec(delta).data());
		}
		return delta;
	} else {
		if(num & ~(width==2 ? 0xffff : 0xff)) {
			throw_err_block(2, err_relative_branch_out_of_bounds, dec(num).data());
		}
		return (int16_t)(width==2 ? num : (int8_t)num);
	}
}

bool asblock_65816(const string& firstword, const char* par)
{
	// first find the mnemonic from the first word
	bool autoclean = false;
	string mnem;
	if(!stricmpwithlower(firstword, "autoclean")) {
		autoclean = true;
		grab_until_space(mnem, par);
	} else {
		mnem = firstword;
	}
	lower(mnem);

	char modifier = 0;
	if(mnem.length() >= 2 && mnem[mnem.length()-2] == '.') {
		modifier = mnem[mnem.length()-1];
		mnem.truncate(mnem.length()-2);
	}

	frozen::string mnem2(mnem.data(), mnem.length());
	auto it = mnemonic_lookup.find(mnem2);
	if(it == mnemonic_lookup.end()) return false;
	const mnemonicinfo& mnem_info = it->second;

	// check if we're doing mvn/mvp, and if yes, parse completely differently
	// this is a little hacky, sorry...
	if(mnem_info.allowed_kinds_mask & 1<<(uint32_t)addr_kind::mvn) {
		int count = 0;
		string parbuf = par;
		autoptr<char**> parts = qpsplit(parbuf.raw(), ',', &count);
		if(count != 2) throw_err_block(2, err_bad_addr_mode);
		uint8_t opcode = mnem_info.types[(int)addr_kind::mvn].opcode_for_width[2];
		write1(opcode);
		int64_t num1 = 0, num2 = 0;
		if(pass == 2) {
			num1 = getnum(parts[0]);
			num2 = getnum(parts[1]);
		}
		if(num1 < 0 || num1 > 255) throw_err_block(2, err_bad_access_width, format_valid_widths(1, 1), num1 > 65535 ? 24 : 16);
		if(num2 < 0 || num2 > 255) throw_err_block(2, err_bad_access_width, format_valid_widths(1, 1), num2 > 65535 ? 24 : 16);
		write1(num1);
		write1(num2);
		// a bit hacky to check this here, but we kinda do need to early-return here
		if(autoclean) throw_err_block(2, err_broken_autoclean);
		return true;
	}
	parse_result parse_res = parse_addr_kind(par, mnem_info.allowed_kinds_mask);
	const mnem_kind_info& kind_info = mnem_info.types[(uint32_t)parse_res.kind];

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
		rep_count = parse_math_expr(parse_res.arg)->evaluate_static().get_integer();
	} else if(parse_res.kind == addr_kind::imp || parse_res.kind == addr_kind::a) {
		arg_len = 0;
	} else {
		int old_optimize = optimizeforbank;
		if(kind_info.flags & flag_bank_0) optimizeforbank = 0;
		else if(kind_info.flags & flag_bank_k) optimizeforbank = -1;
		auto math_parsed = parse_math_expr(parse_res.arg);
		int arg_min_len = math_parsed->get_len(parse_res.kind == addr_kind::imm);
		arg_len = get_real_len(min_w, max_w, arg_min_len, modifier, parse_res);
		optimizeforbank = old_optimize;
		arg_value = pass == 2 ? math_parsed->evaluate().get_integer() : 0;
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
		if(arg_len != 3) throw_err_block(2, err_broken_autoclean);
		handle_autoclean(parse_res.arg, opcode, snespos - 4);
	}
	return true;
}
