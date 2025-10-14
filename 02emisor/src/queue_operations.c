#include <stdio.h>
#include <string.h>
#include "queue_operations.h"
#include "constants.h"

/**
 * Módulo de Operaciones de Cola para el Emisor
 * 
 * Este módulo implementa las operaciones necesarias para que el emisor
 * interactúe con las colas circulares del sistema, tanto para obtener
 * slots libres como para encolar datos procesados.
 */

/**
 * @brief Obtiene el array de la cola de encriptación
 * 
 * Calcula la dirección correcta del array de la cola de encriptación
 * usando el offset almacenado en la memoria compartida.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @return Puntero al array de SlotRef para la cola de encriptación
 */
static inline SlotRef* get_encrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->encrypt_queue.array_offset);
}

/**
 * @brief Obtiene el array de la cola de desencriptación
 * 
 * Calcula la dirección correcta del array de la cola de desencriptación
 * usando el offset almacenado en la memoria compartida.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @return Puntero al array de SlotRef para la cola de desencriptación
 */
static inline SlotRef* get_decrypt_array(SharedMemory* shm) {
    return (SlotRef*)((char*)shm + shm->decrypt_queue.array_offset);
}

/**
 * @brief Obtiene un slot libre de la cola de encriptación
 * 
 * Retira un slot disponible de la cola de encriptación para
 * que el emisor pueda usarlo para almacenar un nuevo carácter.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @return Índice del slot obtenido, -1 si la cola está vacía
 */
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

/**
 * @brief Devuelve un slot a la cola de encriptación
 * 
 * Reinserta un slot en la cola de encriptación después de que
 * su contenido ha sido procesado y ya no se necesita.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @param slot_index Índice del slot a devolver
 * @return SUCCESS si la operación fue exitosa, ERROR en caso contrario
 */
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

/**
 * @brief Encola un slot con datos en la cola de desencriptación
 * 
 * Añade un slot que contiene un carácter encriptado a la cola
 * de desencriptación para que sea procesado por los receptores.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @param slot_index Índice del slot con datos
 * @param text_index Posición original en el texto
 * @return SUCCESS si la operación fue exitosa, ERROR en caso contrario
 */
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