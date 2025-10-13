#include "encoder.h"

unsigned char encrypt_character(char original, unsigned char key) {
    return (unsigned char)original ^ key;
}