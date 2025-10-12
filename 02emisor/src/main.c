#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "constants.h"
#include "structures.h"
#include "shared_memory_access.h"
#include "queue_operations.h"
#include "encoder.h"
#include "process_manager.h"
#include "display.h"

// Variables globales para manejo de señales
volatile sig_atomic_t should_terminate = 0;
SharedMemory* g_shm = NULL;
sem_t* g_sem_global = NULL;
sem_t* g_sem_encrypt_queue = NULL;
sem_t* g_sem_decrypt_queue = NULL;
sem_t* g_sem_encrypt_spaces = NULL;
sem_t* g_sem_decrypt_items = NULL;

// Manejador de señales
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGUSR1) {
        should_terminate = 1;
        // NO hacer sem_post(g_sem_encrypt_spaces);
        // sem_wait se interrumpe con EINTR, y ya lo manejas.
    }
}

// Configurar manejadores de señales
void setup_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
}

// Validar argumentos
int validate_arguments(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, RED "[ERROR] Argumentos incorrectos\n" RESET);
        fprintf(stderr, "Uso: %s <auto|manual> [clave_hex] [delay_ms]\n", argv[0]);
        fprintf(stderr, "Ejemplos:\n");
        fprintf(stderr, "  %s auto          # Usa clave de SHM, delay 100ms\n", argv[0]);
        fprintf(stderr, "  %s auto FF       # Usa clave FF, delay 100ms\n", argv[0]);
        fprintf(stderr, "  %s auto FF 50    # Usa clave FF, delay 50ms\n", argv[0]);
        fprintf(stderr, "  %s manual        # Modo manual\n", argv[0]);
        return ERROR;
    }
    return SUCCESS;
}

// Parsear modo de ejecución
int parse_mode(const char* mode_str) {
    if (strcmp(mode_str, "auto") == 0) return MODE_AUTO;
    if (strcmp(mode_str, "manual") == 0) return MODE_MANUAL;
    return ERROR;
}

// Parsear clave de encriptación
unsigned char parse_encryption_key(const char* key_str) {
    if (!key_str || strlen(key_str) != 2) return 0;
    unsigned char key = 0;
    sscanf(key_str, "%2hhx", &key);
    return key;
}

