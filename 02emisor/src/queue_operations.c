#include <stdio.h>
#include <string.h>
#include "queue_operations.h"
#include "constants.h"
#include "structures.h"

/*
 * Acceso a los arrays de las colas mediante offset.
 */
static inline SlotRef* get_encrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}

static inline SlotRef* get_decrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

/*
 * Dequeue de un slot libre de la cola de encriptación.
 * DEBE ser llamado con el mutex de la cola ya adquirido.
 */
int dequeue_encrypt_slot(SharedMemory* shm) {
    if (shm == NULL) return -1;
    
    Queue* queue = &shm->encrypt_queue;
    if (queue->size == 0) {
        return -1;  // Cola vacía
    }
    
    SlotRef* array = get_encrypt_array(shm);
    int slot_index = array[queue->head].slot_index;
    
    // Avanzar head circularmente
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    return slot_index;
}

/*
 * Enqueue de un slot libre en la cola de encriptación.
 * DEBE ser llamado con el mutex de la cola ya adquirido.
 */
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    if (shm == NULL) return ERROR;
    
    Queue* queue = &shm->encrypt_queue;
    if (queue->size >= queue->capacity) {
        return ERROR;  // Cola llena (no debería pasar)
    }
    
    SlotRef* array = get_encrypt_array(shm);
    array[queue->tail].slot_index = slot_index;
    array[queue->tail].text_index = -1;
    
    // Avanzar tail circularmente
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    return SUCCESS;
}

/*
 * Enqueue de un slot con datos en la cola de desencriptación.
 * DEBE ser llamado con el mutex de la cola ya adquirido.
 */
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index) {
    if (shm == NULL) return ERROR;
    
    Queue* queue = &shm->decrypt_queue;
    if (queue->size >= queue->capacity) {
        fprintf(stderr, RED "[WARNING] Cola de desencriptación llena\n" RESET);
        return ERROR;
    }
    
    SlotRef* array = get_decrypt_array(shm);
    array[queue->tail].slot_index = slot_index;
    array[queue->tail].text_index = text_index;
    
    // Avanzar tail circularmente
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    return SUCCESS;
}

/*
 * Verificar si la cola de encriptación está vacía.
 */
int is_encrypt_queue_empty(SharedMemory* shm) {
    if (shm == NULL) return 1;
    return shm->encrypt_queue.size == 0;
}

/*
 * Verificar si la cola de desencriptación está llena.
 */
int is_decrypt_queue_full(SharedMemory* shm) {
    if (shm == NULL) return 1;
    return shm->decrypt_queue.size >= shm->decrypt_queue.capacity;
}

/*
 * Obtener el número de slots disponibles en la cola de encriptación.
 */
int get_available_encrypt_slots(SharedMemory* shm) {
    if (shm == NULL) return 0;
    return shm->encrypt_queue.size;
}

/*
 * Obtener el número de elementos en la cola de desencriptación.
 */
int get_decrypt_queue_size(SharedMemory* shm) {
    if (shm == NULL) return 0;
    return shm->decrypt_queue.size;
}

/*
 * Imprimir estado de las colas (para debugging).
 */
void print_queue_status(SharedMemory* shm) {
    if (shm == NULL) return;
    
    printf(CYAN "\n--- Estado de Colas ---\n" RESET);
    
    Queue* enc = &shm->encrypt_queue;
    printf("Cola Encrypt (slots libres):\n");
    printf("  Head: %d, Tail: %d, Size: %d/%d\n", 
           enc->head, enc->tail, enc->size, enc->capacity);
    
    if (enc->size > 0 && enc->size <= 10) {
        // Mostrar primeros elementos si hay pocos
        SlotRef* array = get_encrypt_array(shm);
        printf("  Slots disponibles: ");
        int pos = enc->head;
        for (int i = 0; i < enc->size && i < 10; i++) {
            printf("%d ", array[pos].slot_index);
            pos = (pos + 1) % enc->capacity;
        }
        printf("\n");
    }
    
    Queue* dec = &shm->decrypt_queue;
    printf("Cola Decrypt (datos para leer):\n");
    printf("  Head: %d, Tail: %d, Size: %d/%d\n",
           dec->head, dec->tail, dec->size, dec->capacity);
    
    if (dec->size > 0 && dec->size <= 10) {
        // Mostrar primeros elementos si hay pocos
        SlotRef* array = get_decrypt_array(shm);
        printf("  Elementos (slot,text): ");
        int pos = dec->head;
        for (int i = 0; i < dec->size && i < 10; i++) {
            printf("(%d,%d) ", array[pos].slot_index, array[pos].text_index);
            pos = (pos + 1) % dec->capacity;
        }
        printf("\n");
    }
    
    printf("------------------------\n");
}