#include "asar.h"
#include "assembleblock.h"
#include "asar_math.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include "frozen/string.h"
#include "frozen/unordered_map.h"

// Capcom Cx4 (Hitachi HG51B169) - "Capcom Consumer Custom Chip"
// As there is no official public documentation for this chip,
// the behavioral checks (such as a write to an IR register doing nothing)
// are based on the bsnes source code (78474e5) and are subject
// to changes in case there are hardware verifications.
// These are commented with the phrase "pending hardware confirmation".

namespace {

	enum class general_purpose_registers : uint8_t
	{
		r0  = 0x60,  // both [value] and [value+0x10] are accepted
		r1  = 0x61,  // but practically speaking there is no difference
		r2  = 0x62,
		r3  = 0x63,
		r4  = 0x64,
		r5  = 0x65,
		r6  = 0x66,
		r7  = 0x67,
		r8  = 0x68,
		r9  = 0x69,
		r10 = 0x6A,
		r11 = 0x6B,
		r12 = 0x6C,
		r13 = 0x6D,
		r14 = 0x6E,
		r15 = 0x6F,
	};

	enum class special_purpose_registers : uint8_t
	{
		a      = 0x00, // 00 Accumulator             - 24 -    ; as an operand: reads 0, ignores writes
		mach   = 0x01, // 01 Multiply Result High    - 24 - RW
		macl   = 0x02, // 02 Multiply Result Low     - 24 - RW
		mdr    = 0x03, // 03 Memory Data Register    - 24 - RW ; Cx4 bus reads (ROM or SRAM), 8 bits at a time
		romb   = 0x08, // 08 Data ROM Buffer         - 24 - RW ; RDROM result, 24 bits at a time
		ramb   = 0x0C, // 0C RAM Buffer              - 24 - RW ; 
		mar    = 0x13, // 13 Memory Address Register - 24 - RW ; Cx4 bus address (ROM or SRAM)
		dpr    = 0x1C, // 1C RAM Address             - 24 - RW ; Cx4 RAM (SNES $6000-$6BFF), low 12 bits used
		ip     = 0x20, // 20 Instruction Pointer     -  8 - RW ; Program Counter
		p      = 0x28, // 28 Page Select             - 15 - RW
		busrom = 0x2E, // 2E Bus access, ROM wait    -  0 - RW ; reads 0. Starts a bus read into MDR, or a write of MDR
		busram = 0x2F, // 2F Bus access, RAM wait    -  0 - RW ; reads 0. Starts a bus read into MDR, or a write of MDR
		ir0    = 0x50, // 50 Immediate (#$000000)    - 24 - R
		ir1    = 0x51, // 51 Immediate (#$FFFFFF)    - 24 - R
		ir2    = 0x52, // 52 Immediate (#$00FF00)    - 24 - R
		ir3    = 0x53, // 53 Immediate (#$FF0000)    - 24 - R
		ir4    = 0x54, // 54 Immediate (#$00FFFF)    - 24 - R
		ir5    = 0x55, // 55 Immediate (#$FFFF00)    - 24 - R
		ir6    = 0x56, // 56 Immediate (#$800000)    - 24 - R
		ir7    = 0x57, // 57 Immediate (#$7FFFFF)    - 24 - R
		ir8    = 0x58, // 58 Immediate (#$008000)    - 24 - R
		ir9    = 0x59, // 59 Immediate (#$007FFF)    - 24 - R
		ir10   = 0x5A, // 5A Immediate (#$FF7FFF)    - 24 - R
		ir11   = 0x5B, // 5B Immediate (#$FFFF7F)    - 24 - R
		ir12   = 0x5C, // 5C Immediate (#$010000)    - 24 - R
		ir13   = 0x5D, // 5D Immediate (#$FEFFFF)    - 24 - R
		ir14   = 0x5E, // 5E Immediate (#$000100)    - 24 - R
		ir15   = 0x5F, // 5F Immediate (#$00FEFF)    - 24 - R
	};

