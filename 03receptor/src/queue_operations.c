#include <stdio.h>
#include <limits.h>
#include "queue_operations.h"
#include "constants.h"

static inline SlotRef* enc_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}
static inline SlotRef* dec_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

int decrypt_queue_size(SharedMemory* shm) {
    if (!shm) return 0;
    return shm->decrypt_queue.size;
}

SlotInfo dequeue_decrypt_slot_fifo(SharedMemory* shm) {
    SlotInfo info = { .slot_index = -1, .text_index = -1 };
    if (!shm) return info;
    Queue* q = &shm->decrypt_queue;
    if (q->size == 0) return info;

    SlotRef* arr = dec_array(shm);
    info.slot_index = arr[q->head].slot_index;
    info.text_index = arr[q->head].text_index;

    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return info;
}

SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm) {
    SlotInfo info = { .slot_index = -1, .text_index = -1 };
    if (!shm) return info;
    Queue* q = &shm->decrypt_queue;
    if (q->size == 0) return info;

    SlotRef* arr = dec_array(shm);
    int best_pos = -1;
    int best_txt = INT_MAX;

    for (int i = 0, pos = q->head; i < q->size; i++, pos = (pos + 1) % q->capacity) {
        if (arr[pos].text_index < best_txt) {
            best_txt = arr[pos].text_index;
            best_pos = pos;
        }
    }
    if (best_pos == -1) return info;

    // rotar hasta que best_pos quede en head
    while (q->head != best_pos) {
        SlotRef tmp = arr[q->head];
        q->head = (q->head + 1) % q->capacity;
        arr[q->tail] = tmp;
        q->tail = (q->tail + 1) % q->capacity;
    }

    info.slot_index = arr[q->head].slot_index;
    info.text_index = arr[q->head].text_index;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return info;
}

int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    if (!shm) return ERROR;
    Queue* q = &shm->encrypt_queue;
    if (q->size >= q->capacity) return ERROR;

    SlotRef* arr = enc_array(shm);
    arr[q->tail].slot_index = slot_index;
    arr[q->tail].text_index = -1;

    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return SUCCESS;
}

void print_queue_status(SharedMemory* shm) {
    if (!shm) return;
    printf(CYAN "\n[Receptor] Estado de colas: enc %d/%d  |  dec %d/%d\n" RESET,
           shm->encrypt_queue.size, shm->encrypt_queue.capacity,
           shm->decrypt_queue.size, shm->decrypt_queue.capacity);
}

int enqueue_decrypt_sentinel(SharedMemory* shm) {
    if (!shm) return ERROR;
    Queue* q = &shm->decrypt_queue;
    if (q->size >= q->capacity) return ERROR;
    SlotRef* arr = dec_array(shm);
    arr[q->tail].slot_index = -1;  // sentinela
    arr[q->tail].text_index = -1;  // sentinela
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return SUCCESS;
}
