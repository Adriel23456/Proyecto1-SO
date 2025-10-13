#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>
#include "structures.h"

void print_emisor_banner();
void print_emission_status(SharedMemory* shm, int slot_index, char original, 
                          unsigned char encrypted, int text_index);

#endif