	constexpr auto register_lookup = frozen::make_unordered_map<frozen::string, uint8_t>
		({
			{ "a",      (uint8_t)special_purpose_registers::a },
			{ "mach",   (uint8_t)special_purpose_registers::mach },
			{ "macl",   (uint8_t)special_purpose_registers::macl },
			{ "mdr",    (uint8_t)special_purpose_registers::mdr },
			{ "romb",   (uint8_t)special_purpose_registers::romb },
			{ "ramb",   (uint8_t)special_purpose_registers::ramb },
			{ "mar",    (uint8_t)special_purpose_registers::mar },
			{ "dpr",    (uint8_t)special_purpose_registers::dpr },
			{ "ip",     (uint8_t)special_purpose_registers::ip },
			{ "p",      (uint8_t)special_purpose_registers::p },
			{ "busrom", (uint8_t)special_purpose_registers::busrom },
			{ "busram", (uint8_t)special_purpose_registers::busram },
			{ "ir0",    (uint8_t)special_purpose_registers::ir0 },
			{ "ir1",    (uint8_t)special_purpose_registers::ir1 },
			{ "ir2",    (uint8_t)special_purpose_registers::ir2 },
			{ "ir3",    (uint8_t)special_purpose_registers::ir3 },
			{ "ir4",    (uint8_t)special_purpose_registers::ir4 },
			{ "ir5",    (uint8_t)special_purpose_registers::ir5 },
			{ "ir6",    (uint8_t)special_purpose_registers::ir6 },
			{ "ir7",    (uint8_t)special_purpose_registers::ir7 },
			{ "ir8",    (uint8_t)special_purpose_registers::ir8 },
			{ "ir9",    (uint8_t)special_purpose_registers::ir9 },
			{ "ir10",   (uint8_t)special_purpose_registers::ir10 },
			{ "ir11",   (uint8_t)special_purpose_registers::ir11 },
			{ "ir12",   (uint8_t)special_purpose_registers::ir12 },
			{ "ir13",   (uint8_t)special_purpose_registers::ir13 },
			{ "ir14",   (uint8_t)special_purpose_registers::ir14 },
			{ "ir15",   (uint8_t)special_purpose_registers::ir15 },
			{ "r0",     (uint8_t)general_purpose_registers::r0 },
			{ "r1",     (uint8_t)general_purpose_registers::r1 },
			{ "r2",     (uint8_t)general_purpose_registers::r2 },
			{ "r3",     (uint8_t)general_purpose_registers::r3 },
			{ "r4",     (uint8_t)general_purpose_registers::r4 },
			{ "r5",     (uint8_t)general_purpose_registers::r5 },
			{ "r6",     (uint8_t)general_purpose_registers::r6 },
			{ "r7",     (uint8_t)general_purpose_registers::r7 },
			{ "r8",     (uint8_t)general_purpose_registers::r8 },
			{ "r9",     (uint8_t)general_purpose_registers::r9 },
			{ "r10",    (uint8_t)general_purpose_registers::r10 },
			{ "r11",    (uint8_t)general_purpose_registers::r11 },
			{ "r12",    (uint8_t)general_purpose_registers::r12 },
			{ "r13",    (uint8_t)general_purpose_registers::r13 },
			{ "r14",    (uint8_t)general_purpose_registers::r14 },
			{ "r15",    (uint8_t)general_purpose_registers::r15 },
			});

	// Allow spelling IR0-IR15 as #$000000-#$00FEFF as well
	constexpr auto constant_lookup = frozen::make_unordered_map<frozen::string, uint8_t>
		({
			{ "$000000", (uint8_t)special_purpose_registers::ir0 },
			{ "$ffffff", (uint8_t)special_purpose_registers::ir1 },
			{ "$00ff00", (uint8_t)special_purpose_registers::ir2 },
			{ "$ff0000", (uint8_t)special_purpose_registers::ir3 },
			{ "$00ffff", (uint8_t)special_purpose_registers::ir4 },
			{ "$ffff00", (uint8_t)special_purpose_registers::ir5 },
			{ "$800000", (uint8_t)special_purpose_registers::ir6 },
			{ "$7fffff", (uint8_t)special_purpose_registers::ir7 },
			{ "$008000", (uint8_t)special_purpose_registers::ir8 },
			{ "$007fff", (uint8_t)special_purpose_registers::ir9 },
			{ "$ff7fff", (uint8_t)special_purpose_registers::ir10 },
			{ "$ffff7f", (uint8_t)special_purpose_registers::ir11 },
			{ "$010000", (uint8_t)special_purpose_registers::ir12 },
			{ "$feffff", (uint8_t)special_purpose_registers::ir13 },
			{ "$000100", (uint8_t)special_purpose_registers::ir14 },
			{ "$00feff", (uint8_t)special_purpose_registers::ir15 },
			});

	// LD is only allowed to load into A, MDR, MAR or P
	constexpr auto ld_dest_lookup = frozen::make_unordered_map<frozen::string, uint8_t>
		({
			{ "a",   0 },
			{ "mdr", 1 },
			{ "mar", 2 },
			{ "p",   3 },
			});

	enum class addr_kind
	{
		implied,       //                                e.g. NOP, RTS
		branch,        // param=far,   operand=target    e.g. BRA $12, BRA P:$12
		mar,           // <<memory access register>>     i.e. INC MAR
		shifted_a_reg, // param=shift, operand=register  e.g. AND A << 1, R0
		shifted_a_imm, // param=shift, operand=imm8      e.g. AND A << 1, #$12
		a_reg,         //              operand=register  e.g. SHR A, R0
		a_imm,         //              operand=imm8      e.g. SHR A, #$03 (limited to 0-31 for shifts)
		dest_reg,      // param=dest,  operand=register  e.g. LD MDR, R0
		dest_imm,      // param=dest,  operand=imm8      e.g. LD A, #$12
		page_imm,      // param=PL/PH, operand=imm8      e.g. LD PL, #$12
		ram_a,         // param=byte                     e.g. RDRAM RAMB+0, A
		ram_imm,       // param=byte,  operand=offset8   e.g. RDRAM RAMB+0, DPR+$12
		rom_a,         //                                e.g. RDROM A
		rom_abs10,     // param:operand = abs10          e.g. RDROM $123 (limits to the $400 24-bit values of the Cx4 data ROM)
		store,         // param=A/MDR, operand=register  e.g. ST R0, A
		swap,          //              operand=register  e.g. SWAP A, R3
		num_addr_kinds,
	};

	enum mnemonic_flag
	{
		flag_far         = 1 << 0,
		flag_reversed    = 1 << 1,
		flag_shift_count = 1 << 2,
	};

	// All Cx4 instructions are 16-bit, no exceptions.
	struct mnem_kind_info {
		uint8_t opcode  = 0; // bits 15-10 as the high byte, low 2 bits clear
		uint8_t param   = 0; // fixed bits 9-8, used by kinds that don't set param
		uint8_t operand = 0; // fixed bits 7-0, used by kinds that don't set operand
		uint32_t flags  = 0; // combination of `mnemonic_flag`s
	};

