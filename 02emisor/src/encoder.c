#include <stdio.h>
#include <stdlib.h>
#include "encoder.h"
#include "constants.h"

/*
 * Encriptar un carácter usando XOR con la clave.
 */
unsigned char encrypt_character(char original, unsigned char key) {
    return (unsigned char)original ^ key;
}

/*
 * Desencriptar un carácter (para verificación).
 * XOR es simétrico: encrypt(encrypt(x, k), k) = x
 */
char decrypt_character(unsigned char encrypted, unsigned char key) {
    return (char)(encrypted ^ key);
}

/*
 * Verificar que la encriptación/desencriptación funciona correctamente.
 */
int verify_encryption(char original, unsigned char key) {
    unsigned char encrypted = encrypt_character(original, key);
    char decrypted = decrypt_character(encrypted, key);
    return (original == decrypted) ? SUCCESS : ERROR;
}

/*
 * Mostrar información de encriptación para un carácter.
 */
void print_encryption_info(char original, unsigned char encrypted, unsigned char key) {
    printf("Encriptación XOR:\n");
    printf("  • Original: '%c' (0x%02X)\n", 
           (original >= 32 && original < 127) ? original : '.',
           (unsigned char)original);
    printf("  • Clave: 0x%02X\n", key);
    printf("  • Encriptado: 0x%02X\n", encrypted);
    
    // Mostrar operación binaria
    printf("  • Binario:\n");
    printf("    Original:   ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", ((unsigned char)original >> i) & 1);
    }
    printf("\n");
    printf("    Clave:      ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (key >> i) & 1);
    }
    printf("\n");
    printf("    ----------- XOR\n");
    printf("    Encriptado: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (encrypted >> i) & 1);
    }
    printf("\n");
}

/*
 * Generar una visualización del carácter encriptado.
 */
void format_encrypted_display(unsigned char encrypted, char* buffer, size_t size) {
    if (buffer == NULL || size < 10) return;
    
    // Mostrar como hexadecimal
    snprintf(buffer, size, "0x%02X", encrypted);
}

/*
 * Determinar si un carácter es imprimible.
 */
int is_printable_char(char c) {
    return (c >= 32 && c < 127);
}

/*
 * Obtener representación segura de un carácter para display.
 */
void get_safe_char_display(char c, char* buffer, size_t size) {
    if (buffer == NULL || size < 5) return;
    
    if (c == '\n') {
        snprintf(buffer, size, "\\n");
    } else if (c == '\r') {
        snprintf(buffer, size, "\\r");
    } else if (c == '\t') {
        snprintf(buffer, size, "\\t");
    } else if (c == '\0') {
        snprintf(buffer, size, "\\0");
    } else if (is_printable_char(c)) {
        snprintf(buffer, size, "%c", c);
    } else {
        snprintf(buffer, size, "\\x%02X", (unsigned char)c);
    }
}