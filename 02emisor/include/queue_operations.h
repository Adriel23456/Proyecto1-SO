#ifndef QUEUE_OPERATIONS_H
#define QUEUE_OPERATIONS_H

#include "structures.h"

// Operaciones de cola de encriptación
int dequeue_encrypt_slot(SharedMemory* shm);
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index);

// Operaciones de cola de desencriptación
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index);

// Funciones de estado
int is_encrypt_queue_empty(SharedMemory* shm);
int is_decrypt_queue_full(SharedMemory* shm);
int get_available_encrypt_slots(SharedMemory* shm);
int get_decrypt_queue_size(SharedMemory* shm);

// Debugging
void print_queue_status(SharedMemory* shm);

#endif // QUEUE_OPERATIONS_H