	struct mnemonicinfo {
		mnem_kind_info types[(int)addr_kind::num_addr_kinds];
		uint32_t allowed_kinds_mask = 0;
		struct opcode {
			uint8_t byte;
			addr_kind akind;
			uint32_t flags;
			uint8_t param;
			uint8_t operand;
			constexpr opcode(uint8_t byte_, addr_kind akind_, uint32_t flags_ = 0, uint8_t param_ = 0, uint8_t operand_ = 0)
				: byte(byte_), akind(akind_), flags(flags_), param(param_), operand(operand_) {}
		};
		constexpr mnemonicinfo(const std::initializer_list<opcode>& opcodes) {
			for (auto& x : opcodes) {
				// each kind at most once per mnemonic
				assert((allowed_kinds_mask & (1 << (int)x.akind)) == 0);
				assert((x.byte & 0x03) == 0 && x.param <= 3);
				allowed_kinds_mask |= 1 << (int)x.akind;
				auto& this_part = types[(int)x.akind];
				this_part.opcode = x.byte;
				this_part.param = x.param;
				this_part.operand = x.operand;
				this_part.flags = x.flags;
			}
		}
	};

	// { opcode, kind::, flag_, fixed param, fixed operand }
	constexpr auto mnemonic_lookup = frozen::make_unordered_map<frozen::string, mnemonicinfo>({
		{ "nop",    { { 0x00, addr_kind::implied                      } } },
		// 0x04 is currently unknown/unused
		{ "bra",    { { 0x08, addr_kind::branch, flag_far             } } },
		{ "beq",    { { 0x0C, addr_kind::branch, flag_far             } } },
		{ "bcs",    { { 0x10, addr_kind::branch, flag_far             } } },
		{ "bmi",    { { 0x14, addr_kind::branch, flag_far             } } },
		{ "bvs",    { { 0x18, addr_kind::branch, flag_far             } } },
		{ "wait",   { { 0x1C, addr_kind::implied                      } } },
		// 0x20 is currently unknown/unused
		{ "skipvc", { { 0x24, addr_kind::implied, 0, 0, 0             } } },
		{ "skipvs", { { 0x24, addr_kind::implied, 0, 0, 1             } } },
		{ "skipcc", { { 0x24, addr_kind::implied, 0, 1, 0             } } },
		{ "skipcs", { { 0x24, addr_kind::implied, 0, 1, 1             } } },
		{ "skipzc", { { 0x24, addr_kind::implied, 0, 2, 0             } } },
		{ "skipzs", { { 0x24, addr_kind::implied, 0, 2, 1             } } },
		{ "skipnc", { { 0x24, addr_kind::implied, 0, 3, 0             } } },
		{ "skipns", { { 0x24, addr_kind::implied, 0, 3, 1             } } },
		{ "jsr",    { { 0x28, addr_kind::branch, flag_far             } } },
		{ "jeq",    { { 0x2C, addr_kind::branch, flag_far             } } },
		{ "jcs",    { { 0x30, addr_kind::branch, flag_far             } } },
		{ "jmi",    { { 0x34, addr_kind::branch, flag_far             } } },
		{ "jvs",    { { 0x38, addr_kind::branch, flag_far             } } },
		{ "rts",    { { 0x3C, addr_kind::implied                      } } },
		{ "inc",    { { 0x40, addr_kind::mar                          } } },
		// 0x44 is currently unknown/unused
		{ "cmpr",   { { 0x48, addr_kind::shifted_a_reg, flag_reversed },
					  { 0x4C, addr_kind::shifted_a_imm, flag_reversed } } },
		{ "cmp",    { { 0x50, addr_kind::shifted_a_reg                },
					  { 0x54, addr_kind::shifted_a_imm                } } },
        { "sxb",    { { 0x58, addr_kind::implied, 0, 1                } } },
        { "sxw",    { { 0x58, addr_kind::implied, 0, 2                } } },
		// 0x5C is currently unknown/unused
        { "ld",     { { 0x60, addr_kind::dest_reg                     },
                      { 0x64, addr_kind::dest_imm                     },
                      { 0x7C, addr_kind::page_imm                     } } },
        { "rdram",  { { 0x68, addr_kind::ram_a                        },
					  { 0x6C, addr_kind::ram_imm                      } } },
        { "rdrom",  { { 0x70, addr_kind::rom_a                        },
					  { 0x74, addr_kind::rom_abs10                    } } },
		// 0x78 is currently unknown/unused
        { "add",    { { 0x80, addr_kind::shifted_a_reg                },
					  { 0x84, addr_kind::shifted_a_imm                } } },
        { "subr",   { { 0x88, addr_kind::shifted_a_reg, flag_reversed },
					  { 0x8C, addr_kind::shifted_a_imm, flag_reversed } } },
        { "sub",    { { 0x90, addr_kind::shifted_a_reg                },
					  { 0x94, addr_kind::shifted_a_imm                } } },
        { "smul",   { { 0x98, addr_kind::a_reg                        },
					  { 0x9C, addr_kind::a_imm                        } } },
        { "xnor",   { { 0xA0, addr_kind::shifted_a_reg                },
					  { 0xA4, addr_kind::shifted_a_imm                } } },
        { "xor",    { { 0xA8, addr_kind::shifted_a_reg                },
					  { 0xAC, addr_kind::shifted_a_imm                } } },
        { "and",    { { 0xB0, addr_kind::shifted_a_reg                },
				      { 0xB4, addr_kind::shifted_a_imm                } } },
        { "or",     { { 0xB8, addr_kind::shifted_a_reg                },
				      { 0xBC, addr_kind::shifted_a_imm                } } },
        { "shr",    { { 0xC0, addr_kind::a_reg, flag_shift_count      },
				      { 0xC4, addr_kind::a_imm, flag_shift_count      } } },
        { "asr",    { { 0xC8, addr_kind::a_reg, flag_shift_count      },
					  { 0xCC, addr_kind::a_imm, flag_shift_count      } } },
        { "ror",    { { 0xD0, addr_kind::a_reg, flag_shift_count      },
					  { 0xD4, addr_kind::a_imm, flag_shift_count      } } },
        { "shl",    { { 0xD8, addr_kind::a_reg, flag_shift_count      },
					  { 0xDC, addr_kind::a_imm, flag_shift_count      } } },
        { "st",     { { 0xE0, addr_kind::store                        } } },
		// 0xE4 is currently unknown/unused
        { "wrram",  { { 0xE8, addr_kind::ram_a, flag_reversed         },
					  { 0xEC, addr_kind::ram_imm, flag_reversed       } } },
        { "swap",   { { 0xF0, addr_kind::swap                         } } },
		// 0xF4 is currently unknown/unused
        { "clear",  { { 0xF8, addr_kind::implied                      } } },
        { "stop",   { { 0xFC, addr_kind::implied                      } } },
		});

