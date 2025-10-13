// queue_operations.c
// Implementación de operaciones sobre colas circulares en SHM

#include <stdio.h>
#include <limits.h>
#include "queue_operations.h"
#include "constants.h"

/**
 * Macros para acceder a los arrays de las colas mediante sus offsets
 * Esto evita usar punteros inválidos entre procesos
 */
static inline SlotRef* enc_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}

static inline SlotRef* dec_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

/**
 * Extrae el elemento con el MENOR text_index de la cola de desencriptación
 * 
 * Algoritmo:
 * 1. Buscar linealmente el elemento con menor text_index
 * 2. Rotar la cola para que ese elemento quede en head
 * 3. Extraer el elemento de head
 * 
 * Esto garantiza que los receptores procesen los caracteres en orden secuencial
 * sin importar el orden en que fueron producidos por múltiples emisores.
 */
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm) {
    SlotInfo info = { .slot_index = -1, .text_index = -1 };
    
    if (!shm) return info;
    
    Queue* q = &shm->decrypt_queue;
    if (q->size == 0) return info;  // Cola vacía
    
    SlotRef* arr = dec_array(shm);
    int best_pos = -1;
    int best_text = INT_MAX;
    
    // Búsqueda lineal del text_index mínimo en la cola circular
    for (int i = 0, pos = q->head; i < q->size; i++, pos = (pos + 1) % q->capacity) {
        if (arr[pos].text_index < best_text) {
            best_text = arr[pos].text_index;
            best_pos = pos;
        }
    }
    
    if (best_pos == -1) return info;  // No se encontró (no debería pasar)
    
    // Rotar la cola hasta que el mejor elemento quede en head
    // Esto preserva el orden relativo de los demás elementos
    while (q->head != best_pos) {
        SlotRef tmp = arr[q->head];
        q->head = (q->head + 1) % q->capacity;
        arr[q->tail] = tmp;
        q->tail = (q->tail + 1) % q->capacity;
        // El tamaño no cambia durante la rotación
    }
    
    // Extraer el elemento en head (el de menor text_index)
    info.slot_index = arr[q->head].slot_index;
    info.text_index = arr[q->head].text_index;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    
    return info;
}

/**
 * Devuelve un slot libre a la cola de encriptación
 * 
 * Esto permite que los emisores puedan reutilizar el slot para
 * escribir un nuevo carácter encriptado.
 */
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    if (!shm) return ERROR;
    
    Queue* q = &shm->encrypt_queue;
    if (q->size >= q->capacity) return ERROR;  // Cola llena (no debería pasar)
    
    SlotRef* arr = enc_array(shm);
    
    // Agregar el slot libre al final de la cola
    arr[q->tail].slot_index = slot_index;
    arr[q->tail].text_index = -1;  // No aplica para slots libres
    
    // Avanzar tail circularmente
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    
    return SUCCESS;
}