#ifndef ENCODER_H
#define ENCODER_H

#include <stddef.h>

// Funciones de encriptación/desencriptación
unsigned char encrypt_character(char original, unsigned char key);
char decrypt_character(unsigned char encrypted, unsigned char key);
int verify_encryption(char original, unsigned char key);

// Funciones de display
void print_encryption_info(char original, unsigned char encrypted, unsigned char key);
void format_encrypted_display(unsigned char encrypted, char* buffer, size_t size);
int is_printable_char(char c);
void get_safe_char_display(char c, char* buffer, size_t size);

#endif // ENCODER_H