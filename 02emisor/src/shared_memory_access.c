#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include "shared_memory_access.h"
#include "constants.h"

/**
 * Módulo de Acceso a Memoria Compartida para el Emisor
 * 
 * Este módulo proporciona las funciones necesarias para que el emisor
 * interactúe con la memoria compartida, incluyendo la conexión inicial,
 * lectura de datos y almacenamiento de caracteres procesados.
 */

/**
 * @brief Conecta el emisor a la memoria compartida existente
 * 
 * Busca y se conecta al segmento de memoria compartida usando la clave
 * proporcionada. Realiza validaciones básicas para asegurar que la
 * memoria está correctamente inicializada.
 * 
 * @param key Clave IPC del segmento de memoria compartida
 * @return Puntero a la estructura SharedMemory, NULL si hay error
 */
SharedMemory* attach_shared_memory(key_t key) {
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] No se encontró memoria compartida con key 0x%04X: %s\n" RESET, 
                key, strerror(errno));
        return NULL;
    }
    
    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        return NULL;
    }
    
    if (shm->buffer_size <= 0 || shm->file_data_size <= 0) {
        fprintf(stderr, RED "[ERROR] Memoria compartida corrupta\n" RESET);
        shmdt(shm);
        return NULL;
    }
    
    return shm;
}

/**
 * @brief Desconecta el emisor de la memoria compartida
 * 
 * Libera la conexión del emisor con el segmento de memoria compartida.
 * Esta función debe llamarse antes de que el emisor termine.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @return SUCCESS si la operación fue exitosa, ERROR en caso contrario
 */
int detach_shared_memory(SharedMemory* shm) {
    if (shm == NULL) return SUCCESS;
    
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt falló: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

/**
 * @brief Lee un carácter del archivo original en memoria compartida
 * 
 * Accede a la región de datos del archivo en la memoria compartida
 * y retorna el carácter en la posición especificada.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @param position Posición del carácter a leer
 * @return El carácter leído, o '\0' si la posición es inválida
 */
char read_char_at_position(SharedMemory* shm, int position) {
    if (shm == NULL) return '\0';
    if (position < 0 || position >= shm->file_data_size) return '\0';
    
    unsigned char* file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    return (char)file_data[position];
}

/**
 * @brief Almacena un carácter encriptado en el buffer circular
 * 
 * Guarda un carácter procesado en el slot especificado del buffer,
 * junto con metadatos como el timestamp, índice original y PID del
 * emisor que lo procesó.
 * 
 * @param shm Puntero a la estructura SharedMemory
 * @param slot_index Índice del slot donde guardar el carácter
 * @param encrypted_char Carácter ya encriptado
 * @param text_index Posición original en el texto
 * @param emisor_pid PID del emisor que procesó el carácter
 */
void store_character(SharedMemory* shm, int slot_index, unsigned char encrypted_char, 
                     int text_index, pid_t emisor_pid) {
    if (shm == NULL || slot_index < 0 || slot_index >= shm->buffer_size) return;
    
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);
    CharacterSlot* slot = &buffer[slot_index];
    
    slot->ascii_value = encrypted_char;
    slot->slot_index = slot_index + 1;
    slot->timestamp = time(NULL);
    slot->is_valid = 1;
    slot->text_index = text_index;
    slot->emisor_pid = emisor_pid;
}