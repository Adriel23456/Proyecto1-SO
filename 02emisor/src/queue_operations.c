#include <stdio.h>
#include <string.h>
#include "queue_operations.h"
#include "constants.h"

static inline SlotRef* get_encrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}

static inline SlotRef* get_decrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

int dequeue_encrypt_slot(SharedMemory* shm) {
    if (shm == NULL) return -1;
    
    Queue* queue = &shm->encrypt_queue;
    if (queue->size == 0) return -1;
    
    SlotRef* array = get_encrypt_array(shm);
    int slot_index = array[queue->head].slot_index;
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    return slot_index;
}

int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    if (shm == NULL) return ERROR;
    
    Queue* queue = &shm->encrypt_queue;
    if (queue->size >= queue->capacity) return ERROR;
    
    SlotRef* array = get_encrypt_array(shm);
    array[queue->tail].slot_index = slot_index;
    array[queue->tail].text_index = -1;
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    return SUCCESS;
}

int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index) {
    if (shm == NULL) return ERROR;
    
    Queue* queue = &shm->decrypt_queue;
    if (queue->size >= queue->capacity) return ERROR;
    
    SlotRef* array = get_decrypt_array(shm);
    array[queue->tail].slot_index = slot_index;
    array[queue->tail].text_index = text_index;
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    return SUCCESS;
}