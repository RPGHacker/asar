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
    // And anything below 0xC0 indicates a follow-up byte and should never be at the
    // start of a sequence.
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
        if (  // too many cont.bytes
                (*inp & 0xC0) == 0x80 ||

                // invalid codepoints
                val > 0x10FFFF ||

                // check overlong encodings
                (cont_byte_count == 3 && val < 0x1000) ||
                (cont_byte_count == 2 && val < 0x800) ||
                (cont_byte_count == 1 && val < 0x80) ||

                // UTF16 surrogates
                (val >= 0xD800 && val <= 0xDFFF)) {
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

bool codepoint_to_utf8(string* out, unsigned int codepoint) {
    *out = "";
    if (codepoint < 0x80) {
        *out += (unsigned char)codepoint;
    } else if (codepoint < 0x800) {
        *out += (unsigned char)(0xc0 | (codepoint >> 6));
        *out += (unsigned char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint < 0x10000) {
        *out += (unsigned char)(0xe0 | (codepoint >> 12));
        *out += (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out += (unsigned char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint < 0x110000) {
        *out += (unsigned char)(0xf0 | (codepoint >> 18));
        *out += (unsigned char)(0x80 | ((codepoint >> 12) & 0x3f));
        *out += (unsigned char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out += (unsigned char)(0x80 | (codepoint & 0x3f));
    } else
        return false;

    return true;
}

bool is_valid_utf8(const char* inp) {
    while (*inp != '\0') {
        int codepoint;
        inp += utf8_val(&codepoint, inp);

        if (codepoint == -1) return false;
    }

    return true;
}

size_t utf16_val(int* codepoint, const wchar_t* inp) {
    wchar_t first_word = *inp;

    if (first_word <= 0xD800 || first_word >= 0xDFFF) {
        // Single word
        *codepoint = first_word;
        return 1u;
    } else if (first_word >= 0xD800 && first_word <= 0xDBFF) {
        // Start of a surrogate pair
        wchar_t second_word = *(inp + 1);

        if (second_word >= 0xDC00 && second_word <= 0xDFFF) {
            *codepoint = 0x10000 + ((int)(first_word - 0xD800) << 10u) +
                         ((int)(second_word - 0xDC00));
            return 2u;
        }
    }

    // Everything not covered above is considered invalid.
    *codepoint = -1;
    return 0u;
}

bool codepoint_to_utf16(std::wstring* out, unsigned int codepoint) {
    if (codepoint >= 0x10000 && codepoint <= 0x10FFFF) {
        wchar_t high = (wchar_t)(((codepoint - 0x10000) >> 10) + 0xD800);
        wchar_t low = (wchar_t)(((codepoint - 0x10000) & 0b1111111111) + 0xDC00);

        *out = std::wstring() + high + low;
        return true;
    } else if (codepoint <= 0xD800 || codepoint >= 0xDFFF) {
        *out = std::wstring() + (wchar_t)codepoint;
        return true;
    }

    // Everything not covered above should be considered invalid.
    return false;
}


bool utf16_to_utf8(string* result, const wchar_t* u16_str) {
    *result = "";

    int codepoint;
    do {
        u16_str += utf16_val(&codepoint, u16_str);

        string next;
        if (codepoint == -1 || !codepoint_to_utf8(&next, codepoint)) return false;

        *result += next;
    } while (codepoint != 0);

    return true;
}

bool utf8_to_utf16(std::wstring* result, const char* u8_str) {
    *result = L"";

    int codepoint;
    do {
        u8_str += utf8_val(&codepoint, u8_str);

        std::wstring next;
        if (codepoint == -1 || !codepoint_to_utf16(&next, codepoint)) return false;

        *result += next;
    } while (codepoint != 0);

    return true;
}
