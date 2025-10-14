/**
 * Módulo de Acceso a Memoria Compartida
 * 
 * Este módulo proporciona las funciones necesarias para que el receptor
 * acceda a la memoria compartida System V creada por el inicializador.
 * Incluye funciones para conectarse y desconectarse de la memoria compartida,
 * así como para acceder a las estructuras de datos dentro de ella de manera
 * segura usando offsets en lugar de punteros directos.
 * 
 * La memoria compartida contiene un buffer circular de slots de caracteres
 * y las colas de sincronización entre emisores y receptores.
 */

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
/**
 * @brief Conecta el proceso a la memoria compartida existente
 * 
 * Localiza y se conecta al segmento de memoria compartida usando la clave
 * proporcionada. Realiza verificaciones básicas de integridad para
 * asegurar que la memoria está correctamente inicializada.
 * 
 * @param key Clave IPC que identifica el segmento de memoria
 * @return Puntero a la memoria compartida o NULL en caso de error
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
/**
 * @brief Desconecta el proceso de la memoria compartida
 * 
 * Desvincula el proceso del segmento de memoria compartida.
 * Esto no destruye el segmento, solo elimina el mapeo en el
 * espacio de direcciones del proceso actual.
 * 
 * @param shm Puntero a la memoria compartida a desconectar
 * @return SUCCESS en caso de éxito, ERROR en caso contrario
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
/**
 * @brief Obtiene puntero al buffer de slots de caracteres
 * 
 * Calcula la dirección del buffer de slots usando el offset almacenado
 * en la memoria compartida. Esto garantiza que el puntero sea válido
 * en todos los procesos.
 * 
 * @param shm Puntero a la memoria compartida
 * @return Puntero al buffer de slots o NULL en caso de error
 */
CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    if (!shm) return NULL;
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}

/**
 * Lee la información completa de un slot específico
 */
/**
 * @brief Lee la información completa de un slot específico
 * 
 * Copia la información completa de un slot del buffer a una estructura
 * proporcionada por el llamador. Verifica que el índice sea válido.
 * 
 * @param shm Puntero a la memoria compartida
 * @param slot_index Índice del slot a leer
 * @param out Puntero a la estructura donde copiar la información
 * @return SUCCESS si se leyó correctamente, ERROR en caso contrario
 */
int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* out) {
    if (!shm || !out) return ERROR;
    if (slot_index < 0 || slot_index >= shm->buffer_size) return ERROR;
    
    // Copiar el slot completo a la estructura de salida
    *out = get_buffer_pointer(shm)[slot_index];
    return SUCCESS;
}