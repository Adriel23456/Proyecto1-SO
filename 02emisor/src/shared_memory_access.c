#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include "shared_memory_access.h"
#include "constants.h"
#include "structures.h"

/*
 * Conectar a memoria compartida existente.
 */
SharedMemory* attach_shared_memory(key_t key) {
    // Obtener el ID del segmento existente
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
    
    // Verificar integridad básica
    if (shm->buffer_size <= 0 || shm->file_data_size <= 0) {
        fprintf(stderr, RED "[ERROR] Memoria compartida parece estar corrupta\n" RESET);
        shmdt(shm);
        return NULL;
    }
    
    return shm;
}

/*
 * Desconectar de memoria compartida.
 */
int detach_shared_memory(SharedMemory* shm) {
    if (shm == NULL) return SUCCESS;
    
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt falló: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

/*
 * Obtener puntero al buffer de caracteres.
 */
CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    if (shm == NULL) return NULL;
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}

/*
 * Obtener puntero a los datos del archivo.
 */
unsigned char* get_file_data_pointer(SharedMemory* shm) {
    if (shm == NULL) return NULL;
    return (unsigned char*)((char*)shm + shm->file_data_offset);
}

/*
 * Leer un carácter del archivo en la posición especificada.
 */
char read_char_at_position(SharedMemory* shm, int position) {
    if (shm == NULL) return '\0';
    if (position < 0 || position >= shm->file_data_size) return '\0';
    
    unsigned char* file_data = get_file_data_pointer(shm);
    return (char)file_data[position];
}

/*
 * Almacenar un carácter encriptado en un slot del buffer.
 */
void store_character(SharedMemory* shm, int slot_index, unsigned char encrypted_char, 
                     int text_index, pid_t emisor_pid) {
    if (shm == NULL || slot_index < 0 || slot_index >= shm->buffer_size) return;
    
    CharacterSlot* buffer = get_buffer_pointer(shm);
    CharacterSlot* slot = &buffer[slot_index];
    
    slot->ascii_value = encrypted_char;
    slot->slot_index = slot_index + 1;  // Índice 1-based para display
    slot->timestamp = time(NULL);
    slot->is_valid = 1;
    slot->text_index = text_index;
    slot->emisor_pid = emisor_pid;
}

/*
 * Obtener información de un slot del buffer.
 */
int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* slot_info) {
    if (shm == NULL || slot_index < 0 || slot_index >= shm->buffer_size || slot_info == NULL) {
        return ERROR;
    }
    
    CharacterSlot* buffer = get_buffer_pointer(shm);
    *slot_info = buffer[slot_index];
    return SUCCESS;
}

/*
 * Verificar el estado de la memoria compartida.
 */
int check_shared_memory_health(SharedMemory* shm) {
    if (shm == NULL) return ERROR;
    
    // Verificar campos críticos
    if (shm->buffer_size <= 0 || shm->buffer_size > 100000000) {
        fprintf(stderr, RED "[WARNING] Buffer size anormal: %d\n" RESET, shm->buffer_size);
        return ERROR;
    }
    
    if (shm->file_data_size <= 0) {
        fprintf(stderr, RED "[WARNING] File data size inválido: %d\n" RESET, shm->file_data_size);
        return ERROR;
    }
    
    if (shm->shutdown_flag) {
        printf(YELLOW "[INFO] Sistema en proceso de apagado\n" RESET);
    }
    
    return SUCCESS;
}

/*
 * Mostrar información del estado de la memoria compartida.
 */
void print_shared_memory_status(SharedMemory* shm) {
    if (shm == NULL) return;
    
    printf(CYAN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(CYAN "║              ESTADO DE MEMORIA COMPARTIDA               ║\n" RESET);
    printf(CYAN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    
    printf("  • Buffer size: %d slots\n", shm->buffer_size);
    printf("  • Archivo: %s\n", shm->input_filename);
    printf("  • Tamaño archivo: %d bytes\n", shm->file_data_size);
    printf("  • Índice actual: %d/%d\n", shm->current_txt_index, shm->total_chars_in_file);
    printf("  • Caracteres procesados: %d\n", shm->total_chars_processed);
    printf("  • Emisores activos: %d (total: %d)\n", shm->active_emisores, shm->total_emisores);
    printf("  • Receptores activos: %d (total: %d)\n", shm->active_receptores, shm->total_receptores);
    printf("  • Estado: %s\n", shm->shutdown_flag ? "CERRANDO" : "ACTIVO");
    
    // Estado de las colas
    printf("\n  Colas:\n");
    printf("  • Encrypt queue: %d/%d slots libres\n", 
           shm->encrypt_queue.size, shm->encrypt_queue.capacity);
    printf("  • Decrypt queue: %d/%d elementos con datos\n",
           shm->decrypt_queue.size, shm->decrypt_queue.capacity);
}