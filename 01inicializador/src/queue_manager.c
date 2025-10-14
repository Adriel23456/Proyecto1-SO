#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "queue_manager.h"
#include "constants.h"
#include "structures.h"

/**
 * Módulo de Gestión de Colas
 * 
 * Este módulo implementa dos colas circulares en memoria compartida:
 * 1. Cola de encriptación: Mantiene índices de slots libres
 * 2. Cola de desencriptación: Mantiene slots con datos procesados
 * 
 * Las colas usan offsets en lugar de punteros para garantizar
 * consistencia entre procesos en memoria compartida.
 */

/**
 * @brief Funciones auxiliares para acceder a los arrays físicos
 * 
 * Estas funciones calculan la dirección correcta de los arrays
 * usando offsets desde el inicio de la memoria compartida,
 * evitando el uso de punteros que serían inválidos entre procesos.
 */
static inline SlotRef* enc_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}
static inline SlotRef* dec_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

/**
 * @brief Inicializa ambas colas del sistema
 * 
 * Configura las colas de encriptación y desencriptación:
 * - Cola de encriptación: Se llena con todos los índices disponibles [0..buffer_size-1]
 * - Cola de desencriptación: Se inicializa vacía
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @param buffer_size Tamaño del buffer circular
 */
void initialize_queues(SharedMemory* shm, int buffer_size) {
    initialize_encrypt_queue(shm, buffer_size);
    initialize_decrypt_queue(shm);

    printf("  • Estado de las colas:\n");
    printf("    - QueueEncript: %d posiciones disponibles\n", shm->encrypt_queue.size);
    printf("    - QueueDeencript: %d elementos (vacía)\n", shm->decrypt_queue.size);
}

void initialize_encrypt_queue(SharedMemory* shm, int buffer_size) {
    Queue* q = &shm->encrypt_queue;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    q->capacity = buffer_size;

    SlotRef* arr = enc_array(shm);
    for (int i = 0; i < buffer_size; i++) {
        arr[q->tail].slot_index = i;
        arr[q->tail].text_index = -1;
        q->tail = (q->tail + 1) % q->capacity;
    }
    q->size = buffer_size;

    printf("  • Cola de encriptación inicializada:\n");
    printf("    Slots disponibles: ");
    int preview = (buffer_size < 5) ? buffer_size : 5;
    int pos = q->head;
    for (int i = 0; i < preview; i++) {
        printf("%d ", arr[pos].slot_index);
        pos = (pos + 1) % q->capacity;
    }
    if (buffer_size > 5) printf("... (%d total)", buffer_size);
    printf("\n");
}

void initialize_decrypt_queue(SharedMemory* shm) {
    Queue* q = &shm->decrypt_queue;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    // capacity se configuró en create_shared_memory
    printf("  • Cola de desencriptación inicializada (vacía)\n");
}

/**
 * @brief Agrega un slot libre a la cola de encriptación
 * 
 * Inserta un nuevo slot disponible en la cola de encriptación.
 * Esta operación se usa cuando un slot se libera después de ser procesado.
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @param slot_index Índice del slot a encolar
 * @return SUCCESS si la operación fue exitosa, ERROR si la cola está llena
 */
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    Queue* q = &shm->encrypt_queue;
    if (q->size >= q->capacity) return ERROR;
    SlotRef* arr = enc_array(shm);
    arr[q->tail].slot_index = slot_index;
    arr[q->tail].text_index = -1;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return SUCCESS;
}

/**
 * @brief Obtiene un slot libre de la cola de encriptación
 * 
 * Retira y retorna el siguiente slot disponible de la cola
 * de encriptación. Los emisores usan esta función para obtener
 * slots donde escribir nuevos datos.
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @return Índice del slot obtenido, -1 si la cola está vacía
 */
int dequeue_encrypt_slot(SharedMemory* shm) {
    Queue* q = &shm->encrypt_queue;
    if (q->size == 0) return -1;
    SlotRef* arr = enc_array(shm);
    int slot_index = arr[q->head].slot_index;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return slot_index;
}

/**
 * @brief Agrega un slot con datos a la cola de desencriptación
 * 
 * Inserta un slot que contiene datos encriptados en la cola
 * de desencriptación para que sea procesado por los receptores.
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @param slot_index Índice del slot con datos
 * @param text_index Posición del carácter en el texto original
 * @return SUCCESS si la operación fue exitosa, ERROR si la cola está llena
 */
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index) {
    Queue* q = &shm->decrypt_queue;
    if (q->size >= q->capacity) return ERROR;
    SlotRef* arr = dec_array(shm);
    arr[q->tail].slot_index = slot_index;
    arr[q->tail].text_index = text_index;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return SUCCESS;
}

/**
 * @brief Obtiene el siguiente slot con datos de la cola de desencriptación
 * 
 * Retira y retorna el siguiente slot con datos encriptados
 * siguiendo una política FIFO (First In, First Out).
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @return Estructura SlotInfo con los índices del slot y texto, -1 en ambos si la cola está vacía
 */
SlotInfo dequeue_decrypt_slot(SharedMemory* shm) {
    SlotInfo info = { .slot_index = -1, .text_index = -1 };
    Queue* q = &shm->decrypt_queue;
    if (q->size == 0) return info;
    SlotRef* arr = dec_array(shm);
    info.slot_index = arr[q->head].slot_index;
    info.text_index = arr[q->head].text_index;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return info;
}

/**
 * @brief Obtiene el slot con el menor text_index de la cola de desencriptación
 * 
 * Busca y retorna el slot que contiene el carácter con la menor
 * posición en el texto original. Esto permite mantener el orden
 * del texto al desencriptar. Complejidad O(n) donde n es el tamaño
 * actual de la cola.
 * 
 * @param shm Puntero a la estructura de memoria compartida
 * @return Estructura SlotInfo con los índices del slot y texto, -1 en ambos si la cola está vacía
 */
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm) {
    SlotInfo info = { .slot_index = -1, .text_index = -1 };
    Queue* q = &shm->decrypt_queue;
    if (q->size == 0) return info;

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
    if (best_pos == -1) return info;

    // Rotación hasta llevar el mejor elemento a head
    while (q->head != best_pos) {
        SlotRef tmp = arr[q->head];
        q->head = (q->head + 1) % q->capacity;
        arr[q->tail] = tmp;
        q->tail = (q->tail + 1) % q->capacity;
        // q->size no cambia
    }

    // Pop del elemento óptimo
    info.slot_index = arr[q->head].slot_index;
    info.text_index = arr[q->head].text_index;
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return info;
}

void print_queue_status(SharedMemory* shm) {
    printf("Estado de las colas:\n");
    printf("  • QueueEncript: %d/%d slots disponibles\n",
           shm->encrypt_queue.size, shm->encrypt_queue.capacity);
    printf("  • QueueDeencript: %d/%d elementos con datos\n",
           shm->decrypt_queue.size, shm->decrypt_queue.capacity);
}

int is_encrypt_queue_empty(SharedMemory* shm) {
    return shm->encrypt_queue.size == 0;
}

int is_decrypt_queue_empty(SharedMemory* shm) {
    return shm->decrypt_queue.size == 0;
}
