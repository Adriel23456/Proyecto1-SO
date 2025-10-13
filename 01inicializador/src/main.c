#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "constants.h"
#include "structures.h"
#include "shared_memory_init.h"
#include "queue_manager.h"
#include "file_processor.h"
#include "semaphore_init.h"

/*
 * Banner principal del programa.
 */
static void print_banner() {
    printf(BOLD GREEN "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║           INICIALIZADOR DE MEMORIA COMPARTIDA            ║\n" RESET);
    printf(BOLD GREEN "║         Sistema de Comunicación entre Procesos           ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

/*
 * Validación de argumentos.
 */
static int validate_arguments(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, RED "[ERROR] Número incorrecto de argumentos\n" RESET);
        fprintf(stderr, "Uso: %s <archivo_entrada> <tamaño_buffer> <clave_encriptación>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s assets/data.txt 500 AA\n", argv[0]);
        return ERROR;
    }

    if (access(argv[1], F_OK) == -1) {
        fprintf(stderr, RED "[ERROR] El archivo '%s' no existe\n" RESET, argv[1]);
        return ERROR;
    }

    long bs = strtol(argv[2], NULL, 10);
    if (bs < MIN_BUFFER_SIZE || bs > INT_MAX) {
        fprintf(stderr, RED "[ERROR] Tamaño de buffer inválido (>= %d requerido)\n" RESET, MIN_BUFFER_SIZE);
        return ERROR;
    }

    if (strlen(argv[3]) != 2) {
        fprintf(stderr, RED "[ERROR] La clave debe ser hexadecimal de 2 caracteres (ej: AA)\n" RESET);
        return ERROR;
    }
    return SUCCESS;
}

/*
 * Parseo de clave hex.
 */
static unsigned char parse_encryption_key(const char* key_str) {
    unsigned char key = 0;
    (void)sscanf(key_str, "%2hhx", &key);
    return key;
}

