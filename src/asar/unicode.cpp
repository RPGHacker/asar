#include "unicode.h"

size_t utf8_val(int* codepoint, const char* inp) {
	unsigned char c = *inp++;
	int val;
	if (c < 0x80) {
		// plain ascii
		*codepoint = c;
		return 1u;
	}
	// RPG Hacker: Byte sequences starting with 0xC0 or 0xC1 are invalid.
	// So are byte sequences starting with anything >= 0xF5.
	// And anything below 0xC0 indicates a follow-up byte and should never be at the start of a sequence.
	else if (c > 0xC1 && c < 0xF5) {
		// 1, 2 or 3 continuation bytes
		int cont_byte_count = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : 1;
		// bit hack to extract the significant bits from the start byte
		val = (c & ((1 << (6 - cont_byte_count)) - 1));
		for (int i = 0; i < cont_byte_count; i++) {
			unsigned char next = *inp++;
			if ((next & 0xC0) != 0x80) {
				*codepoint = -1;
				return 0u;
			}
			val = (val << 6) | (next & 0x3F);
		}		
		if (// too many cont.bytes
			(*inp & 0xC0) == 0x80 ||
			
			// invalid codepoints
			val > 0x10FFFF ||

			// check overlong encodings
			(cont_byte_count == 3 && val < 0x1000) ||
			(cont_byte_count == 2 && val < 0x800) ||
			(cont_byte_count == 1 && val < 0x80) ||
			
			// UTF16 surrogates
			(val >= 0xD800 && val <= 0xDFFF)
		) {			
			*codepoint = -1;
			return 0u;
		};
		*codepoint = val;
		return 1u + cont_byte_count;
	}

	// if none of the above, this couldn't possibly be a valid encoding
	*codepoint = -1;
	return 0u;
}

string codepoint_to_utf8(unsigned int codepoint) {
	string out;
	if (codepoint < 0x80) {
		out += (unsigned char)codepoint;
	}
	else if (codepoint < 0x800) {
		out += (unsigned char)(0xc0 | (codepoint >> 6));
		out += (unsigned char)(0x80 | (codepoint & 0x3f));
	}
	else if (codepoint < 0x10000) {
		out += (unsigned char)(0xe0 | (codepoint >> 12));
		out += (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
		out += (unsigned char)(0x80 | (codepoint & 0x3f));
	}
	else if (codepoint < 0x110000) {
		out += (unsigned char)(0xf0 | (codepoint >> 18));
		out += (unsigned char)(0x80 | ((codepoint >> 12) & 0x3f));
		out += (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
		out += (unsigned char)(0x80 | (codepoint & 0x3f));
	}
	return out;
}

bool is_valid_utf8(const char* inp) {
	while (*inp != '\0') {
		int codepoint;
		inp += utf8_val(&codepoint, inp);

		if (codepoint == -1) return false;
	}

	return true;
}
