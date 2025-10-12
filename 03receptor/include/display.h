#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>
#include "structures.h"

void print_receptor_banner(void);

void print_receive_status(SharedMemory* shm,
                          int slot_index,
                          unsigned char encrypted,
                          char decrypted,
                          int text_index,
                          time_t produced_at);

void print_reception_progress(SharedMemory* shm, int local_count);
void print_receptor_summary(SharedMemory* shm, int local_count, time_t start_ts);

void print_info(const char* msg);
void print_warn(const char* msg);
void print_err (const char* msg);

#endif // DISPLAY_H
