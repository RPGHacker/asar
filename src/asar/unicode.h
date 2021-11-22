#if !defined(ASAR_UNICODE_H)
#define ASAR_UNICODE_H

#include "libstr.h"

// Writes the code point at *inp into codepoint (-1 on invalid UTF-8).
// Returns the number of bytes consumed for the code point (0 for invalid UTF-8).
size_t utf8_val(int* codepoint, const char* inp);

// converts a unicode codepoint to a UTF-8 byte sequence
string codepoint_to_utf8(unsigned int codepoint);

// checks if input string contains only valid UTF-8
bool is_valid_utf8(const char* inp);

#endif
