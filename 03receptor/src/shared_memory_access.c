// shared_memory_access.c
// Implementación de funciones para manejo de memoria compartida System V

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "constants.h"
#include "shared_memory_access.h"

/**
 * Conecta el proceso a la memoria compartida creada por el inicializador
 */
SharedMemory* attach_shared_memory(key_t key) {
    // Obtener ID del segmento existente
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] No se encontró memoria compartida con key 0x%04X: %s\n" RESET,
                key, strerror(errno));
        return NULL;
    }
    
    // Adjuntar el segmento a nuestro espacio de direcciones
    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        return NULL;
    }
    
    // Verificación básica de integridad
    if (shm->buffer_size <= 0 || shm->file_data_size <= 0) {
        fprintf(stderr, RED "[ERROR] Memoria compartida corrupta o no inicializada\n" RESET);
        shmdt(shm);
        return NULL;
    }
    
    return shm;
}

/**
 * Desconecta el proceso de la memoria compartida
 * NOTA: Esto NO destruye el segmento, solo desadjunta el proceso
 */
int detach_shared_memory(SharedMemory* shm) {
    if (!shm) return SUCCESS;
    
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    
    return SUCCESS;
}

/**
 * Obtiene puntero al buffer de CharacterSlot usando el offset almacenado en SHM
 */
CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    if (!shm) return NULL;
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}

/**
 * Lee la información completa de un slot específico
 */
int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* out) {
    if (!shm || !out) return ERROR;
    if (slot_index < 0 || slot_index >= shm->buffer_size) return ERROR;
    
    // Copiar el slot completo a la estructura de salida
    *out = get_buffer_pointer(shm)[slot_index];
    return SUCCESS;
}