// shared_memory_access.h
// Funciones para adjuntar/desadjuntar memoria compartida System V
// y acceder a los slots del buffer circular

#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include <sys/types.h>
#include <sys/ipc.h>
#include "structures.h"

/**
 * attach_shared_memory - Conecta el proceso a la memoria compartida existente
 * @key: Clave System V (SHM_BASE_KEY)
 * 
 * Retorna: Puntero a SharedMemory o NULL si falla
 */
SharedMemory* attach_shared_memory(key_t key);

/**
 * detach_shared_memory - Desconecta el proceso de la memoria compartida
 * @shm: Puntero a la memoria compartida
 * 
 * Retorna: SUCCESS o ERROR
 */
int detach_shared_memory(SharedMemory* shm);

/**
 * get_buffer_pointer - Obtiene puntero al array de CharacterSlot
 * @shm: Puntero a la memoria compartida
 * 
 * Retorna: Puntero al inicio del buffer de slots
 */
CharacterSlot* get_buffer_pointer(SharedMemory* shm);

/**
 * get_slot_info - Lee la información de un slot específico
 * @shm: Puntero a la memoria compartida
 * @slot_index: Índice del slot [0..buffer_size-1]
 * @out: Estructura donde se copiará la info del slot
 * 
 * Retorna: SUCCESS o ERROR
 */
int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* out);

#endif // SHARED_MEMORY_ACCESS_H