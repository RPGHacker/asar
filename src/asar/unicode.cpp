#include "unicode.h"

// skips to the next UTF-8 codepoint in the input. doesn't validate input!
const char* utf8_next(const char* inp) {
	inp++;
	// skip all continuation bytes
	while (((unsigned char)(*inp) & 0xC0) == 0x80) inp++;
	return inp;
}

// returns the code point at *inp, or -1 on invalid UTF-8.
int utf8_val(const char* inp) {
	unsigned char c = *inp++;
	int val;
	if (c < 0x80) return c; // plain ascii
	else if (c < 0xC0) return -1; // start byte cannot be a cont. byte
	else if (c < 0xF8) {
		// 1, 2 or 3 continuation bytes
		int cont_byte_count = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : 1;
		// bit hack to extract the significant bits from the start byte
		val = (c & ((1 << (6 - cont_byte_count)) - 1));
		for (int i = 0; i < cont_byte_count; i++) {
			unsigned char next = *inp++;
			if ((next & 0xC0) != 0x80) return -1;
			val = (val << 6) | (next & 0x3F);
		}
		if ((*inp & 0xC0) == 0x80) return -1; // too many cont.bytes
		if (val > 0x10FFFF) return -1; // invalid codepoints
									   // check overlong encodings
		if ((cont_byte_count == 3 && val < 0x1000) ||
			(cont_byte_count == 2 && val < 0x800) ||
			(cont_byte_count == 1 && val < 0x80)) return -1;
		if (val >= 0xD800 && val <= 0xDFFF) return -1; // UTF16 surrogates
		return val;
	}
	// if more than F8, this couldn't possibly be a valid encoding
	else return -1;
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
