#include <stdio.h>
#include "decoder.h"

unsigned char xor_apply(unsigned char value, unsigned char key) {
    return value ^ key;
}

int is_printable_char(char c) {
    return (c >= 32 && c < 127);
}

void safe_char_repr(char c, char* out, size_t outlen) {
    if (!out || outlen < 5) return;
    if (c == '\n') snprintf(out, outlen, "\\n");
    else if (c == '\r') snprintf(out, outlen, "\\r");
    else if (c == '\t') snprintf(out, outlen, "\\t");
    else if (c == '\0') snprintf(out, outlen, "\\0");
    else if (is_printable_char(c)) snprintf(out, outlen, "%c", c);
    else snprintf(out, outlen, "\\x%02X", (unsigned char)c);
}
