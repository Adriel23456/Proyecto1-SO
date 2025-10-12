#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // sysconf
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <limits.h>
#include "shared_memory_init.h"
#include "constants.h"
#include "structures.h"

/*
 * Lectura del tamaño máximo de segmento de memoria compartida (System V).
 * Se consulta /proc/sys/kernel/shmmax; si no es accesible, se asume un valor muy alto
 * y se delega el chequeo a shmget.
 */
static unsigned long long read_shmmax_bytes(void) {
    FILE* f = fopen("/proc/sys/kernel/shmmax", "r");
    if (!f) return ULLONG_MAX; // Mejor deferir al kernel si no se puede leer
    unsigned long long v = 0ULL;
    if (fscanf(f, "%llu", &v) != 1) {
        fclose(f);
        return ULLONG_MAX;
    }
    fclose(f);
    return v;
}

/*
 * Cálculo del tamaño total a reservar en el segmento de SHM:
 *  - estructura base
 *  - buffer de CharacterSlot[buffer_size]
 *  - datos del archivo file_data[file_size]
 *  - arrays de colas: 2 * SlotRef[buffer_size]
 * Se alinea al tamaño de página reportado por el sistema.
 */
static size_t compute_total_size_aligned(int buffer_size, int file_size,
                                         size_t* base_size_out,
                                         size_t* buffer_bytes_out,
                                         size_t* file_bytes_out,
                                         size_t* enc_queue_bytes_out,
                                         size_t* dec_queue_bytes_out,
                                         size_t* page_size_out) {
    size_t base_size        = sizeof(SharedMemory);
    size_t buffer_bytes     = (size_t)buffer_size * sizeof(CharacterSlot);
    size_t file_bytes       = (size_t)file_size;
    size_t enc_queue_bytes  = (size_t)buffer_size * sizeof(SlotRef);
    size_t dec_queue_bytes  = (size_t)buffer_size * sizeof(SlotRef);

    long pg = sysconf(_SC_PAGESIZE);
    size_t page_size = (pg > 0) ? (size_t)pg : (size_t)PAGE_SIZE;

    size_t total = base_size
                 + buffer_bytes
                 + file_bytes
                 + enc_queue_bytes
                 + dec_queue_bytes;

    size_t aligned = ((total + page_size - 1) / page_size) * page_size;

    if (base_size_out)       *base_size_out        = base_size;
    if (buffer_bytes_out)    *buffer_bytes_out     = buffer_bytes;
    if (file_bytes_out)      *file_bytes_out       = file_bytes;
    if (enc_queue_bytes_out) *enc_queue_bytes_out  = enc_queue_bytes;
    if (dec_queue_bytes_out) *dec_queue_bytes_out  = dec_queue_bytes;
    if (page_size_out)       *page_size_out        = page_size;

    return aligned;
}

/*
 * Creación del segmento de memoria compartida con todas las regiones necesarias.
 * También configura los offsets y capacidades de las colas para el uso posterior.
 */
