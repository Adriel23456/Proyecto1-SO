// queue_operations.h
// Operaciones sobre las colas circulares en memoria compartida
// NOTA: Las funciones de modificación de colas deben llamarse
//       con el semáforo mutex correspondiente ya tomado

#ifndef QUEUE_OPERATIONS_H
#define QUEUE_OPERATIONS_H

#include "structures.h"

/**
 * SlotInfo - Información de un slot extraído de la cola de desencriptación
 * @slot_index: Índice del slot en el buffer [0..buffer_size-1]
 * @text_index: Índice del carácter en el texto original
 */
typedef struct {
    int slot_index;
    int text_index;
} SlotInfo;

/**
 * dequeue_decrypt_slot_ordered - Extrae el slot con el menor text_index
 * @shm: Puntero a la memoria compartida
 * 
 * Implementa extracción ordenada para garantizar secuencialidad.
 * Complejidad O(n) donde n es el tamaño actual de la cola.
 * DEBE ser llamado con g_sem_decrypt_queue tomado.
 * 
 * Retorna: SlotInfo con slot_index=-1 si la cola está vacía
 */
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm);

/**
 * enqueue_encrypt_slot - Devuelve un slot libre a la cola de encriptación
 * @shm: Puntero a la memoria compartida
 * @slot_index: Índice del slot a liberar [0..buffer_size-1]
 * 
 * DEBE ser llamado con g_sem_encrypt_queue tomado.
 * 
 * Retorna: SUCCESS o ERROR si la cola está llena
 */
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index);

#endif // QUEUE_OPERATIONS_H