// Función principal del emisor
int main(int argc, char* argv[]) {
    // Validar argumentos
    if (validate_arguments(argc, argv) == ERROR) {
        return EXIT_FAILURE;
    }
    
    // Parsear parámetros
    int mode = parse_mode(argv[1]);
    if (mode == ERROR) {
        fprintf(stderr, RED "[ERROR] Modo inválido. Use 'auto' o 'manual'\n" RESET);
        return EXIT_FAILURE;
    }
    
    unsigned char custom_key = 0;
    int has_custom_key = 0;
    if (argc >= 3) {
        custom_key = parse_encryption_key(argv[2]);
        has_custom_key = 1;
    }
    
    int delay_ms = DEFAULT_DELAY_MS;
    if (argc >= 4 && mode == MODE_AUTO) {
        delay_ms = atoi(argv[3]);
        if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) {
            delay_ms = DEFAULT_DELAY_MS;
        }
    }
    
    // Configurar señales
    setup_signal_handlers();
    
    // Banner del emisor
    print_emisor_banner();
    
    // Conectar a memoria compartida
    printf(CYAN "[EMISOR] Conectando a memoria compartida...\n" RESET);
    SharedMemory* shm = attach_shared_memory(SHM_BASE_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a memoria compartida\n" RESET);
        fprintf(stderr, "Asegúrese de ejecutar primero el inicializador\n");
        return EXIT_FAILURE;
    }
    g_shm = shm;
    
    // Determinar clave de encriptación
    unsigned char encryption_key = has_custom_key ? custom_key : shm->encryption_key;
    
    printf(GREEN "✓ Conectado a memoria compartida\n" RESET);
    printf("  • Buffer size: %d slots\n", shm->buffer_size);
    printf("  • Archivo: %s (%d caracteres)\n", shm->input_filename, shm->total_chars_in_file);
    printf("  • Clave de encriptación: 0x%02X\n", encryption_key);
    printf("  • Modo: %s\n", mode == MODE_AUTO ? "AUTOMÁTICO" : "MANUAL");
    if (mode == MODE_AUTO) {
        printf("  • Delay: %d ms\n", delay_ms);
    }
    
    // Abrir semáforos POSIX nombrados
    printf(CYAN "\n[EMISOR] Abriendo semáforos POSIX...\n" RESET);
    
    sem_t* sem_global = sem_open(SEM_NAME_GLOBAL_MUTEX, 0);
    if (sem_global == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir %s: %s\n" RESET, 
                SEM_NAME_GLOBAL_MUTEX, strerror(errno));
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    g_sem_global = sem_global;
    
    sem_t* sem_encrypt_queue = sem_open(SEM_NAME_ENCRYPT_QUEUE, 0);
    if (sem_encrypt_queue == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir %s: %s\n" RESET,
                SEM_NAME_ENCRYPT_QUEUE, strerror(errno));
        sem_close(sem_global);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    g_sem_encrypt_queue = sem_encrypt_queue;
    
    sem_t* sem_decrypt_queue = sem_open(SEM_NAME_DECRYPT_QUEUE, 0);
    if (sem_decrypt_queue == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir %s: %s\n" RESET,
                SEM_NAME_DECRYPT_QUEUE, strerror(errno));
        sem_close(sem_global);
        sem_close(sem_encrypt_queue);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    g_sem_decrypt_queue = sem_decrypt_queue;
    
    sem_t* sem_encrypt_spaces = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    if (sem_encrypt_spaces == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir %s: %s\n" RESET,
                SEM_NAME_ENCRYPT_SPACES, strerror(errno));
        sem_close(sem_global);
        sem_close(sem_encrypt_queue);
        sem_close(sem_decrypt_queue);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    g_sem_encrypt_spaces = sem_encrypt_spaces;
    
    sem_t* sem_decrypt_items = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);
    if (sem_decrypt_items == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir %s: %s\n" RESET,
                SEM_NAME_DECRYPT_ITEMS, strerror(errno));
        sem_close(sem_global);
        sem_close(sem_encrypt_queue);
        sem_close(sem_decrypt_queue);
        sem_close(sem_encrypt_spaces);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    g_sem_decrypt_items = sem_decrypt_items;
    
    printf(GREEN "✓ Semáforos POSIX abiertos correctamente\n" RESET);
    
    // Registrar emisor
    pid_t my_pid = getpid();
    register_emisor(shm, my_pid, sem_global);
    
    printf(BOLD GREEN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║              EMISOR PID %6d INICIADO                 ║\n" RESET, my_pid);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
    
    // Variables para estadísticas
    int chars_sent = 0;
    time_t start_time = time(NULL);
    
    // Bucle principal de emisión - SIN BUSY WAITING
    while (!should_terminate && !shm->shutdown_flag) {

        // 1) Esperar espacio disponible (bloquea sin busy waiting)
        if (sem_wait(sem_encrypt_spaces) != 0) {
            if (errno == EINTR) {
                if (should_terminate || shm->shutdown_flag) break;
                continue; // reintentar
            }
            fprintf(stderr, RED "[ERROR] sem_wait(encrypt_spaces): %s\n" RESET, strerror(errno));
            break;
        }

        // 2) Tomar un slot libre de la cola (sección crítica)
        sem_wait(sem_encrypt_queue);
        int slot_index = dequeue_encrypt_slot(shm);
        sem_post(sem_encrypt_queue);

        if (slot_index < 0) {
            // Inconsistencia: el semáforo dijo que había espacio pero la cola está vacía.
            // Devolver el permiso y volver a bloquear. NO avances índice.
            sem_post(sem_encrypt_spaces);
            // (opcional) pequeño backoff si quieres evitar spam de logs
            // struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000}; nanosleep(&ts, NULL);
            continue;
        }

        // 3) Ahora sí, tomar el siguiente índice del texto (esto incrementa el puntero global)
        int txt_index = get_next_text_index(shm, g_sem_global);

        // Si ya no hay más caracteres, devolvemos el slot y el permiso.
        if (txt_index >= shm->total_chars_in_file) {
            sem_wait(sem_encrypt_queue);
            enqueue_encrypt_slot(shm, slot_index);
            sem_post(sem_encrypt_queue);
            sem_post(sem_encrypt_spaces);

            printf(YELLOW "\n[EMISOR %d] Fin del archivo alcanzado\n" RESET, getpid());
            break;
        }

        // 4) Leer/encriptar/almacenar
        char original_char = read_char_at_position(shm, txt_index);
        unsigned char encrypted = encrypt_character(original_char, encryption_key);
        store_character(shm, slot_index, encrypted, txt_index, my_pid);

        // 5) Pasar a la cola de desencriptación + avisar item disponible
        sem_wait(sem_decrypt_queue);
        enqueue_decrypt_slot(shm, slot_index, txt_index);
        sem_post(sem_decrypt_queue);
        sem_post(sem_decrypt_items);

        // 6) Display y contadores locales
        print_emission_status(shm, slot_index, original_char, encrypted, txt_index);
        chars_sent++;

        // 7) Control de modo
        if (mode == MODE_MANUAL) {
            printf(CYAN "\nPresione ENTER para continuar (o Ctrl+C para salir)..." RESET);
            char buffer[10];
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        } else {
            usleep(delay_ms * 1000);
        }
    }
    
    // Estadísticas finales
    time_t end_time = time(NULL);
    int elapsed = (int)(end_time - start_time);
    
    printf(BOLD YELLOW "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD YELLOW "║             EMISOR PID %6d FINALIZANDO               ║\n" RESET, my_pid);
    printf(BOLD YELLOW "╚══════════════════════════════════════════════════════════╝\n" RESET);
    
    printf(WHITE "\nEstadísticas del emisor:\n" RESET);
    printf("  • Caracteres enviados: %d\n", chars_sent);
    printf("  • Tiempo de ejecución: %d segundos\n", elapsed);
    if (elapsed > 0) {
        printf("  • Velocidad promedio: %.2f chars/segundo\n", (float)chars_sent / elapsed);
    }
    
    // Desregistrar emisor
    unregister_emisor(shm, my_pid, sem_global);
    
    // Cerrar semáforos (no eliminarlos, solo cerrar el handle local)
    sem_close(sem_global);
    sem_close(sem_encrypt_queue);
    sem_close(sem_decrypt_queue);
    sem_close(sem_encrypt_spaces);
    sem_close(sem_decrypt_items);
    
    // Desconectar de memoria compartida
    detach_shared_memory(shm);
    
    printf(GREEN "\n[EMISOR %d] Proceso terminado correctamente\n" RESET, my_pid);
    
    return EXIT_SUCCESS;
}