	struct parse_result {
		addr_kind kind;
		uint8_t param;
		uint8_t operand;
	};

	// ---------------------------------------------------------------------------
	// Helper functions for the parsers
	// ---------------------------------------------------------------------------

	// checks for matching characters at the start of haystack, ignoring spaces.
	// returns index into haystack right after the match
	template<char... chars>
	int64_t startmatch(const string& haystack) {
		static const char needle[] = { chars... };
		size_t haystack_i = 0;
		for (size_t i = 0; i < sizeof...(chars); i++) {
			while (haystack[haystack_i] == ' ')
			{
				haystack_i++;
			}

			if (needle[i] != to_lower(haystack[haystack_i++]))
			{
				return -1;
			}
		}
		return haystack_i;
	}

	// Look up a register by name, case-insensitive, returns its enum
	int find_register(const string& name) {
		string lowered = name;
		lower(lowered);
		auto it = register_lookup.find(frozen::string(lowered.data(), lowered.length()));
		if (it == register_lookup.end())
		{
			return -1;
		}

		return it->second;
	}

	// Look up a register by name, or a constant (IRn) by value, returns its enum
	int find_source_register(const string& text) {
		int reg = find_register(text);
		int64_t after_hash = startmatch<'#'>(text);
		if (reg >= 0 || after_hash < 0)
		{
			return reg;
		}

		string value(text.data() + after_hash);
		strip_whitespace(value);
		lower(value);
		auto it = constant_lookup.find(frozen::string(value.data(), value.length()));
		if (it == constant_lookup.end())
		{
			return -1;
		}
		return it->second;
	}

	// Look up the LD destination by name, returns its enum
	int find_ld_dest(const string& name) {
		string lowered = name;
		strip_whitespace(lowered);
		lower(lowered);
		auto it = ld_dest_lookup.find(frozen::string(lowered.data(), lowered.length()));
		if (it == ld_dest_lookup.end())
		{
			return -1;
		}

		return it->second;
	}

	// Some opcodes allow shifting A within the operation.
	// Return the shift count
	uint8_t shift_to_param(int64_t shift) {
		switch (shift)
		{
		case 0: return 0;
		case 1: return 1;
		case 8: return 2;
		case 16: return 3;
		}
		throw_err_block(1, err_bad_addr_mode);
	}

	// Check if a given number fits within a specified bit width
	void check_range(int64_t value, int bits) {
		if (value >= 0 && value < ((int64_t)1 << bits))
		{
			return;
		}

		int value_bits = 0;
		for (uint64_t v = (uint64_t)value; v; v >>= 1)
		{
			value_bits++;
		}

		string allowed = STR "only " + dec(bits).data() + "-bit";
		throw_err_block(2, err_bad_access_width, allowed.data(), value_bits);
	}

	// The Cx4 uses a lot of "<instruction> X, Y" operands. Split from
	// that comma, while ignoring anything within parentheses or quotes
	bool split_operands(const char* par, string& first, string& second) {
		const char* comma = strqpchr(par, ',');
		if (!comma || strqpchr(comma + 1, ','))
		{
			return false;
		}

		first = string(par, (int)(comma - par));
		second = string(comma + 1);
		strip_whitespace(first);
		strip_whitespace(second);
		return first.length() > 0 && second.length() > 0;
	}

	// Check if a case-insensitive param1 equals to a lowercase param2
	bool is_word(const string& text, const char* lower_word) {
		return !stricmpwithlower(text.data(), lower_word);
	}

	// Match "KEYWORD+expr" case insensitively
	// It is used for RAMB+ and DPR+ for the RDRAM/WRRAM opcodes
	template<char... chars>
	bool match_keyword_plus(const string& text, string& rest) {
		int64_t after = startmatch<chars..., '+'>(text);
		if (after < 0)
		{
			return false;
		}
		rest = string(text.data() + after);
		strip_whitespace(rest);
		return rest.length() > 0;
	}

