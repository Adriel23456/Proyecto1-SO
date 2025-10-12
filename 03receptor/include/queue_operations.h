#ifndef QUEUE_OPERATIONS_H
#define QUEUE_OPERATIONS_H

#include "structures.h"

/* Operaciones sobre las colas en SHM.
   NOTA: Las funciones que modifican colas deben llamarse con
   el mutex POSIX correspondiente tomado externamente. */

typedef struct {
    int slot_index;
    int text_index;
} SlotInfo;

// Cola de desencriptación (datos listos para leer)
int      decrypt_queue_size(SharedMemory* shm);
SlotInfo dequeue_decrypt_slot_fifo(SharedMemory* shm);
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm);

// Encola un sentinela (-1,-1) en la cola de decrypt.
// Debe llamarse con g_sem_decrypt_queue tomado.
int enqueue_decrypt_sentinel(SharedMemory* shm);

// Cola de encriptación (slots libres)
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index);

// Utilidades
void print_queue_status(SharedMemory* shm);

#endif // QUEUE_OPERATIONS_H
