#ifndef DECODER_H
#define DECODER_H

unsigned char xor_apply(unsigned char value, unsigned char key); // simétrico
int is_printable_char(char c);
void safe_char_repr(char c, char* out, size_t outlen);

#endif // DECODER_H