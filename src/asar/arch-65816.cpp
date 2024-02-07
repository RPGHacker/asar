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
	// right now these 2 are only used for autoclean, so not set for some weird opcodes
	string parsed_arg;
	int written_len;
	uint8_t written_opcode;
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
template<addr_kind... allowed_kinds>
parse_result parse_addr_kind(insn_context& ctx) {
	int64_t start_i = 0, end_i = ctx.arg.length();
// If this addressing kind is allowed, return it, along with a trimmed version of the string.
#define RETURN_IF_ALLOWED(kind) \
		if constexpr(((allowed_kinds == addr_kind::kind) || ...)) { \
			string out(ctx.arg.data() + start_i, end_i - start_i); \
			ctx.parsed_arg = out; \
			return parse_result{addr_kind::kind, out}; \
		}
	if((start_i = startmatch<'#'>(ctx.arg)) >= 0) {
		RETURN_IF_ALLOWED(imm);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((start_i = startmatch<'('>(ctx.arg)) >= 0) {
		if((end_i = endmatch<',', 's', ')', ',', 'y'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(sy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')', ',', 'y'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(indy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<',', 'x', ')'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(xind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<')'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(ind);
			asar_throw_warning(1, warning_id_assuming_address_mode, "($00)", "$00", " (if this was intentional, add a +0 after the parentheses.)");
		}
	}
	if((start_i = startmatch<'['>(ctx.arg)) >= 0) {
		if((end_i = endmatch<']', ',', 'y'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(lindy);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
		if((end_i = endmatch<']'>(ctx.arg)) >= 0) {
			RETURN_IF_ALLOWED(lind);
			asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
		}
	}
	start_i = 0;
	if((end_i = endmatch<',', 'x'>(ctx.arg)) >= 0) {
		RETURN_IF_ALLOWED(x);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 'y'>(ctx.arg)) >= 0) {
		RETURN_IF_ALLOWED(y);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if((end_i = endmatch<',', 's'>(ctx.arg)) >= 0) {
		RETURN_IF_ALLOWED(s);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	end_i = ctx.arg.length();
	// hoping that by now, something would have stripped whitespace lol
	if(ctx.arg.length() == 0) {
		RETURN_IF_ALLOWED(imp);
		asar_throw_error(1, error_type_block, error_id_bad_addr_mode);
	}
	if(ctx.arg == "a" || ctx.arg == "A") {
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

int get_real_len(int min_len, int max_len, insn_context& ctx, const parse_result& parsed) {
	// we can theoretically give min_len to getlen now :o
	int arg_min_len = getlen(parsed.arg, parsed.kind == addr_kind::imm);
	int out_len;
	if(ctx.modifier != 0) {
		out_len = getlenfromchar(ctx.modifier);
		if(out_len < min_len || out_len > max_len)
			asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(min_len, max_len), out_len*8);
	} else {
		if(parsed.kind == addr_kind::imm) {
			if(!is_hex_constant(parsed.arg.data()))
				asar_throw_warning(2, warning_id_implicitly_sized_immediate);
			if(arg_min_len == 3 && max_len == 2) {
				// lda #label
				// todo: throw pedantic warning
				ctx.written_len = max_len;
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
	ctx.written_len = out_len;
	return out_len;
}

void handle_implicit_rep(string& arg, int opcode) {
	int64_t rep_count = getnum(arg);
	if(foundlabel) asar_throw_error(0, error_type_block, error_id_no_labels_here);
	for (int64_t i=0;i<rep_count;i++) { write1(opcode); }
}

void opcode0(insn_context& ctx, uint8_t opcode) {
	ctx.written_opcode = opcode;
	write1(opcode);
}
void opcode1(insn_context& ctx, uint8_t opcode, int num) {
	opcode0(ctx, opcode);
	write1(num);
}
void opcode2(insn_context& ctx, uint8_t opcode, int num) {
	opcode0(ctx, opcode);
	write2(num);
}
void opcode3(insn_context& ctx, uint8_t opcode, int num) {
	opcode0(ctx, opcode);
	write3(num);
}

template<int base, bool allow_imm = true>
void the8(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::xind, K::s, K::abs, K::lind, K::imm, K::indy, K::ind, K::sy, K::x, K::y, K::lindy>(ctx);
	addr_kind kind = parsed.kind;
	int min_len = 0, max_len = 4;
	if(kind == K::xind || kind == K::s || kind == K::sy || kind == K::indy || kind == K::ind || kind == K::lind || kind == K::lindy) min_len = max_len = 1;
	else if(kind == K::abs) min_len = 1, max_len = 3;
	else if(kind == K::x) min_len = 1, max_len = 3;
	else if(kind == K::y) min_len = max_len = 2;
	else if(kind == K::imm) min_len = 1, max_len = 2;
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	int real_len = get_real_len(min_len, max_len, ctx, parsed);
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
		opcode1(ctx, base + opcode_offset, the_num);
	} else if(real_len == 2) {
		if(kind == K::imm) opcode_offset = 0x9;
		if(kind == K::abs) opcode_offset = 0xd;
		if(kind == K::y) opcode_offset = 0x19;
		if(kind == K::x) opcode_offset = 0x1d;
		opcode2(ctx, base + opcode_offset, the_num);
	} else if(real_len == 3) {
		if(kind == K::abs) opcode_offset = 0xf;
		if(kind == K::x) opcode_offset = 0x1f;
		opcode3(ctx, base + opcode_offset, the_num);
	}
}

template<int base, int accum_opc, bool is_bit = false>
void thenext8(insn_context& ctx) {
	using K = addr_kind;
	parse_result parsed;
	if constexpr(is_bit) {
		parsed = parse_addr_kind<K::x, K::abs, K::imm>(ctx);
	} else {
		parsed = parse_addr_kind<K::x, K::abs, K::imm, K::a, K::imp>(ctx);
		// todo: some checks on ctx.modifier here
		if(parsed.kind == K::imm) return handle_implicit_rep(parsed.arg, accum_opc);
		if(parsed.kind == K::a || parsed.kind == K::imp) {
			opcode0(ctx, accum_opc);
			return;
		}
	}
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	int real_len = get_real_len(1, 2, ctx, parsed);
	int opcode = 0;
	if(parsed.kind == K::imm) {
		opcode = accum_opc;
	}
	if(real_len == 1) {
		if(parsed.kind == K::abs) opcode = base+0x6;
		if(parsed.kind == K::x) opcode = base+0x16;
		opcode1(ctx, opcode, the_num);
	} else if(real_len == 2) {
		if(parsed.kind == K::abs) opcode = base+0xe;
		if(parsed.kind == K::x) opcode = base+0x1e;
		opcode2(ctx, opcode, the_num);
	}
}

template<int base>
void tsb_trb(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::abs>(ctx);
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	int real_len = get_real_len(1, 2, ctx, parsed);
	if(real_len == 1) opcode1(ctx, base + 0x04, the_num);
	else opcode2(ctx, base + 0x0c, the_num);
}

template<int opc, int width = 1>
void branch(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::abs>(ctx);
	int64_t num = 0;
	if(pass == 2) {
		num = getnum(parsed.arg);
		bool target_is_abs = foundlabel;
		if(ctx.modifier != 0) {
			if(to_lower(ctx.modifier) == 'a') target_is_abs = true;
			else if(to_lower(ctx.modifier) == 'r') target_is_abs = false;
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
			num = delta;
		} else {
			if(num & ~(width==2 ? 0xffff : 0xff)) {
				asar_throw_error(2, error_type_block, error_id_relative_branch_out_of_bounds, dec(num).data());
			}
			num = (int16_t)(width==2 ? num : (int8_t)num);
		}
	}
	if(width == 1) opcode1(ctx, opc, num);
	else if(width == 2) opcode2(ctx, opc, num);
}

template<int opc>
void implied(insn_context& ctx) {
	if(ctx.arg != "") {
		// todo: some kind of "this instruction doesn't take an argument" message?
		asar_throw_error(0, error_type_block, error_id_bad_addr_mode);
	}
	opcode0(ctx, opc);
}

template<int opc>
void implied_rep(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::imm, K::imp>(ctx);
	if(parsed.kind == K::imp) {
		opcode0(ctx, opc);
	} else {
		handle_implicit_rep(parsed.arg, opc);
	}
}

template<int base, char op, char xy>
void xy_ops(insn_context& ctx) {
	using K = addr_kind;
	parse_result parsed;
	if(op == 'S') { // stx
		parsed = parse_addr_kind<K::abs, (xy == 'X' ? K::y : K::x)>(ctx);
	} else if(op == 'L') { // ldx
		parsed = parse_addr_kind<K::abs, K::imm, (xy == 'X' ? K::y : K::x)>(ctx);
	} else { // cpx
		parsed = parse_addr_kind<K::abs, K::imm>(ctx);
	}
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	int real_len = get_real_len(1, 2, ctx, parsed);
	int opcode = 0;
	if(parsed.kind == K::imm) {
		opcode = base;
	} else if(parsed.kind == K::abs) {
		if(real_len == 1) opcode = base + 0x4;
		else opcode = base + 0xc;
	} else { // ,x or ,y
		if(real_len == 1) opcode = base + 0x14;
		else opcode = base + 0x1c;
	}
	if(real_len == 1) opcode1(ctx, opcode, the_num);
	else if(real_len == 2) opcode2(ctx, opcode, the_num);
}

template<int opc>
void interrupt(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::imm, K::imp>(ctx);
	if(parsed.kind == K::imp) {
		opcode1(ctx, opc, 0);
	} else {
		int64_t num = pass == 2 ? getnum(parsed.arg) : 0;
		// this is kinda hacky
		if(num < 0 || num > 255) asar_throw_error(2, error_type_block, error_id_bad_access_width, format_valid_widths(1, 1), num > 65535 ? 24 : 16);
		opcode1(ctx, opc, num);
	}
}

template<int opc, addr_kind k, int width>
void oneoff(insn_context& ctx) {
	auto parsed = parse_addr_kind<k>(ctx);
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	// only for error checking, we know the answer anyways
	get_real_len(width, width, ctx, parsed);
	if(width == 1) opcode1(ctx, opc, the_num);
	else if(width == 2) opcode2(ctx, opc, the_num);
	else if(width == 3) opcode3(ctx, opc, the_num);
}

void stz(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = parse_addr_kind<K::abs, K::x>(ctx);
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	int real_len = get_real_len(1, 2, ctx, parsed);
	if(real_len == 1) {
		if(parsed.kind == K::abs) opcode1(ctx, 0x64, the_num);
		else if(parsed.kind == K::x) opcode1(ctx, 0x74, the_num);
	} else {
		if(parsed.kind == K::abs) opcode2(ctx, 0x9c, the_num);
		else if(parsed.kind == K::x) opcode2(ctx, 0x9e, the_num);
	}
}

template<char which>
void jmp_jsr_jml(insn_context& ctx) {
	using K = addr_kind;
	auto parsed = which == 'R' ? parse_addr_kind<K::abs, K::xind>(ctx)
		: which == 'P' ? parse_addr_kind<K::abs, K::xind, K::ind, K::lind>(ctx)
			: /* 'L' */ parse_addr_kind<K::abs, K::lind>(ctx);
	int64_t the_num = pass == 2 ? getnum(parsed.arg) : 0;
	// set optimizeforbank to -1 (i.e. auto, assume DBR = current bank)
	// because jmp and jsr's arguments are relative to the program bank anyways
	int old_optimize = optimizeforbank;
	if(parsed.kind == K::lind || parsed.kind == K::ind) {
		// these ones for Some Reason always read the pointer from bank 0 lol
		optimizeforbank = 0;
	} else {
		// the rest use bank K
		optimizeforbank = -1;
	}
	int the_width = 2;
	if(which == 'L' && parsed.kind == K::abs) the_width = 3;
	get_real_len(the_width, the_width, ctx, parsed);
	optimizeforbank = old_optimize;
	if(which == 'R') {
		if(parsed.kind == K::abs) opcode2(ctx, 0x20, the_num);
		else if(parsed.kind == K::xind) opcode2(ctx, 0xfc, the_num);
	} else if(which == 'L') {
		if(parsed.kind == K::abs) opcode3(ctx, 0x5c, the_num);
		else if(parsed.kind == K::lind) opcode2(ctx, 0xdc, the_num);
	} else {
		if(parsed.kind == K::abs) opcode2(ctx, 0x4c, the_num);
		else if(parsed.kind == K::ind) opcode2(ctx, 0x6c, the_num);
		else if(parsed.kind == K::xind) opcode2(ctx, 0x7c, the_num);
		else if(parsed.kind == K::lind) opcode2(ctx, 0xdc, the_num);
	}
}

template<int opc>
void mvn_mvp(insn_context& ctx) {
	int count;
	autoptr<char**> parts = qpsplit(ctx.arg.raw(), ',', &count);
	if(count != 2) asar_throw_error(2, error_type_block, error_id_bad_addr_mode);
	// todo length checks ???
	opcode0(ctx, opc);
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
	if(word[0][0] == '\'') return false;
	int word_i = 0;
	bool autoclean = false;
	if(!stricmpwithlower(word[0], "autoclean")) {
		word_i++;
		autoclean = true;
	}
	string opc = word[word_i++];
	string par;
	for(int i = word_i; i < numwords; i++){
		if(i > word_i) par += " ";
		par += word[i];
	}
	for(int i = 0; i < opc.length(); i++) opc.raw()[i] = to_lower(opc[i]);
	char mod = 0;
	if(opc.length() >= 2 && opc[opc.length()-2] == '.') {
		mod = opc[opc.length()-1];
		opc.truncate(opc.length()-2);
	}
	if(!mnemonics.exists(opc.data())) return false;
	insn_context ctx{par, {}, mod, 0};
	ctx.orig_insn[0] = opc[0];
	ctx.orig_insn[1] = opc[1];
	ctx.orig_insn[2] = opc[2];
	mnemonics.find(opc.data())(ctx);
	if(autoclean && pass > 0) {
		// should be changed to "can't use autoclean on this instruction"?
		if(ctx.written_len != 3) asar_throw_error(2, error_type_block, error_id_broken_autoclean);
		handle_autoclean(ctx.parsed_arg, ctx.written_opcode, snespos - 4);
	}
	return true;
}
