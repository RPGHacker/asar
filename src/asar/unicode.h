#if !defined(ASAR_UNICODE_H)
#define ASAR_UNICODE_H

#include "libstr.h"

const char* utf8_next(const char* inp);
int utf8_val(const char* inp);
string codepoint_to_utf8(unsigned int codepoint);

#endif