	// Some opcodes allow shifting A within the operation.
	// Check whether the shifted variable truly is A, and by how much
	bool parse_shifted_a(const string& text, uint8_t& param) {
		int64_t after_a = startmatch<'a'>(text);
		if (after_a < 0)
		{
			return false;
		}

		string rest(text.data() + after_a);
		strip_whitespace(rest);

		// There is nothing after "A"; there is no shift
		if (rest.length() == 0)
		{
			param = 0;
			return true;
		}

		// There is something after "A"; It has to be "<<"
		int64_t after_shift = startmatch<'<', '<'>(rest);
		if (after_shift < 0)
		{
			return false;
		}

		// There is a "<<" after A. The shift count could be a math expression
		int64_t shift = parse_math_expr(rest.data() + after_shift)->evaluate_static().get_integer();

		// Parse the shift count
		param = shift_to_param(shift);
		return true;
	}

	// Check if the text is an immediate value
	bool is_imm(const string& text) {
		return startmatch<'#'>(text) >= 0;
	}

	// Check the immediate value, including if it fits in the specified bit width.
	// It is possible that the value is a label, which is available in pass 2
	int64_t imm_value(const string& text, int bits) {
		int64_t value = pass == 2 ? getnum(text.data() + startmatch<'#'>(text)) : 0;
		check_range(value, bits);
		return value;
	}

	// RDRAM/WRRAM's "RAMB" only accept the values 0, 1 or 2 as the parameter
	uint8_t ram_byte(const string& expr) {
		int64_t byte = parse_math_expr(expr)->evaluate_static().get_integer();
		if (byte < 0 || byte > 2)
		{
			throw_err_block(1, err_cx4_invalid_ram_byte, dec(byte).data());
		}
		return (uint8_t)byte;
	}

	// Register A ($00) reads as the value 0 when it's the register operand.
	// This is an unintuitive side effect and the user should be warned about it.
	void warn_if_a_operand(int reg) {
		if (reg == (int)special_purpose_registers::a)
		{
			throw_warning(2, warn_cx4_a_operand_reads_zero);
		}
	}

	// BUSROM ($2E) or BUSRAM ($2F) should really only be read with
	// an LD MDR, BUSROM or LD MDR, BUSRAM.
	// In all other cases, it reads as value 0 and starts a bus read
	void warn_if_bus_read(int reg) {
		if (reg == (int)special_purpose_registers::busrom)
		{
			throw_warning(2, warn_cx4_bus_read_reads_zero, "BUSROM", "BUSROM");
		}
		else if (reg == (int)special_purpose_registers::busram)
		{
			throw_warning(2, warn_cx4_bus_read_reads_zero, "BUSRAM", "BUSRAM");
		}
	}

	// Return the value of a constant register (IR0-IR15) by its hex code,
	// from its entry in constant_lookup.
	uint32_t constant_value(int code) {
		for (auto& entry : constant_lookup)
		{
			if (entry.second == code)
			{
				string digits(entry.first.data() + 1, (int)entry.first.size() - 1);
				return (uint32_t)strtoul(digits.data(), nullptr, 16);
			}
		}
		return 0;
	}

	// A shift by register only uses the low 5 bits of its value, and a count
	// above 24 shifts by 0. This is pending hardware confirmation.
	// IR0-IR15 have fixed values, and the logic makes them all shift by 0.
	// This is also pending hardware confirmation.
	void warn_if_constant_shift_count(int reg) {
		if (reg < (int)special_purpose_registers::ir0 || reg > (int)special_purpose_registers::ir15)
		{
			return;
		}

		int ir = reg - (int)special_purpose_registers::ir0;
		uint32_t value = constant_value(reg);
		uint32_t count = value & 0x1F;
		if (count > 24)
		{
			count = 0;
		}
		if (count == 0)
		{
			string name = STR "IR" + dec(ir).data();
			throw_warning(2, warn_cx4_constant_shift_count, name.data(), hex(value, 6).data());
		}
	}

	// `text` in upper case.
	string uppercase(string text) {
		for (int i = 0; i < text.length(); i++)
		{
			text.raw()[i] = (char)to_upper(text.data()[i]);
		}
		return text;
	}

	// Return the upper-case name of register by its hex code.
	string register_name(int code) {
		for (auto& entry : register_lookup)
		{
			if (entry.second == code)
			{
				return uppercase(string(entry.first.data(), (int)entry.first.size()));
			}
		}
		return "";
	}

	// Registers have an 8-bit code, but some instructions only use the 
	// low 4 bits of that code, as a general-purpose register (GPR) number. 
	// Warn if the register is not a GPR, and show the GPR number it acts as.
	void warn_if_not_gpr(int reg, const char* mnemonic) {
		if (reg >= (int)general_purpose_registers::r0 && reg <= (int)general_purpose_registers::r15)
		{
			return;
		}

		string gpr = dec(reg & 0x0F);
		throw_warning(2, warn_cx4_register_low_bits, mnemonic, register_name(reg).data(),
			hex((unsigned int)reg, 2).data(), gpr.data(), gpr.data());
	}

	// ---------------------------------------------------------------------------
	// The parsers
	// ---------------------------------------------------------------------------

	// Implied opcodes: NOP, WAIT, RTS, CLEAR, STOP, SKIPxx, SXB, SXW
	bool parse_implied(const char* par, const mnem_kind_info& info, parse_result& out) {
		while (*par == ' ')
		{
			par++;
		}
		return *par == '\0';
	}

