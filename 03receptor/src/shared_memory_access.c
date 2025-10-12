#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "constants.h"
#include "shared_memory_access.h"

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
        fprintf(stderr, RED "[ERROR] Memoria compartida corrupta o no inicializada\n" RESET);
        shmdt(shm);
        return NULL;
    }
    return shm;
}

int detach_shared_memory(SharedMemory* shm) {
    if (!shm) return SUCCESS;
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}
unsigned char* get_file_data_pointer(SharedMemory* shm) {
    return (unsigned char*)((char*)shm + shm->file_data_offset);
}

int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* out) {
    if (!shm || !out) return ERROR;
    if (slot_index < 0 || slot_index >= shm->buffer_size) return ERROR;
    *out = get_buffer_pointer(shm)[slot_index];
    return SUCCESS;
}

void clear_slot(SharedMemory* shm, int slot_index) {
    if (!shm) return;
    if (slot_index < 0 || slot_index >= shm->buffer_size) return;
    CharacterSlot* slot = &get_buffer_pointer(shm)[slot_index];
    slot->is_valid   = 0;
    slot->ascii_value= 0;
    // Conservamos timestamp y metadatos para auditoría si se desea
}
