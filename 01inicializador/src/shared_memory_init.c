#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include "shared_memory_init.h"
#include "constants.h"
#include "structures.h"

// Crear y configurar la memoria compartida
SharedMemory* create_shared_memory(int buffer_size, int file_size) {
    key_t key = SHM_BASE_KEY;
    
    // Calcular el tamaño total necesario
    size_t base_size = sizeof(SharedMemory);
    size_t buffer_data_size = buffer_size * sizeof(CharacterSlot);
    size_t file_data_size = file_size;
    size_t total_size = base_size + buffer_data_size + file_data_size;
    
    // Alinear al tamaño de página
    total_size = ((total_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    
    printf("  • Tamaño base de estructura: %ld bytes\n", base_size);
    printf("  • Tamaño del buffer: %ld bytes (%d slots)\n", buffer_data_size, buffer_size);
    printf("  • Tamaño de datos del archivo: %d bytes\n", file_size);
    printf("  • Tamaño total alineado: %ld bytes\n", total_size);
    
    // Intentar eliminar memoria compartida previa si existe
    int old_shmid = shmget(key, 0, 0);
    if (old_shmid != -1) {
        printf(YELLOW "  ! Memoria compartida existente detectada, eliminando...\n" RESET);
        if (shmctl(old_shmid, IPC_RMID, NULL) == -1) {
            fprintf(stderr, RED "  [WARNING] No se pudo eliminar memoria previa\n" RESET);
        }
    }
    
    // Crear el segmento de memoria compartida
    int shmid = shmget(key, total_size, IPC_CREAT | IPC_EXCL | IPC_PERMS);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] shmget falló: %s\n" RESET, strerror(errno));
        return NULL;
    }
    
    printf("  • ID de segmento: %d\n", shmid);
    
    // Adjuntar el segmento a nuestro espacio de direcciones
    SharedMemory* shm = (SharedMemory*) shmat(shmid, NULL, 0);
    if (shm == (void*) -1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        shmctl(shmid, IPC_RMID, NULL);
        return NULL;
    }
    
    // Inicializar toda la memoria a cero
    memset(shm, 0, total_size);
    
    return shm;
}

// Inicializar los slots del buffer de caracteres
void initialize_buffer_slots(SharedMemory* shm, int buffer_size) {
    // Obtener puntero al inicio del buffer
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);
    
    for (int i = 0; i < buffer_size; i++) {
        buffer[i].ascii_value = 0;
        buffer[i].slot_index = i + 1;  // Índices empiezan en 1
        buffer[i].timestamp = 0;
        buffer[i].is_valid = 0;
        buffer[i].text_index = -1;
        buffer[i].emisor_pid = 0;
    }
    
    // Imprimir algunos slots de ejemplo
    printf("  • Slots inicializados:\n");
    for (int i = 0; i < MIN(3, buffer_size); i++) {
        printf("    - Slot %d: índice=%d, vacío\n", i, buffer[i].slot_index);
    }
    if (buffer_size > 3) {
        printf("    ... y %d más\n", buffer_size - 3);
    }
}

// Copiar los datos del archivo a memoria compartida
void copy_file_to_shared_memory(SharedMemory* shm, unsigned char* file_data, int file_size) {
    // Obtener puntero al inicio de los datos del archivo
    unsigned char* shm_file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    
    // Copiar los datos
    memcpy(shm_file_data, file_data, file_size);
    
    // Verificar los primeros bytes
    printf("  • Primeros bytes del archivo en memoria compartida:\n    ");
    int preview_size = MIN(20, file_size);
    for (int i = 0; i < preview_size; i++) {
        if (shm_file_data[i] >= 32 && shm_file_data[i] < 127) {
            printf("%c", shm_file_data[i]);
        } else {
            printf("\\x%02x", shm_file_data[i]);
        }
    }
    if (file_size > preview_size) {
        printf("...");
    }
    printf("\n");
}

// Adjuntar a memoria compartida existente (para otros procesos)
SharedMemory* attach_shared_memory(key_t key) {
    // Obtener el ID del segmento existente
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] No se encontró memoria compartida con key 0x%04X\n" RESET, key);
        return NULL;
    }
    
    // Adjuntar el segmento
    SharedMemory* shm = (SharedMemory*) shmat(shmid, NULL, 0);
    if (shm == (void*) -1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        return NULL;
    }
    
    return shm;
}

// Desadjuntar memoria compartida
int detach_shared_memory(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt falló: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

// Eliminar memoria compartida (solo el finalizador debe hacer esto)
int cleanup_shared_memory(SharedMemory* shm) {
    key_t key = SHM_BASE_KEY;
    
    // Obtener el ID del segmento
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        return ERROR;
    }
    
    // Desadjuntar primero
    detach_shared_memory(shm);
    
    // Eliminar el segmento
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo eliminar memoria compartida: %s\n" RESET, 
                strerror(errno));
        return ERROR;
    }
    
    return SUCCESS;
}

// Obtener puntero al buffer de caracteres
CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}

// Obtener puntero a los datos del archivo
unsigned char* get_file_data_pointer(SharedMemory* shm) {
    return (unsigned char*)((char*)shm + shm->file_data_offset);
}