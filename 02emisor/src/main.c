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

volatile sig_atomic_t should_terminate = 0;
SharedMemory* g_shm = NULL;
sem_t* g_sem_global = NULL;
sem_t* g_sem_encrypt_queue = NULL;
sem_t* g_sem_decrypt_queue = NULL;
sem_t* g_sem_encrypt_spaces = NULL;
sem_t* g_sem_decrypt_items = NULL;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGUSR1) {
        should_terminate = 1;
    }
}

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

int validate_arguments(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, RED "[ERROR] Argumentos incorrectos\n" RESET);
        fprintf(stderr, "Uso: %s <auto|manual> [clave_hex] [delay_ms]\n", argv[0]);
        return ERROR;
    }
    return SUCCESS;
}

int parse_mode(const char* mode_str) {
    if (strcmp(mode_str, "auto") == 0) return MODE_AUTO;
    if (strcmp(mode_str, "manual") == 0) return MODE_MANUAL;
    return ERROR;
}

unsigned char parse_encryption_key(const char* key_str) {
    if (!key_str || strlen(key_str) != 2) return 0;
    unsigned char key = 0;
    sscanf(key_str, "%2hhx", &key);
    return key;
}

int main(int argc, char* argv[]) {
    if (validate_arguments(argc, argv) == ERROR) return EXIT_FAILURE;
    
    int mode = parse_mode(argv[1]);
    if (mode == ERROR) {
        fprintf(stderr, RED "[ERROR] Modo inválido\n" RESET);
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
        if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) delay_ms = DEFAULT_DELAY_MS;
    }
    
    setup_signal_handlers();
    print_emisor_banner();
    
    printf(CYAN "[EMISOR] Conectando a memoria compartida...\n" RESET);
    SharedMemory* shm = attach_shared_memory(SHM_BASE_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a SHM\n" RESET);
        return EXIT_FAILURE;
    }
    g_shm = shm;
    
    unsigned char encryption_key = has_custom_key ? custom_key : shm->encryption_key;
    
    printf(GREEN "✓ Conectado a memoria compartida\n" RESET);
    printf("  • Buffer size: %d slots\n", shm->buffer_size);
    printf("  • Archivo: %s (%d caracteres)\n", shm->input_filename, shm->total_chars_in_file);
    printf("  • Clave: 0x%02X\n", encryption_key);
    printf("  • Modo: %s\n", mode == MODE_AUTO ? "AUTOMÁTICO" : "MANUAL");
    if (mode == MODE_AUTO) printf("  • Delay: %d ms\n", delay_ms);
    
    printf(CYAN "\n[EMISOR] Abriendo semáforos POSIX...\n" RESET);
    
    g_sem_global = sem_open(SEM_NAME_GLOBAL_MUTEX, 0);
    g_sem_encrypt_queue = sem_open(SEM_NAME_ENCRYPT_QUEUE, 0);
    g_sem_decrypt_queue = sem_open(SEM_NAME_DECRYPT_QUEUE, 0);
    g_sem_encrypt_spaces = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    g_sem_decrypt_items = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);
    
    if (g_sem_global == SEM_FAILED || g_sem_encrypt_queue == SEM_FAILED ||
        g_sem_decrypt_queue == SEM_FAILED || g_sem_encrypt_spaces == SEM_FAILED ||
        g_sem_decrypt_items == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudieron abrir semáforos\n" RESET);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    
    printf(GREEN "✓ Semáforos abiertos\n" RESET);
    
    pid_t my_pid = getpid();
    register_emisor(shm, my_pid, g_sem_global);
    
    printf(BOLD GREEN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║              EMISOR PID %6d INICIADO                 ║\n" RESET, my_pid);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
    
    int chars_sent = 0;
    time_t start_time = time(NULL);
    
    while (!should_terminate && !shm->shutdown_flag) {
        if (sem_wait(g_sem_encrypt_spaces) != 0) {
            if (errno == EINTR) {
                if (should_terminate || shm->shutdown_flag) break;
                continue;
            }
            break;
        }

        sem_wait(g_sem_encrypt_queue);
        int slot_index = dequeue_encrypt_slot(shm);
        sem_post(g_sem_encrypt_queue);

        if (slot_index < 0) {
            sem_post(g_sem_encrypt_spaces);
            continue;
        }

        int txt_index = get_next_text_index(shm, g_sem_global);
        if (txt_index >= shm->total_chars_in_file) {
            sem_wait(g_sem_encrypt_queue);
            enqueue_encrypt_slot(shm, slot_index);
            sem_post(g_sem_encrypt_queue);
            sem_post(g_sem_encrypt_spaces);
            printf(YELLOW "\n[EMISOR %d] Fin del archivo alcanzado\n" RESET, getpid());
            break;
        }

        char original_char = read_char_at_position(shm, txt_index);
        unsigned char encrypted = encrypt_character(original_char, encryption_key);
        store_character(shm, slot_index, encrypted, txt_index, my_pid);

        sem_wait(g_sem_decrypt_queue);
        enqueue_decrypt_slot(shm, slot_index, txt_index);
        sem_post(g_sem_decrypt_queue);
        sem_post(g_sem_decrypt_items);

        print_emission_status(shm, slot_index, original_char, encrypted, txt_index);
        chars_sent++;

        if (mode == MODE_MANUAL) {
            printf(CYAN "\nPresione ENTER..." RESET);
            char buffer[10];
            errno = 0;
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                if (errno == EINTR || should_terminate || shm->shutdown_flag) break;
            }
        }
    }
    
    time_t end_time = time(NULL);
    
    // NUEVO: Guardar estadísticas antes de desregistrar
    save_emisor_stats(shm, my_pid, chars_sent, start_time, end_time, g_sem_global);
    
    printf(BOLD YELLOW "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD YELLOW "║             EMISOR PID %6d FINALIZANDO               ║\n" RESET, my_pid);
    printf(BOLD YELLOW "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("  • Caracteres enviados: %d\n", chars_sent);
    printf("  • Tiempo: %d segundos\n", (int)(end_time - start_time));
    
    unregister_emisor(shm, my_pid, g_sem_global);
    
    sem_close(g_sem_global);
    sem_close(g_sem_encrypt_queue);
    sem_close(g_sem_decrypt_queue);
    sem_close(g_sem_encrypt_spaces);
    sem_close(g_sem_decrypt_items);
    
    detach_shared_memory(shm);
    
    printf(GREEN "\n[EMISOR %d] Proceso terminado\n" RESET, my_pid);
    
    return EXIT_SUCCESS;
}