SharedMemory* create_shared_memory(int buffer_size, int file_size) {
    key_t key = SHM_BASE_KEY;

    // Cálculo de tamaños y alineación
    size_t base_size, buffer_bytes, file_bytes, enc_q_bytes, dec_q_bytes, page_sz;
    size_t total_size = compute_total_size_aligned(buffer_size, file_size,
                                                   &base_size, &buffer_bytes, &file_bytes,
                                                   &enc_q_bytes, &dec_q_bytes, &page_sz);

    printf("  • Tamaño base de estructura: %zu bytes\n", base_size);
    printf("  • Tamaño del buffer: %zu bytes (%d slots)\n", buffer_bytes, buffer_size);
    printf("  • Tamaño de datos del archivo: %d bytes\n", file_size);
    printf("  • Tamaño arrays de colas: %zu + %zu bytes\n", enc_q_bytes, dec_q_bytes);
    printf("  • Tamaño total alineado: %zu bytes\n", total_size);

    // Validación contra shmmax
    unsigned long long shmmax = read_shmmax_bytes();
    if ((unsigned long long)total_size > shmmax) {
        fprintf(stderr, RED "[ERROR] El tamaño requerido (%zu) excede shmmax (%llu). "
                            "Aumente /proc/sys/kernel/shmmax o reduzca parámetros.\n" RESET,
                total_size, shmmax);
        return NULL;
    }

    // Intentar eliminar memoria previa con la misma key
    int old_shmid = shmget(key, 0, 0);
    if (old_shmid != -1) {
        printf(YELLOW "  ! Memoria compartida existente detectada, eliminando...\n" RESET);
        if (shmctl(old_shmid, IPC_RMID, NULL) == -1) {
            fprintf(stderr, RED "  [ADVERTENCIA] No se pudo eliminar memoria previa\n" RESET);
        }
    }

    // Crear el segmento
    int shmid = shmget(key, total_size, IPC_CREAT | IPC_EXCL | IPC_PERMS);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] shmget falló: %s\n" RESET, strerror(errno));
        return NULL;
    }
    printf("  • ID de segmento: %d\n", shmid);

    // Adjuntar a nuestro espacio de direcciones
    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        shmctl(shmid, IPC_RMID, NULL);
        return NULL;
    }

    // Inicializar en cero todo el segmento
    memset(shm, 0, total_size);

    // Configurar offsets y capacidades (orden físico):
    // [SharedMemory][CharacterSlot buffer][file_data][enc_queue_array][dec_queue_array]
    shm->buffer_offset = sizeof(SharedMemory);
    shm->file_data_offset = shm->buffer_offset + buffer_bytes;

    shm->encrypt_queue.capacity   = buffer_size;
    shm->encrypt_queue.array_offset = shm->file_data_offset + file_bytes;

    shm->decrypt_queue.capacity   = buffer_size;
    shm->decrypt_queue.array_offset = shm->encrypt_queue.array_offset + enc_q_bytes;

    return shm;
}

/*
 * Inicialización de los slots del buffer de caracteres.
 */
void initialize_buffer_slots(SharedMemory* shm, int buffer_size) {
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);

    for (int i = 0; i < buffer_size; i++) {
        buffer[i].ascii_value = 0;
        buffer[i].slot_index  = i + 1;  // Índices de 1..N
        buffer[i].timestamp   = 0;
        buffer[i].is_valid    = 0;
        buffer[i].text_index  = -1;
        buffer[i].emisor_pid  = 0;
    }

    printf("  • Slots inicializados:\n");
    int preview = (buffer_size < 3) ? buffer_size : 3;
    for (int i = 0; i < preview; i++) {
        printf("    - Slot %d: índice=%d, vacío\n", i, buffer[i].slot_index);
    }
    if (buffer_size > 3) {
        printf("    ... y %d más\n", buffer_size - 3);
    }
}

/*
 * Copiar los datos del archivo al bloque file_data mapeado en SHM.
 */
void copy_file_to_shared_memory(SharedMemory* shm, unsigned char* file_data, int file_size) {
    unsigned char* shm_file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    memcpy(shm_file_data, file_data, (size_t)file_size);

    printf("  • Primeros bytes del archivo en memoria compartida:\n    ");
    int preview_size = MIN(20, file_size);
    for (int i = 0; i < preview_size; i++) {
        unsigned char c = shm_file_data[i];
        if (c >= 32 && c < 127) printf("%c", c);
        else printf("\\x%02x", c);
    }
    if (file_size > preview_size) printf("...");
    printf("\n");
}

/*
 * Adjuntar a memoria compartida existente.
 */
SharedMemory* attach_shared_memory(key_t key) {
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        fprintf(stderr, RED "[ERROR] No se encontró memoria compartida con key 0x%04X\n" RESET, key);
        return NULL;
    }

    SharedMemory* shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, RED "[ERROR] shmat falló: %s\n" RESET, strerror(errno));
        return NULL;
    }

    return shm;
}

/*
 * Desadjuntar memoria compartida.
 */
int detach_shared_memory(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        fprintf(stderr, RED "[ERROR] shmdt falló: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

/*
 * Eliminar memoria compartida (uso exclusivo del proceso finalizador).
 */
int cleanup_shared_memory(SharedMemory* shm) {
    key_t key = SHM_BASE_KEY;
    int shmid = shmget(key, 0, 0);
    if (shmid == -1) return ERROR;

    detach_shared_memory(shm);
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo eliminar memoria compartida: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    return SUCCESS;
}

/*
 * Accesos convenientes por offset.
 */
CharacterSlot* get_buffer_pointer(SharedMemory* shm) {
    return (CharacterSlot*)((char*)shm + shm->buffer_offset);
}
unsigned char* get_file_data_pointer(SharedMemory* shm) {
    return (unsigned char*)((char*)shm + shm->file_data_offset);
}
