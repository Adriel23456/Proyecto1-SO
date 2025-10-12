#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>
#include "structures.h"

// Funciones de display principales
void print_emisor_banner();
void print_emission_status(SharedMemory* shm, int slot_index, char original, 
                          unsigned char encrypted, int text_index);
void print_emission_progress(SharedMemory* shm, int chars_sent);
void print_activity_summary(SharedMemory* shm, int chars_sent, time_t start_time);

// Funciones de mensajes
void print_warning(const char* message);
void print_error(const char* message);
void print_info(const char* message);

// Funciones de utilidad
void clear_screen();
void print_separator();

#endif // DISPLAY_H