	// Branches and jumps: BRA BEQ BCS BMI BVS JSR JEQ JCS JMI JVS. All make use of flag_far
	// operand: target word index within a 256-word (512-byte) page. Absolute, not relative.
	// param:   2 = far (bit 9): page comes from the P register e.g. BRA P:$12
	//          0 = near: current page                          e.g. BRA $12
	bool parse_branch(const char* par, const mnem_kind_info& info, parse_result& out) {
		string target = par;
		strip_whitespace(target);

		// Far flag check
		int64_t after_far = startmatch<'p', ':'>(target);
		bool far = after_far >= 0;
		if (far)
		{
			if (!(info.flags & flag_far))
			{
				return false;
			}
			out.param = 2;
			target = string(target.data() + after_far);
			strip_whitespace(target);
		}

		if (target.length() == 0)
		{
			return false;
		}

		// Labels are only final on pass 2.
		if (pass != 2)
		{
			return true;
		}

		auto expr = parse_math_expr(target);
		int64_t num = expr->evaluate().get_integer();
		if (expr->has_label() == 0)
		{
			check_range(num, 8);
			out.operand = (uint8_t)num;
			return true;
		}

		if (num & 1)
		{
			throw_err_block(2, err_cx4_branch_misaligned, hex((unsigned int)num, 6).data());
		}
		if (!far && (num & ~0x1FF) != ((snespos & 0xFFFFFF) & ~0x1FF))
		{
			throw_err_block(2, err_cx4_branch_other_page, hex((unsigned int)num, 6).data());
		}
		out.operand = (uint8_t)(num >> 1);
		return true;
	}

	// Simply parse "MAR" for INC MAR
	bool parse_mar(const char* par, const mnem_kind_info& info, parse_result& out) {
		string operand = par;
		strip_whitespace(operand);
		return is_word(operand, "mar");
	}

	// Opcodes that can shift A: CMP ADD SUB XNOR XOR AND OR, with a register operand
	// Reversed versions also: CMPR SUBR, uses flag_reversed to switch dest, src
	bool parse_shifted_a_reg(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		// Check if it's a reversed instruction
		bool reversed = info.flags & flag_reversed;
		const string& a_text = reversed ? second : first;
		const string& reg_text = reversed ? first : second;
		int reg = find_source_register(reg_text);
		if (reg < 0 || !parse_shifted_a(a_text, out.param))
		{
			return false;
		}
		warn_if_a_operand(reg);
		warn_if_bus_read(reg);
		out.operand = (uint8_t)reg;
		return true;
	}

	// Opcodes that can shift A: CMP ADD SUB XNOR XOR AND OR, with a register operand
	// Reversed versions also: CMPR SUBR, uses flag_reversed to switch dest, src
	bool parse_shifted_a_imm(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		// Check if it's a reversed instruction
		bool reversed = info.flags & flag_reversed;
		const string& a_text = reversed ? second : first;
		const string& imm_text = reversed ? first : second;
		if (!is_imm(imm_text) || !parse_shifted_a(a_text, out.param))
		{
			return false;
		}
		out.operand = (uint8_t)imm_value(imm_text, 8);
		return true;
	}

