#include <stdio.h>
#include "decoder.h"

/**
 * Módulo de Decodificación
 * 
 * Este módulo implementa las funciones necesarias para la desencriptación
 * de caracteres y su representación segura en la salida. Utiliza la misma
 * operación XOR que el emisor para revertir la encriptación.
 */

/**
 * @brief Desencripta un carácter usando XOR con la clave
 * 
 * Realiza la operación XOR inversa para recuperar el carácter original.
 * Al ser XOR una operación simétrica, usar la misma clave dos veces
 * restaura el valor original.
 * 
 * @param value Carácter encriptado a desencriptar
 * @param key Clave de encriptación (la misma usada por el emisor)
 * @return Carácter desencriptado
 */
unsigned char xor_apply(unsigned char value, unsigned char key) {
    return value ^ key;
}

/**
 * @brief Verifica si un carácter es imprimible de forma segura
 * 
 * Determina si un carácter puede mostrarse directamente en la terminal
 * sin causar efectos secundarios no deseados.
 * 
 * @param c Carácter a verificar
 * @return 1 si el carácter es imprimible, 0 en caso contrario
 */
int is_printable_char(char c) {
    return (c >= 32 && c < 127);
}

/**
 * @brief Genera una representación segura de un carácter
 * 
 * Convierte cualquier carácter en una representación segura para mostrar,
 * escapando caracteres especiales y no imprimibles. Por ejemplo:
 * - \n para salto de línea
 * - \r para retorno de carro
 * - \xHH para otros caracteres no imprimibles (en hexadecimal)
 * 
 * @param c Carácter a convertir
 * @param out Buffer donde se escribirá la representación
 * @param outlen Tamaño del buffer de salida
 */
void safe_char_repr(char c, char* out, size_t outlen) {
    if (!out || outlen < 5) return;
    if (c == '\n') snprintf(out, outlen, "\\n");
    else if (c == '\r') snprintf(out, outlen, "\\r");
    else if (c == '\t') snprintf(out, outlen, "\\t");
    else if (c == '\0') snprintf(out, outlen, "\\0");
    else if (is_printable_char(c)) snprintf(out, outlen, "%c", c);
    else snprintf(out, outlen, "\\x%02X", (unsigned char)c);
}
