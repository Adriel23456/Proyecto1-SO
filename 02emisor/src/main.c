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

static void print_usage(const char* argv0) {
    fprintf(stderr, "Uso:\n");
    fprintf(stderr, "  %s                      # auto, clave SHM, delay=0\n", argv0);
    fprintf(stderr, "  %s auto                # auto, clave SHM, delay=0\n", argv0);
    fprintf(stderr, "  %s manual              # manual, clave SHM\n", argv0);
    fprintf(stderr, "  %s auto <KEY>          # auto, clave=<KEY>, delay=0 (KEY=2 hex)\n", argv0);
    fprintf(stderr, "  %s manual <KEY>        # manual, clave=<KEY>\n", argv0);
    fprintf(stderr, "  %s auto <KEY> <MS>     # auto, clave=<KEY>, delay=<MS>\n", argv0);
    fprintf(stderr, "  %s auto <MS>           # auto, clave SHM, delay=<MS>\n", argv0);
    fprintf(stderr, "Notas:\n");
    fprintf(stderr, "  - <KEY> es 2 hex (ej: AA, ff)\n");
    fprintf(stderr, "  - <MS> es delay en milisegundos (0..%d)\n", MAX_DELAY_MS);
}

int validate_arguments(int argc, char* argv[]) {
    // Ahora permitimos de 1 a 4 argumentos totales (argv[0] + 0..3 adicionales)
    if (argc < 1 || argc > 4) {
        print_usage(argv[0]);
        return ERROR;
    }
    return SUCCESS;
}

int parse_mode(const char* mode_str) {
    if (!mode_str) return MODE_AUTO; // por defecto: auto
    if (strcmp(mode_str, "auto") == 0) return MODE_AUTO;
    if (strcmp(mode_str, "manual") == 0) return MODE_MANUAL;
    return ERROR;
}

static int is_two_hex_chars(const char* s, unsigned char* out_key) {
    if (!s || strlen(s) != 2) return 0;
    unsigned char k = 0;
    if (sscanf(s, "%2hhx", &k) == 1) {
        if (out_key) *out_key = k;
        return 1;
    }
    return 0;
}

static int parse_nonneg_int(const char* s, int* out) {
    if (!s || !*s) return 0;
    char* end = NULL;
    long v = strtol(s, &end, 10);
    if (*end != '\0') return 0;
    if (v < 0 || v > MAX_DELAY_MS) return 0;
    if (out) *out = (int)v;
    return 1;
}

unsigned char parse_encryption_key(const char* key_str) {
    if (!key_str || strlen(key_str) != 2) return 0;
    unsigned char key = 0;
    sscanf(key_str, "%2hhx", &key);
    return key;
}

int main(int argc, char* argv[]) {
    if (validate_arguments(argc, argv) == ERROR) return EXIT_FAILURE;
    
    // -------------------------------
    // Parsing flexible de argumentos
    // -------------------------------
    // Casos soportados:
    //   1) (sin args)                  -> auto,   key=SHM, delay=0
    //   2) auto                        -> auto,   key=SHM, delay=0
    //   3) manual                      -> manual, key=SHM
    //   4) auto <KEY>                  -> auto,   key=<KEY>, delay=0
    //   5) manual <KEY>                -> manual, key=<KEY>
    //   6) auto <KEY> <MS>             -> auto,   key=<KEY>, delay=<MS>
    //   7) auto <MS>                   -> auto,   key=SHM,   delay=<MS>

    int mode = MODE_AUTO;
    unsigned char custom_key = 0;
    int has_custom_key = 0;
    int delay_ms = DEFAULT_DELAY_MS;  // por defecto: 0 (sin slowdown)

    if (argc == 1) {
        // ./emisor
        mode = MODE_AUTO;
        has_custom_key = 0;
        delay_ms = 0;
    } else {
        // hay al menos un argumento de modo
        mode = parse_mode(argv[1]);
        if (mode == ERROR) {
            fprintf(stderr, RED "[ERROR] Modo inválido. Use 'auto' o 'manual'\n" RESET);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (argc == 2) {
            // ./emisor auto  |  ./emisor manual
            has_custom_key = 0;
            delay_ms = (mode == MODE_AUTO) ? 0 : 0; // manual ignora delay
        } else if (argc == 3) {
            // ./emisor auto X   (X puede ser KEY o MS)
            // ./emisor manual KEY
            if (mode == MODE_AUTO) {
                unsigned char k = 0;
                if (is_two_hex_chars(argv[2], &k)) {
                    custom_key = k;
                    has_custom_key = 1;
                    delay_ms = 0; // auto con KEY pero sin MS -> sin slowdown
                } else {
                    // Si no es KEY válido, intentamos MS (delay)
                    int d = 0;
                    if (!parse_nonneg_int(argv[2], &d)) {
                        fprintf(stderr, RED "[ERROR] Argumento inválido '%s'. Espere <KEY(hex2)> o <MS>\n" RESET, argv[2]);
                        print_usage(argv[0]);
                        return EXIT_FAILURE;
                    }
                    has_custom_key = 0; // clave desde SHM
                    delay_ms = d;       // auto con delay explícito
                }
            } else { // MODE_MANUAL
                unsigned char k = 0;
                if (!is_two_hex_chars(argv[2], &k)) {
                    fprintf(stderr, RED "[ERROR] Clave inválida para modo manual. Use 2 hex (ej: AA)\n" RESET);
                    print_usage(argv[0]);
                    return EXIT_FAILURE;
                }
                custom_key = k;
                has_custom_key = 1;
                delay_ms = 0; // en manual no aplica slowdown
            }
        } else { // argc == 4
            // ./emisor auto KEY MS
            // ./emisor manual KEY (MS ignorado)
            if (mode == MODE_AUTO) {
                unsigned char k = 0;
                int d = 0;
                if (!is_two_hex_chars(argv[2], &k) || !parse_nonneg_int(argv[3], &d)) {
                    fprintf(stderr, RED "[ERROR] Use: auto <KEY(hex2)> <MS>\n" RESET);
                    print_usage(argv[0]);
                    return EXIT_FAILURE;
                }
                custom_key = k;
                has_custom_key = 1;
                delay_ms = d;
            } else { // manual con KEY y un tercer arg que ignoramos
                unsigned char k = 0;
                if (!is_two_hex_chars(argv[2], &k)) {
                    fprintf(stderr, RED "[ERROR] Clave inválida para modo manual. Use 2 hex (ej: AA)\n" RESET);
                    print_usage(argv[0]);
                    return EXIT_FAILURE;
                }
                custom_key = k;
                has_custom_key = 1;
                delay_ms = 0; // manual ignora slowdown
            }
        }
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

        // --- NUEVO: aplicar slowdown sólo en modo AUTO y sólo si delay_ms > 0 ---
        if (mode == MODE_AUTO && delay_ms > 0) {
            usleep((useconds_t)delay_ms * 1000);
        }

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