	// Shifts: SMUL SHR ASR ROR SHL, with a register operand
	bool parse_a_reg(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second) || !is_word(first, "a"))
		{
			return false;
		}

		int reg = find_source_register(second);
		if (reg < 0)
		{
			return false;
		}
		warn_if_a_operand(reg);
		warn_if_bus_read(reg);
		if (info.flags & flag_shift_count)
		{
			warn_if_constant_shift_count(reg);
		}
		out.operand = (uint8_t)reg;
		return true;
	}

	// Shifts: SMUL SHR ASR ROR SHL, with an immediate operand
	bool parse_a_imm(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second) || !is_word(first, "a") || !is_imm(second))
		{
			return false;
		}

		if (!(info.flags & flag_shift_count))
		{
			out.operand = (uint8_t)imm_value(second, 8);
			return true;
		}

		// The shift count is 5 bits, and a count above 24 shifts by 0
		int64_t count = pass == 2 ? getnum(second.data() + startmatch<'#'>(second)) : 0;
		if (count < 0 || count > 31)
		{
			throw_err_block(2, err_cx4_invalid_shift_count, dec(count).data());
		}
		if (count > 24)
		{
			throw_warning(2, warn_cx4_shift_count_above_24, dec(count).data());
		}
		out.operand = (uint8_t)count;
		return true;
	}

	// LD instruction that can load into A, MDR, MAR or P.
	// The source is a GPR.
	bool parse_dest_reg(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		int dest = find_ld_dest(first);
		if (dest < 0)
		{
			return false;
		}
		int reg = dest == 3 ? find_register(second) : find_source_register(second);
		if (reg < 0)
		{
			return false;
		}
		if (dest == 3)
		{
			warn_if_not_gpr(reg, "LD P");
		}
		else
		{
			warn_if_a_operand(reg);
			if (dest != 1)
			{
				warn_if_bus_read(reg);
			}
		}
		out.param = (uint8_t)dest;
		out.operand = (uint8_t)reg;
		return true;
	}

	// LD instruction that can load into A, MDR, MAR or P.
	// The source is an immediate value.
	bool parse_dest_imm(const char* par, const mnem_kind_info& info, parse_result& out) {
		// Destination: everything before the first comma. Anything that isn't in
		// ld_dest_lookup (PL/PH, R0, ...) is left for the other LD kinds.
		const char* comma = strchr(par, ',');
		if (!comma)
		{
			return false;
		}

		int dest = find_ld_dest(string(par, (int)(comma - par)));
		if (dest < 0)
		{
			return false;
		}
		out.param = (uint8_t)dest;

		// The immediate: '#' followed by any math expression
		const char* value_text = comma + 1;
		while (*value_text == ' ')
		{
			value_text++;
		}
		if (*value_text != '#')
		{
			return false;
		}
		value_text++;

		int64_t value = pass == 2 ? getnum(value_text) : 0;
		check_range(value, 8);
		out.operand = (uint8_t)value;
		return true;
	}

	// LD instruction that can load into PL or PH
	// The source is an immediate value indicating the low or high byte
	bool parse_page_imm(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second) || !is_imm(second))
		{
			return false;
		}

		if (is_word(first, "pl"))
		{
			out.param = 0;
		}
		else if (is_word(first, "ph"))
		{
			out.param = 1;
		}
		else
		{
			return false;
		}
		out.operand = (uint8_t)imm_value(second, out.param == 1 ? 7 : 8);
		return true;
	}

	// RDRAM, and WRRAM, A-based
	// Syntax: RDRAM RAMB+0, A   WRRAM A, RAMB+0
	bool parse_ram_a(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		bool reversed = info.flags & flag_reversed;
		const string& byte_text = reversed ? second : first;
		const string& a_text = reversed ? first : second;
		string byte_expr;
		if (!is_word(a_text, "a") || !match_keyword_plus<'r', 'a', 'm', 'b'>(byte_text, byte_expr) || is_imm(byte_expr))
		{
			return false;
		}
		out.param = ram_byte(byte_expr);
		return true;
	}

	// RDRAM, and WRRAM, DPR+index-based
	// Syntax: RDRAM RAMB+0, DPR+$12    WRRAM DPR+$12, RAMB+0
	bool parse_ram_imm(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		bool reversed = info.flags & flag_reversed;
		const string& byte_text = reversed ? second : first;
		const string& dpr_text = reversed ? first : second;
		string byte_expr, offset_expr;
		if (!match_keyword_plus<'d', 'p', 'r'>(dpr_text, offset_expr) || is_imm(offset_expr)
			|| !match_keyword_plus<'r', 'a', 'm', 'b'>(byte_text, byte_expr) || is_imm(byte_expr))
		{
			return false;
		}
		out.param = ram_byte(byte_expr);

		// Labels are only final on pass 2, like in imm_value().
		int64_t offset = pass == 2 ? getnum(offset_expr) : 0;
		check_range(offset, 8);
		out.operand = (uint8_t)offset;
		return true;
	}

	// RDROM. It always has A as an operand
	bool parse_rom_a(const char* par, const mnem_kind_info& info, parse_result& out) {
		string operand = par;
		strip_whitespace(operand);
		return is_word(operand, "a");
	}

	// RDROM. It takes a 10-bit data ROM address ($000-$3FF)
	// e.g. RDROM $123
	bool parse_rom_abs(const char* par, const mnem_kind_info& info, parse_result& out) {
		string operand = par;
		strip_whitespace(operand);
		if (operand.length() == 0 || is_imm(operand))
		{
			return false;
		}

		if (pass != 2)
		{
			return true;
		}

		int64_t address = getnum(operand);
		if (address < 0 || address > 0x3FF)
		{
			string shown = address < 0
				? STR "-$" + hex((unsigned int)-address, 3).data()
				: STR "$" + hex((unsigned int)address, 3).data();
			throw_err_block(2, err_cx4_rdrom_out_of_range, shown.data());
		}
		out.param = (uint8_t)(address >> 8);
		out.operand = (uint8_t)address;
		return true;
	}

	// ST instruction that can write to any register. The source is A or MDR.
	// Writes to A and IR0-IR15 are ignored, pending hardware confirmation.
	// Writing A to BUSROM and BUSRAM ignores the value in A; It writes MDR to the address
	// in MAR, pending hardware confirmation.
	bool parse_store(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return false;
		}

		// A and MDR have the same codes as the LD destinations.
		int reg = find_register(first);
		int source = find_ld_dest(second);
		if (reg < 0 || (source != 0 && source != 1))
		{
			return false;
		}

		// Stores into A are ignored
		if (reg == (int)special_purpose_registers::a)
		{
			throw_warning(2, warn_cx4_write_ignored, "A");
		}

		// Stores into IR0-IR15 are ignored
		else if (reg >= (int)special_purpose_registers::ir0 && reg <= (int)special_purpose_registers::ir15)
		{
			string name = STR "IR" + dec(reg - (int)special_purpose_registers::ir0).data();
			throw_warning(2, warn_cx4_write_ignored, name.data());
		}

		// Stores into BUSROM/BUSRAM is unconventional
		else if (source == 0 && (reg == (int)special_purpose_registers::busrom || reg == (int)special_purpose_registers::busram))
		{
			const char* name = reg == (int)special_purpose_registers::busrom ? "BUSROM" : "BUSRAM";
			throw_warning(2, warn_cx4_bus_write_ignores_value, name, name);
		}
		else if (reg == (int)special_purpose_registers::mach)
		{
			// Stores into MACH discards the upper two bytes.
			// This might be a bsnes bug rather than how the Cx4 works,
			// pending hardware confirmation.
			throw_warning(2, warn_cx4_mach_write_truncated);
		}
		out.param = (uint8_t)source;
		out.operand = (uint8_t)reg;
		return true;
	}

	// SWAP. It only accepts GPR as the source, and swaps it with A.
	// e.g. SWAP A, R3
	bool parse_swap(const char* par, const mnem_kind_info& info, parse_result& out) {
		string first, second;
		if (!split_operands(par, first, second) || !is_word(first, "a"))
		{
			return false;
		}

		int reg = find_register(second);
		if (reg < 0)
		{
			return false;
		}
		warn_if_not_gpr(reg, "SWAP");
		out.operand = (uint8_t)reg;
		return true;
	}

	bool parse_kind(addr_kind kind, const char* par, const mnem_kind_info& info, parse_result& out) {
		switch (kind) {
		case addr_kind::implied:       return parse_implied(par, info, out);
		case addr_kind::branch:        return parse_branch(par, info, out);
		case addr_kind::mar:           return parse_mar(par, info, out);
		case addr_kind::shifted_a_reg: return parse_shifted_a_reg(par, info, out);
		case addr_kind::shifted_a_imm: return parse_shifted_a_imm(par, info, out);
		case addr_kind::a_reg:         return parse_a_reg(par, info, out);
		case addr_kind::a_imm:         return parse_a_imm(par, info, out);
		case addr_kind::dest_reg:      return parse_dest_reg(par, info, out);
		case addr_kind::dest_imm:      return parse_dest_imm(par, info, out);
		case addr_kind::page_imm:      return parse_page_imm(par, info, out);
		case addr_kind::ram_a:         return parse_ram_a(par, info, out);
		case addr_kind::ram_imm:       return parse_ram_imm(par, info, out);
		case addr_kind::rom_a:         return parse_rom_a(par, info, out);
		case addr_kind::rom_abs10:     return parse_rom_abs(par, info, out);
		case addr_kind::store:         return parse_store(par, info, out);
		case addr_kind::swap:          return parse_swap(par, info, out);
		case addr_kind::num_addr_kinds: break;
		}
		return false;
	}

	// Moving past the last word of a page switches to page P,
	//  or stops on the second cache page.
	// Only STOP doesSo at the last word, only STOP does what it says.
	// Pending hardware confirmation.
	void warn_if_last_word_of_page(const string& mnem, const mnem_kind_info& info, const parse_result& res) {
		if ((snespos & 0x1FF) != 0x1FE || info.opcode == 0xFC) // 0xFC: STOP
		{
			return;
		}
		if (res.kind != addr_kind::branch)
		{
			throw_warning(2, warn_cx4_last_word_of_page);
			return;
		}

		bool far = res.param & 2;
		bool call = info.opcode >= 0x28;                               // JSR JEQ JCS JMI JVS
		bool conditional = info.opcode != 0x08 && info.opcode != 0x28; // not BRA or JSR
		string effects = far
			? " Its far target is still reached, unless the Cx4 stops."
			: " Its target is in page P, not this page.";
		if (conditional)
		{
			effects += " If it isn't taken, execution continues at word 0 of page P.";
		}
		if (call)
		{
			effects += " The call returns to word 0 of page P.";
		}
		throw_warning(2, warn_cx4_branch_last_word_of_page, uppercase(mnem).data(), effects.data());
	}

	// Skipping over the last word of a page switches to page P or stops on the
	// second cache page, pending hardware confirmation.
	void warn_if_skip_second_to_last_word_of_page(const string& mnem, const mnem_kind_info& info) {
		if ((snespos & 0x1FF) == 0x1FC && info.opcode == 0x24) // 0x24: SKIPxx
		{
			throw_warning(2, warn_cx4_skip_second_to_last_word_of_page, uppercase(mnem).data());
		}
	}

	// Called when no kind matched, to give a clearer error than
	// err_bad_addr_mode for some likely mistakes.
	void throw_if_known_mistake(const string& mnem, const char* par) {
		string first, second;
		if (!split_operands(par, first, second))
		{
			return;
		}

		string rest;
		if (is_word(mnem, "rdram") || is_word(mnem, "wrram"))
		{
			bool reversed = is_word(mnem, "wrram");
			const string& byte_text = reversed ? second : first;
			const string& addr_text = reversed ? first : second;
			if (is_word(byte_text, "ramb"))
			{
				throw_err_block(1, err_cx4_ramb_without_byte);
			}
			if (is_word(addr_text, "dpr"))
			{
				throw_err_block(1, err_cx4_dpr_without_offset);
			}
			if (startmatch<'d', 'p', 'r', '-'>(addr_text) >= 0)
			{
				throw_err_block(1, err_cx4_dpr_negative_offset);
			}
		}
		else if (is_word(mnem, "ld") && match_keyword_plus<'r', 'a', 'm', 'b'>(second, rest))
		{
			throw_err_block(1, err_cx4_ld_ramb_byte);
		}
	}

	void emit(const mnem_kind_info& info, const parse_result& res) {
		write1(res.operand);
		write1(info.opcode | (res.param & 0x03));
	}

} // namespace

bool asblock_cx4(const string& firstword, const char* par)
{
	string mnem = firstword;
	lower(mnem);

	frozen::string mnem2(mnem.data(), mnem.length());
	auto it = mnemonic_lookup.find(mnem2);
	if (it == mnemonic_lookup.end()) return false;
	const mnemonicinfo& mnem_info = it->second;

	// try each kind this mnemonic allows, in addr_kind order
	for (int k = 0; k < (int)addr_kind::num_addr_kinds; k++) {
		if (!(mnem_info.allowed_kinds_mask & (1u << k))) continue;
		const mnem_kind_info& kind_info = mnem_info.types[k];
		parse_result res{ (addr_kind)k, kind_info.param, kind_info.operand };
		if (parse_kind(res.kind, par, kind_info, res)) {
			warn_if_last_word_of_page(mnem, kind_info, res);
			warn_if_skip_second_to_last_word_of_page(mnem, kind_info);
			emit(kind_info, res);
			return true;
		}
	}
	throw_if_known_mistake(mnem, par);
	throw_err_block(1, err_bad_addr_mode);
}