int main(int argc, char* argv[]) {
    print_banner();

    if (validate_arguments(argc, argv) == ERROR) {
        return EXIT_FAILURE;
    }

    char* input_filename = argv[1];
    int buffer_size = atoi(argv[2]);
    unsigned char encryption_key = parse_encryption_key(argv[3]);

    printf(CYAN "[INFO] Parámetros de inicialización:\n" RESET);
    printf("  • Archivo de entrada: %s\n", input_filename);
    printf("  • Tamaño del buffer: %d slots\n", buffer_size);
    printf("  • Clave de encriptación: 0x%02X (binario: ", encryption_key);
    for (int i = 7; i >= 0; i--) printf("%d", (encryption_key >> i) & 1);
    printf(")\n\n");

    // Paso 1: leer archivo y generar .bin
    printf(YELLOW "[PASO 1] Procesando archivo de entrada...\n" RESET);
    size_t file_size = 0;
    unsigned char* file_data = process_input_file(input_filename, &file_size);
    if (!file_data) {
        fprintf(stderr, RED "[ERROR] No se pudo procesar el archivo de entrada\n" RESET);
        return EXIT_FAILURE;
    }
    printf(GREEN "  ✓ Archivo procesado: %zu bytes leídos\n" RESET, file_size);
    printf(GREEN "  ✓ Archivo binario generado: %s.bin\n" RESET, input_filename);

    if (file_size > (size_t)INT_MAX) {
        fprintf(stderr, RED "[ERROR] Archivo demasiado grande para parámetros actuales (%zu bytes)\n" RESET, file_size);
        free(file_data);
        return EXIT_FAILURE;
    }

    // Paso 2: crear SHM con todas las regiones necesarias
    printf(YELLOW "\n[PASO 2] Creando memoria compartida...\n" RESET);
    SharedMemory* shm = create_shared_memory(buffer_size, (int)file_size);
    if (!shm) {
        free(file_data);
        return EXIT_FAILURE;
    }

    printf(GREEN "  ✓ Memoria compartida creada\n" RESET);
    printf("  • ID de memoria: 0x%04X\n", SHM_BASE_KEY);
    printf("  • Tamaño total (aprox.): %zu bytes\n",
           (size_t)sizeof(SharedMemory)
         + (size_t)buffer_size * sizeof(CharacterSlot)
         + (size_t)file_size
         + (size_t)buffer_size * sizeof(int) * 2 /* SlotRef estimado: 2 ints */
    );

    // Paso 3: inicialización de metadatos
    printf(YELLOW "\n[PASO 3] Inicializando estructura de memoria compartida...\n" RESET);
    shm->shm_id                 = SHM_BASE_KEY;
    shm->buffer_size            = buffer_size;
    shm->encryption_key         = encryption_key;
    shm->current_txt_index      = 0;
    shm->total_chars_in_file    = (int)file_size;
    shm->total_chars_processed  = 0;
    shm->total_emisores         = 0;
    shm->active_emisores        = 0;
    shm->total_receptores       = 0;
    shm->active_receptores      = 0;
    shm->shutdown_flag          = 0;
    strncpy(shm->input_filename, input_filename, sizeof(shm->input_filename) - 1);
    shm->input_filename[sizeof(shm->input_filename) - 1] = '\0';
    shm->file_data_size         = (int)file_size;
    shm->emisor_stats_count = 0;
    shm->receptor_stats_count = 0;
    memset(shm->emisor_stats, 0, sizeof(shm->emisor_stats));
    memset(shm->receptor_stats, 0, sizeof(shm->receptor_stats));
    printf(GREEN "  ✓ Estructura inicializada\n" RESET);

    // Paso 4: slots del buffer
    printf(YELLOW "\n[PASO 4] Inicializando buffer de caracteres...\n" RESET);
    initialize_buffer_slots(shm, buffer_size);
    printf(GREEN "  ✓ %d slots de caracteres inicializados\n" RESET, buffer_size);

    // Paso 5: datos del archivo dentro de SHM
    printf(YELLOW "\n[PASO 5] Copiando datos del archivo a memoria compartida...\n" RESET);
    copy_file_to_shared_memory(shm, file_data, (int)file_size);
    printf(GREEN "  ✓ Datos del archivo copiados a memoria compartida\n" RESET);

    // Paso 6: colas
    printf(YELLOW "\n[PASO 6] Inicializando colas de sincronización...\n" RESET);
    initialize_queues(shm, buffer_size);
    printf(GREEN "  ✓ Cola de encriptación inicializada con %d posiciones\n" RESET, buffer_size);
    printf(GREEN "  ✓ Cola de desencriptación inicializada (vacía)\n" RESET);

    // Paso 7: semáforos POSIX
    printf(YELLOW "\n[PASO 7] Inicializando semáforos POSIX...\n" RESET);
    if (initialize_semaphores(buffer_size) == ERROR) {
        fprintf(stderr, RED "[ERROR] No se pudieron inicializar los semáforos POSIX\n" RESET);
        cleanup_shared_memory(shm);
        free(file_data);
        return EXIT_FAILURE;
    }

    printf(GREEN "  ✓ Semáforos POSIX creados e inicializados\n" RESET);
    printf("  • %s = 1\n",  SEM_NAME_GLOBAL_MUTEX);
    printf("  • %s = 1\n",  SEM_NAME_ENCRYPT_QUEUE);
    printf("  • %s = 1\n",  SEM_NAME_DECRYPT_QUEUE);
    printf("  • %s = %d\n", SEM_NAME_ENCRYPT_SPACES, buffer_size);
    printf("  • %s = 0\n",  SEM_NAME_DECRYPT_ITEMS);

    // Resumen
    printf(BOLD GREEN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║              INICIALIZACIÓN COMPLETADA                   ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);

    printf(WHITE "\nResumen del sistema:\n" RESET);
    printf("  • Memoria compartida ID: 0x%04X\n", SHM_BASE_KEY);
    printf("  • Buffer circular: %d slots\n", buffer_size);
    printf("  • Archivo fuente: %s (%zu bytes)\n", input_filename, file_size);
    printf("  • Clave XOR: 0x%02X\n", encryption_key);
    printf("  • Semáforos POSIX: %s, %s, %s, %s, %s\n",
           SEM_NAME_GLOBAL_MUTEX, SEM_NAME_ENCRYPT_QUEUE, SEM_NAME_DECRYPT_QUEUE,
           SEM_NAME_ENCRYPT_SPACES, SEM_NAME_DECRYPT_ITEMS);

    printf(CYAN "\n[INFO] El sistema está listo para recibir emisores y receptores\n" RESET);
    printf(CYAN "[INFO] Use los siguientes comandos para iniciar los procesos:\n" RESET);
    printf("  • Emisor:      ./emisor auto|manual [clave]\n");
    printf("  • Receptor:    ./receptor auto|manual [clave]\n");
    printf("  • Finalizador: ./finalizador\n");

    printf(MAGENTA "\n[INICIALIZADOR] Proceso terminando exitosamente...\n" RESET);

    // Limpieza local del buffer del archivo (la SHM permanece)
    free(file_data);
    return EXIT_SUCCESS;
}
