#include <string>
#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"

#define write1 write1_pick

void asinit_65816()
{
}

void asend_65816()
{
}

// like an address mode, but without a specific width
enum class addr_kind {
	abs, // $00
	x, // $00,x
	y, // $00,y
	ind, // ($00)
	lind, // [$00]
	xind, // ($00,x)
	indy, // ($00),y
	lindy, // [$00],y
	s, // $00,s
	sy, // ($00,s),y
	imp, // implied (no argument)
	a, // 'A' as argument
	imm, // #$00
};

struct insn_context {
	string arg;
	char orig_insn[3]; // oops i didn't end up using this... could be useful in some error messages maybe
	char modifier;
};

// checks for matching characters at the start of haystack, ignoring spaces.
// returns index into haystack right after the match
template<char... chars>
ssize_t startmatch(const string& haystack) {
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
ssize_t endmatch(const string& haystack) {
	static const char needle[] = {chars...};
	ssize_t haystack_i = haystack.length()-1;
	for(ssize_t i = sizeof...(chars)-1; i >= 0; i--) {
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
template<addr_kind... allowed_kinds>
std::pair<addr_kind, string> parse_addr_kind(insn_context& arg) {
	ssize_t start_i = 0, end_i = arg.arg.length();
// If this addressing kind is allowed, return it, along with a trimmed version of the string.
#define RETURN_IF_ALLOWED(kind) \
		if constexpr(((allowed_kinds == addr_kind::kind) || ...)) { \
			string out(arg.arg.data() + start_i, end_i - start_i); \
			return std::make_pair(addr_kind::kind, out); \
		}
	if((start_i = startmatch<'#'>(arg.arg)) >= 0) {
		RETURN_IF_ALLOWED(imm);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((start_i = startmatch<'('>(arg.arg)) >= 0) {
		if((end_i = endmatch<',', 's', ')', ',', 'y'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(sy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')', ',', 'y'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(indy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<',', 'x', ')'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(xind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(ind);
			asar_throw_warning(1, warning_id_assuming_address_mode, "($00)", "$00", " (if this was intentional, add a +0 after the parentheses.)");
		}
	}
	if((start_i = startmatch<'['>(arg.arg)) >= 0) {
		if((end_i = endmatch<']', ',', 'y'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(lindy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<']'>(arg.arg)) >= 0) {
			RETURN_IF_ALLOWED(lind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
	}
	start_i = 0;
	if((end_i = endmatch<',', 'x'>(arg.arg)) >= 0) {
		RETURN_IF_ALLOWED(x);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 'y'>(arg.arg)) >= 0) {
		RETURN_IF_ALLOWED(y);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 's'>(arg.arg)) >= 0) {
		RETURN_IF_ALLOWED(s);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	// hoping that by now, something would have stripped whitespace lol
	if(arg.arg.length() == 0) {
		RETURN_IF_ALLOWED(imp);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if(arg.arg == "a" || arg.arg == "A") {
		RETURN_IF_ALLOWED(a);
		// todo: some hint for "don't name your label "a" "?
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if constexpr(((allowed_kinds == addr_kind::abs) || ...)) {
		return std::make_pair(addr_kind::abs, arg.arg);
	}
	asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
#undef RETURN_IF_ALLOWED
	// clang doesn't know that asar_throw_error is noreturn in this case... :(
	// we don't really have anything good to return either tho
	__builtin_unreachable();
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

int get_real_len(int min_len, int max_len, char modifier, int arg_min_len) {
	if(modifier != 0) {
		int given_len = getlenfromchar(modifier);
		if(given_len < min_len || given_len > max_len)
			asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(min_len, max_len), given_len*8);
		return given_len;
	} else {
		if(arg_min_len > max_len) {
			asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(min_len, max_len), arg_min_len*8);
		}
		// todo warn about widening when dpbase != 0
		return std::max(arg_min_len, min_len);
	}
}

void check_implicit_immediate(char modifier, const string& target) {
	if(modifier != 0) return;
	if(is_hex_constant(target.data())) return;
	asar_throw_warning(2, warning_id_implicitly_sized_immediate);
}

template<int base, bool allow_imm = true>
void the8(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::xind, K::s, K::abs, K::lind, K::imm, K::indy, K::ind, K::sy, K::x, K::y, K::lindy>(arg);
	addr_kind kind = parse_result.first;
	int min_len = 0, max_len = 4;
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	int arg_min_len = getlen(parse_result.second, kind == K::imm);
	if(kind == K::xind || kind == K::s || kind == K::sy || kind == K::indy || kind == K::ind || kind == K::lind || kind == K::lindy) min_len = max_len = 1;
	else if(kind == K::abs) min_len = 1, max_len = 3;
	else if(kind == K::x) min_len = 1, max_len = 3;
	else if(kind == K::y) min_len = max_len = 2;
	else if(kind == K::imm) min_len = 1, max_len = 2;
	if(kind == K::imm) check_implicit_immediate(arg.modifier, parse_result.second);
	int real_len = get_real_len(min_len, max_len, arg.modifier, arg_min_len);
	int opcode_offset = 0;
	if(real_len == 1) {
		if(kind == K::xind) opcode_offset = 0x1;
		if(kind == K::s) opcode_offset = 0x3;
		if(kind == K::abs) opcode_offset = 0x5;
		if(kind == K::lind) opcode_offset = 0x7;
		if(kind == K::imm) opcode_offset = 0x9;
		if(kind == K::indy) opcode_offset = 0x11;
		if(kind == K::ind) opcode_offset = 0x12;
		if(kind == K::sy) opcode_offset = 0x13;
		if(kind == K::x) opcode_offset = 0x15;
		if(kind == K::lindy) opcode_offset = 0x17;
	} else if(real_len == 2) {
		if(kind == K::imm) opcode_offset = 0x9;
		if(kind == K::abs) opcode_offset = 0xd;
		if(kind == K::y) opcode_offset = 0x19;
		if(kind == K::x) opcode_offset = 0x1d;
	} else if(real_len == 3) {
		if(kind == K::abs) opcode_offset = 0xf;
		if(kind == K::x) opcode_offset = 0x1f;
	}
	write1(opcode_offset + base);
	if(real_len == 1) write1(the_num);
	else if(real_len == 2) write2(the_num);
	else if(real_len == 3) write3(the_num);
}

template<int base, int accum_opc, bool is_bit = false>
void thenext8(insn_context& arg) {
	using K = addr_kind;
	addr_kind kind;
	string parsed_str;
	if constexpr(is_bit) {
		auto parse_result = parse_addr_kind<K::x, K::abs, K::imm>(arg);
		kind = parse_result.first;
		parsed_str = parse_result.second;
	} else {
		auto parse_result = parse_addr_kind<K::x, K::abs, K::imm, K::a, K::imp>(arg);
		kind = parse_result.first;
		parsed_str = parse_result.second;
		// todo: some checks on arg.modifier here
		if(kind == K::imm) {
			// implied rep
			int64_t rep_count = getnum(parsed_str);
			if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
			for (int64_t i=0;i<rep_count;i++) { write1(accum_opc); }
			return;
		}
		if(kind == K::a || kind == K::imp) {
			write1(accum_opc);
			return;
		}
	}
	int64_t the_num = pass == 2 ? getnum(parsed_str) : 0;
	int arg_min_len = getlen(parsed_str, kind == K::imm);
	int real_len = get_real_len(1, 2, arg.modifier, arg_min_len);
	int opcode = 0;
	if(kind == K::imm) {
		check_implicit_immediate(arg.modifier, parsed_str);
		opcode = accum_opc;
	} else if(real_len == 1) {
		if(kind == K::abs) opcode = base+0x6;
		if(kind == K::x) opcode = base+0x16;
	} else if(real_len == 2) {
		if(kind == K::abs) opcode = base+0xe;
		if(kind == K::x) opcode = base+0x1e;
	}
	write1(opcode);
	if(real_len == 1) write1(the_num);
	else if(real_len == 2) write2(the_num);
}

template<int base>
void tsb_trb(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::abs>(arg);
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	int arg_min_len = getlen(parse_result.second, false);
	int real_len = get_real_len(1, 2, arg.modifier, arg_min_len);
	int opcode_offset = real_len == 1 ? 0x04 : 0x0c;
	write1(opcode_offset + base);
	if(real_len == 1) write1(the_num);
	else if(real_len == 2) write2(the_num);
}

template<int opc, int width = 1>
void branch(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::abs>(arg);
	int64_t num = 0;
	if(pass == 2) {
		num = getnum(parse_result.second);
		if(foundlabel) {
			int64_t delta = num - (snespos + width + 1);
			if((num & ~0xffff) != (snespos & ~0xffff)) {
				// todo: should throw error "can't branch to different bank"
				asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(delta).data());
			}
			if(width==1 && (delta < -128 || delta > 127)) {
				asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(delta).data());
			}
			num = delta;
		} else {
			if(num & ~(width==2 ? 0xffff : 0xff)) {
				asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(num).data());
			}
			num = (int16_t)(width==2 ? num : (int8_t)num);
		}
	}

	write1(opc);
	if(width == 1) write1(num);
	else if(width == 2) write2(num);
}

template<int opc>
void implied(insn_context& arg) {
	if(arg.arg != "") {
		// todo: some kind of "this instruction doesn't take an argument" message?
		asar_throw_error(0, error_type_block, error_id_bad_addr_mode);
	}
	write1(opc);
}

template<int opc>
void implied_rep(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::imm, K::imp>(arg);
	if(parse_result.first == K::imp) {
		write1(opc);
	} else {
		int64_t rep_count = getnum(parse_result.second);
		if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
		for (int64_t i=0;i<rep_count;i++) { write1(opc); }
	}
}

template<int base, char op, char xy>
void xy_ops(insn_context& arg) {
	using K = addr_kind;
	std::pair<addr_kind, string> parse_result;
	if(op == 'S') { // stx
		parse_result = parse_addr_kind<K::abs, (xy == 'X' ? K::y : K::x)>(arg);
	} else if(op == 'L') { // ldx
		parse_result = parse_addr_kind<K::abs, K::imm, (xy == 'X' ? K::y : K::x)>(arg);
	} else { // cpx
		parse_result = parse_addr_kind<K::abs, K::imm>(arg);
	}
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	int arg_min_len = getlen(parse_result.second, false);
	int real_len = get_real_len(1, 2, arg.modifier, arg_min_len);
	int opcode = 0;
	if(parse_result.first == K::imm) {
		check_implicit_immediate(arg.modifier, parse_result.second);
		opcode = base;
	} else if(parse_result.first == K::abs) {
		if(real_len == 1) opcode = base + 0x4;
		else opcode = base + 0xc;
	} else { // ,x or ,y
		if(real_len == 1) opcode = base + 0x14;
		else opcode = base + 0x1c;
	}
	write1(opcode);
	if(real_len == 1) write1(the_num);
	else if(real_len == 2) write2(the_num);
}

template<int opc>
void interrupt(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::imm, K::imp>(arg);
	if(parse_result.first == K::imp) {
		write1(opc);
		write1(0);
	} else {
		int64_t num = pass == 2 ? getnum(parse_result.second) : 0;
		// this is kinda hacky
		if(num < 0 || num > 255) asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(1, 1), num > 65535 ? 24 : 16);
		write1(opc);
		write1(num);
	}
}

template<int opc, addr_kind k, int width>
void oneoff(insn_context& arg) {
	auto parse_result = parse_addr_kind<k>(arg);
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	int arg_min_len = getlen(parse_result.second, false);
	// only for error checking, we know the answer anyways
	get_real_len(width, width, arg.modifier, arg_min_len);
	write1(opc);
	if(width == 1) write1(the_num);
	else if(width == 2) write2(the_num);
	else if(width == 3) write3(the_num);
}

void stz(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = parse_addr_kind<K::abs, K::x>(arg);
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	int arg_min_len = getlen(parse_result.second, false);
	int real_len = get_real_len(1, 2, arg.modifier, arg_min_len);
	if(real_len == 1) {
		if(parse_result.first == K::abs) write1(0x64);
		else if(parse_result.first == K::x) write1(0x74);
		write1(the_num);
	} else {
		if(parse_result.first == K::abs) write1(0x9c);
		else if(parse_result.first == K::x) write1(0x9e);
		write2(the_num);
	}
}

template<char which>
void jmp_jsr_jml(insn_context& arg) {
	using K = addr_kind;
	auto parse_result = which == 'R' ? parse_addr_kind<K::abs, K::xind>(arg)
		: which == 'P' ? parse_addr_kind<K::abs, K::xind, K::ind, K::lind>(arg)
			: /* 'L' */ parse_addr_kind<K::abs, K::lind>(arg);
	int64_t the_num = pass == 2 ? getnum(parse_result.second) : 0;
	// set optimizeforbank to -1 (i.e. auto, assume DBR = current bank)
	// because jmp and jsr's arguments are relative to the program bank anyways
	int old_optimize = optimizeforbank;
	if(parse_result.first == K::lind || parse_result.first == K::ind) {
		// these ones for Some Reason always read the pointer from bank 0 lol
		optimizeforbank = 0;
	} else {
		// the rest use bank K
		optimizeforbank = -1;
	}
	int arg_min_len = getlen(parse_result.second, false);
	optimizeforbank = old_optimize;
	get_real_len(2, (which == 'L' && parse_result.first == K::abs) ? 3 : 2, arg.modifier, arg_min_len);
	if(which == 'R') {
		if(parse_result.first == K::abs) write1(0x20);
		else if(parse_result.first == K::xind) write1(0xfc);
	} else if(which == 'L') {
		if(parse_result.first == K::abs) {
			write1(0x5c);
			write3(the_num);
			return;
		} else if(parse_result.first == K::lind) {
			write1(0xdc);
		}
	} else {
		if(parse_result.first == K::abs) write1(0x4c);
		else if(parse_result.first == K::ind) write1(0x6c);
		else if(parse_result.first == K::xind) write1(0x7c);
		else if(parse_result.first == K::lind) write1(0xdc);
	}
	write2(the_num);
}

template<int opc>
void mvn_mvp(insn_context& arg) {
	int count;
	autoptr<char**> parts = qpsplit(arg.arg.raw(), ',', &count);
	if(count != 2) asar_throw_error(2, error_type_block, error_id_bad_addr_mode);
	// todo length checks ???
	write1(opc);
	if(pass == 2) {
		write1(getnum(parts[0]));
		write1(getnum(parts[1]));
	} else {
		write2(0);
	}
}

assocarr<void(*)(insn_context&)> mnemonics = {
	{ "ora", the8<0x00> },
	{ "and", the8<0x20> },
	{ "eor", the8<0x40> },
	{ "adc", the8<0x60> },
	{ "sta", the8<0x80, false> },
	{ "lda", the8<0xa0> },
	{ "cmp", the8<0xc0> },
	{ "sbc", the8<0xe0> },
	{ "asl", thenext8<0x00, 0x0a> },
	{ "bit", thenext8<0x1e, 0x89, true> },
	{ "rol", thenext8<0x20, 0x2a> },
	{ "lsr", thenext8<0x40, 0x4a> },
	{ "ror", thenext8<0x60, 0x6a> },
	{ "dec", thenext8<0xc0, 0x3a> },
	{ "inc", thenext8<0xe0, 0x1a> },
	{ "bcc", branch<0x90> },
	{ "bcs", branch<0xb0> },
	{ "beq", branch<0xf0> },
	{ "bmi", branch<0x30> },
	{ "bne", branch<0xd0> },
	{ "bpl", branch<0x10> },
	{ "bra", branch<0x80> },
	{ "bvc", branch<0x50> },
	{ "bvs", branch<0x70> },
	{ "brl", branch<0x82, 2> },
	{ "clc", implied<0x18> },
	{ "cld", implied<0xd8> },
	{ "cli", implied<0x58> },
	{ "clv", implied<0xb8> },
	{ "dex", implied_rep<0xca> },
	{ "dey", implied_rep<0x88> },
	{ "inx", implied_rep<0xe8> },
	{ "iny", implied_rep<0xc8> },
	{ "nop", implied_rep<0xea> },
	{ "pha", implied<0x48> },
	{ "phb", implied<0x8b> },
	{ "phd", implied<0x0b> },
	{ "phk", implied<0x4b> },
	{ "php", implied<0x08> },
	{ "phx", implied<0xda> },
	{ "phy", implied<0x5a> },
	{ "pla", implied<0x68> },
	{ "plb", implied<0xab> },
	{ "pld", implied<0x2b> },
	{ "plp", implied<0x28> },
	{ "plx", implied<0xfa> },
	{ "ply", implied<0x7a> },
	{ "rti", implied<0x40> },
	{ "rtl", implied<0x6b> },
	{ "rts", implied<0x60> },
	{ "sec", implied<0x38> },
	{ "sed", implied<0xf8> },
	{ "sei", implied<0x78> },
	{ "stp", implied<0xdb> },
	{ "tax", implied<0xaa> },
	{ "tay", implied<0xa8> },
	{ "tcd", implied<0x5b> },
	{ "tcs", implied<0x1b> },
	{ "tdc", implied<0x7b> },
	{ "tsc", implied<0x3b> },
	{ "tsx", implied<0xba> },
	{ "txa", implied<0x8a> },
	{ "txs", implied<0x9a> },
	{ "txy", implied<0x9b> },
	{ "tya", implied<0x98> },
	{ "tyx", implied<0xbb> },
	{ "wai", implied<0xcb> },
	{ "xba", implied<0xeb> },
	{ "xce", implied<0xfb> },
	{ "ldy", xy_ops<0xa0, 'L', 'Y'> },
	{ "ldx", xy_ops<0xa2, 'L', 'X'> },
	{ "cpy", xy_ops<0xc0, 'C', 'Y'> },
	{ "cpx", xy_ops<0xe0, 'C', 'X'> },
	{ "stx", xy_ops<0x82, 'S', 'X'> },
	{ "sty", xy_ops<0x80, 'S', 'Y'> },
	{ "cop", interrupt<0x02> },
	{ "wdm", interrupt<0x42> },
	{ "brk", interrupt<0x00> },
	{ "tsb", tsb_trb<0x00> },
	{ "trb", tsb_trb<0x10> },
	{ "rep", oneoff<0xc2, addr_kind::imm, 1> },
	{ "sep", oneoff<0xe2, addr_kind::imm, 1> },
	{ "pei", oneoff<0xd4, addr_kind::ind, 1> },
	{ "pea", oneoff<0xf4, addr_kind::abs, 2> },
	{ "mvn", mvn_mvp<0x54> },
	{ "mvp", mvn_mvp<0x44> },
	{ "jsl", oneoff<0x22, addr_kind::abs, 3> },
	{ "per", branch<0x62, 2> },
	{ "stz", stz },
	{ "jmp", jmp_jsr_jml<'P'> },
	{ "jsr", jmp_jsr_jml<'R'> },
	{ "jml", jmp_jsr_jml<'L'> },
};


bool asblock_65816(char** word, int numwords)
{
#define is(test) (!stricmpwithupper(word[0], test))
	if(word[0][0] == '\'') return false;
	string par;
	for(int i = 1; i < numwords; i++){
		if(i > 1) par += " ";
		par += word[i];
	}
	// todo handle code like `nop = $1234`
	string opc = word[0];
	for(int i = 0; i < opc.length(); i++) opc.raw()[i] = to_lower(opc[i]);
	char mod = 0;
	if(opc.length() >= 2 && opc[opc.length()-2] == '.') {
		mod = opc[opc.length()-1];
		opc.truncate(opc.length()-2);
	}
	if(!mnemonics.exists(opc.data())) return false;
	insn_context ctx{par, {}, mod};
	ctx.orig_insn[0] = opc[0];
	ctx.orig_insn[1] = opc[1];
	ctx.orig_insn[2] = opc[2];
	mnemonics.find(opc.data())(ctx);
	return true;
}
