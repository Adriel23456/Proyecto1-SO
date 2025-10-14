#include "encoder.h"

/**
 * Módulo de Encriptación
 * 
 * Este módulo implementa la lógica de encriptación de caracteres
 * utilizando una operación XOR simple. Cada carácter del texto
 * original se encripta usando una clave proporcionada.
 */

/**
 * @brief Encripta un carácter usando XOR con una clave
 * 
 * Realiza una operación XOR bit a bit entre el carácter original
 * y la clave de encriptación. Esta operación es reversible,
 * lo que permite la posterior desencriptación por el receptor.
 * 
 * @param original Carácter original a encriptar
 * @param key Clave de encriptación (1 byte)
 * @return Carácter encriptado
 */
unsigned char encrypt_character(char original, unsigned char key) {
    return (unsigned char)original ^ key;
}