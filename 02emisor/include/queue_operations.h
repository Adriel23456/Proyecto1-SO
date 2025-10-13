#ifndef QUEUE_OPERATIONS_H
#define QUEUE_OPERATIONS_H

#include "structures.h"

int dequeue_encrypt_slot(SharedMemory* shm);
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index);
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index);

#endif