#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include "structures.h"

/*
 * Información retornada al sacar un elemento de la cola de desencriptación.
 */
typedef struct {
    int slot_index;
    int text_index;
} SlotInfo;

/*
 * Operaciones principales de colas sobre la SHM:
 *  - initialize_queues: configura ambas colas; encrypt llena con [0..buffer_size-1].
 *  - enqueue/dequeue en encrypt: maneja slots libres.
 *  - enqueue/dequeue en decrypt: maneja slots con datos; versión ordered preserva secuencia.
 *  - Utilidades: estado actual de colas y checks de vacío.
 */
void initialize_queues(SharedMemory* shm, int buffer_size);
void initialize_encrypt_queue(SharedMemory* shm, int buffer_size);
void initialize_decrypt_queue(SharedMemory* shm);

int  enqueue_encrypt_slot(SharedMemory* shm, int slot_index);
int  dequeue_encrypt_slot(SharedMemory* shm);

int      enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index);
SlotInfo dequeue_decrypt_slot(SharedMemory* shm);
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm);

void print_queue_status(SharedMemory* shm);
int  is_encrypt_queue_empty(SharedMemory* shm);
int  is_decrypt_queue_empty(SharedMemory* shm);

#endif // QUEUE_MANAGER_H
