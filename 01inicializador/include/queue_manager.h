#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include "structures.h"

// Estructura para información de slot dequeue
typedef struct {
    int slot_index;
    int text_index;
} SlotInfo;

// Funciones principales de colas
void initialize_queues(SharedMemory* shm, int buffer_size);
void initialize_encrypt_queue(SharedMemory* shm, int buffer_size);
void initialize_decrypt_queue(SharedMemory* shm);

// Operaciones de cola de encriptación
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index);
int dequeue_encrypt_slot(SharedMemory* shm);

// Operaciones de cola de desencriptación
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index);
SlotInfo dequeue_decrypt_slot(SharedMemory* shm);
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm);

// Funciones de estado
void print_queue_status(SharedMemory* shm);
int is_encrypt_queue_empty(SharedMemory* shm);
int is_decrypt_queue_empty(SharedMemory* shm);

#endif // QUEUE_MANAGER_H