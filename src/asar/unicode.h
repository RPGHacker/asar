#if !defined(ASAR_UNICODE_H)
#define ASAR_UNICODE_H

#include "libstr.h"

#include <string>

// Writes the code point at *inp into codepoint (-1 on invalid UTF-8).
// Returns the number of bytes consumed for the code point (0 for invalid UTF-8).
size_t utf8_val(int* codepoint, const char* inp);

// converts a unicode codepoint to a UTF-8 byte sequence
bool codepoint_to_utf8(string* out, unsigned int codepoint);

// checks if input string contains only valid UTF-8
bool is_valid_utf8(const char* inp);


// RPG Hacker: UTF-16 functions below expect wchar_t to be at least 16-bit wide.
// Not that this should be a problem, since we shouldn't need those for anything aside from Windows.
// char16_t would be more reliable, but would mean always having to cast to wchar_t* for Windows APIs.

// Same as utf8_val(), but for UTF-16.
size_t utf16_val(int* codepoint, const wchar_t* inp);

// Same as codepoint_to_utf8(), but returns a UTF-16 sequence.
// RPG Hacker: I use std::wstring here, because Asar doesn't have its own wstring class,
// and making one just for this occasion would be overkill.
// This seems like the least annoying path forward.
bool codepoint_to_utf16(std::wstring* out, unsigned int codepoint);

// Converters a UTF-16 string to a UTF-8 string.
bool utf16_to_utf8(string* result, const wchar_t* u16_str);

// Converters a UTF-8 string to a UTF-16 string.
bool utf8_to_utf16(std::wstring* result, const char* u8_str);

#endif
