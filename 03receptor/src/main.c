// src/main.c
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <limits.h>

#include "constants.h"
#include "structures.h"
#include "shared_memory_access.h"
#include "queue_operations.h"        // dequeue_decrypt_slot_ordered / enqueue_encrypt_slot
#include "decoder.h"              // xor_apply, is_printable_char
#include "process_manager.h"      // register_receptor / unregister_receptor
#include "output_file.h"          // open_output_file / write_decoded_char

// --------------------------- Globals ---------------------------------
static volatile sig_atomic_t should_terminate = 0;

// Handles globales a SHM y semáforos (para limpieza ordenada)
static SharedMemory* g_shm = NULL;
static sem_t* g_sem_global        = NULL;
static sem_t* g_sem_encrypt_queue = NULL;
static sem_t* g_sem_decrypt_queue = NULL;
static sem_t* g_sem_encrypt_spaces= NULL;
static sem_t* g_sem_decrypt_items = NULL;

// --------------------------- Utils -----------------------------------
static void on_signal(int sig) {
    (void)sig;
    should_terminate = 1;
}

static void sleep_ms(int ms) {
    if (ms <= 0) return;
    usleep((useconds_t)ms * 1000);
}

static int parse_mode(const char* s) {
    if (!s) return -1;
    if (strcmp(s, "auto") == 0)   return MODE_AUTO;
    if (strcmp(s, "manual") == 0) return MODE_MANUAL;
    return -1;
}

static unsigned char parse_key_opt(const char* s, int* has_custom) {
    *has_custom = 0;
    if (!s) return 0;
    if (strlen(s) != 2) return 0;
    unsigned char k = 0;
    if (sscanf(s, "%2hhx", &k) == 1) {
        *has_custom = 1;
        return k;
    }
    return 0;
}

static void print_banner(void) {
    printf(BOLD GREEN "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║                        RECEPTOR                          ║\n" RESET);
    printf(BOLD GREEN "║         Sistema de Comunicación entre Procesos           ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

static void pretty_time(time_t t, char* buf, size_t n) {
    if (!buf || n < 20) return;
    struct tm* tm = localtime(&t);
    if (!tm) { snprintf(buf, n, "--:--:--"); return; }
    strftime(buf, n, "%H:%M:%S", tm);
}

static void print_reception_box(SharedMemory* shm,
                                int slot_index,
                                int text_index,
                                unsigned char encrypted,
                                char plain,
                                time_t inserted_at,
                                pid_t emisor_pid)
{
    char ts[32]; pretty_time(inserted_at, ts, sizeof ts);
    char disp[8]; safe_char_repr(plain, disp, sizeof disp);

    int enc_free = shm->encrypt_queue.size;
    int dec_items = shm->decrypt_queue.size;

    const char* color = BLUE;

    printf("%s╔════════════════════════════════════════════════════╗\n", color);
    printf(  "║               CARÁCTER RECIBIDO                  ║\n");
    printf(  "╠════════════════════════════════════════════════════╣\n");
    printf(  "║%s PID Receptor: %-6d                              %s║\n", RESET, getpid(), color);
    printf(  "║%s Índice texto: %-6d / %-6d                    %s║\n", RESET, text_index,
            shm->total_chars_in_file, color);
    printf(  "║%s Slot memoria: %-3d                               %s║\n", RESET, slot_index + 1, color);
    printf(  "║%s Encriptado:  0x%02X                               %s║\n", RESET, encrypted, color);
    printf(  "║%s Desencript.: '%-4s' (0x%02X)                     %s║\n", RESET, disp, (unsigned char)plain, color);
    printf(  "║%s Insertado:   %-8s  Emisor PID: %-6d         %s║\n", RESET, ts, (int)emisor_pid, color);
    printf(  "║%s Colas: [Libres: %3d] [Con datos: %3d]           %s║\n", RESET, enc_free, dec_items, color);
    printf(  "╚════════════════════════════════════════════════════╝\n" RESET);
}

// --------------------------- Main ------------------------------------
int main(int argc, char* argv[]) {
    print_banner();

    // ---- parse args ----
    if (argc < 2 || argc > 4) {
        fprintf(stderr, RED "[ERROR] Uso: %s <auto|manual> [clave_hex] [delay_ms]\n" RESET, argv[0]);
        fprintf(stderr, "Ejemplos:\n");
        fprintf(stderr, "  %s auto          # Usa clave de SHM, delay 100ms\n", argv[0]);
        fprintf(stderr, "  %s auto AA       # Usa clave AA, delay 100ms\n", argv[0]);
        fprintf(stderr, "  %s auto AA 10    # Usa clave AA, delay 10ms\n", argv[0]);
        fprintf(stderr, "  %s manual        # Modo manual\n", argv[0]);
        return EXIT_FAILURE;
    }

    int mode = parse_mode(argv[1]);
    if (mode == -1) {
        fprintf(stderr, RED "[ERROR] Modo inválido. Use 'auto' o 'manual'\n" RESET);
        return EXIT_FAILURE;
    }

    int has_custom_key = 0;
    unsigned char key = parse_key_opt((argc >= 3) ? argv[2] : NULL, &has_custom_key);

    int delay_ms = DEFAULT_DELAY_MS;
    if (argc >= 4 && mode == MODE_AUTO) {
        int d = atoi(argv[3]);
        if (d >= MIN_DELAY_MS && d <= MAX_DELAY_MS) delay_ms = d;
    }

    // ---- signals ----
    struct sigaction sa; memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    // ---- attach SHM ----
    printf(CYAN "ℹ [RECEPTOR] Conectando a memoria compartida...\n" RESET);
    SharedMemory* shm = attach_shared_memory(SHM_BASE_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a SHM. ¿Ejecutaste el inicializador?\n" RESET);
        return EXIT_FAILURE;
    }
    g_shm = shm;

    unsigned char effective_key = has_custom_key ? key : shm->encryption_key;

    printf(GREEN "✓ Conectado a SHM\n" RESET);
    printf("  • Buffer size: %d slots\n", shm->buffer_size);
    printf("  • Archivo fuente: %s (%d bytes)\n", shm->input_filename, shm->total_chars_in_file);
    printf("  • Clave de SHM: 0x%02X\n", effective_key);
    printf("  • Modo: %s\n", mode == MODE_AUTO ? "AUTOMÁTICO" : "MANUAL");

    // ---- open semaphores ----
    printf(CYAN "ℹ [RECEPTOR] Abriendo semáforos POSIX...\n" RESET);
    g_sem_global         = sem_open(SEM_NAME_GLOBAL_MUTEX, 0);
    g_sem_encrypt_queue  = sem_open(SEM_NAME_ENCRYPT_QUEUE, 0);
    g_sem_decrypt_queue  = sem_open(SEM_NAME_DECRYPT_QUEUE, 0);
    g_sem_encrypt_spaces = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    g_sem_decrypt_items  = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);

    if (g_sem_global == SEM_FAILED || g_sem_encrypt_queue == SEM_FAILED ||
        g_sem_decrypt_queue == SEM_FAILED || g_sem_encrypt_spaces == SEM_FAILED ||
        g_sem_decrypt_items == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudieron abrir todos los semáforos: %s\n" RESET, strerror(errno));
        if (g_sem_global         && g_sem_global         != SEM_FAILED) sem_close(g_sem_global);
        if (g_sem_encrypt_queue  && g_sem_encrypt_queue  != SEM_FAILED) sem_close(g_sem_encrypt_queue);
        if (g_sem_decrypt_queue  && g_sem_decrypt_queue  != SEM_FAILED) sem_close(g_sem_decrypt_queue);
        if (g_sem_encrypt_spaces && g_sem_encrypt_spaces != SEM_FAILED) sem_close(g_sem_encrypt_spaces);
        if (g_sem_decrypt_items  && g_sem_decrypt_items  != SEM_FAILED) sem_close(g_sem_decrypt_items);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    printf(GREEN "✓ Semáforos abiertos\n" RESET);

    // ---- register receptor ----
    pid_t my_pid = getpid();
    if (register_receptor(shm, my_pid, g_sem_global) != SUCCESS) {
        fprintf(stderr, RED "[ERROR] No se pudo registrar el receptor\n" RESET);
        // cleanup
        sem_close(g_sem_global);
        sem_close(g_sem_encrypt_queue);
        sem_close(g_sem_decrypt_queue);
        sem_close(g_sem_encrypt_spaces);
        sem_close(g_sem_decrypt_items);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }

    // ---- open output file (local) ----
    char out_path[PATH_MAX];
    int out_fd = open_output_file(shm->input_filename, shm->total_chars_in_file,
                                  out_path, sizeof out_path);
    if (out_fd == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo preparar archivo de salida: %s\n" RESET, strerror(errno));
        unregister_receptor(shm, my_pid, g_sem_global);
        sem_close(g_sem_global);
        sem_close(g_sem_encrypt_queue);
        sem_close(g_sem_decrypt_queue);
        sem_close(g_sem_encrypt_spaces);
        sem_close(g_sem_decrypt_items);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    printf(GREEN "✓ Archivo de salida: %s\n" RESET, out_path);

    // ---- main loop ----
    int chars_recv = 0;
    time_t t0 = time(NULL);

    while (!should_terminate && !shm->shutdown_flag) {
        // 1) Esperar item disponible (bloquea sin busy waiting)
        if (sem_wait(g_sem_decrypt_items) != 0) {
            if (errno == EINTR) {
                if (should_terminate || shm->shutdown_flag) break;
                continue;
            }
            fprintf(stderr, RED "[ERROR] sem_wait(decrypt_items): %s\n" RESET, strerror(errno));
            break;
        }

        // 2) Extraer elemento (sección crítica)
        sem_wait(g_sem_decrypt_queue);
        SlotInfo info = dequeue_decrypt_slot_ordered(shm);
        sem_post(g_sem_decrypt_queue);

        if (info.slot_index < 0) {
            // Inconsistencia: semáforo indicó item pero la cola estaba vacía
            // Nada que devolver; continuar
            continue;
        }

        // 3) Leer el slot
        CharacterSlot slot;
        if (get_slot_info(shm, info.slot_index, &slot) != SUCCESS || !slot.is_valid) {
            // Si el slot no es válido, liberarlo por seguridad
            sem_wait(g_sem_encrypt_queue);
            enqueue_encrypt_slot(shm, info.slot_index);
            sem_post(g_sem_encrypt_queue);
            sem_post(g_sem_encrypt_spaces);
            continue;
        }

        unsigned char enc = slot.ascii_value;
        char plain = (char)xor_apply(enc, effective_key);

        // 4) Escribir el byte desencriptado en la salida por índice
        if (write_decoded_char(out_fd, info.text_index, (unsigned char)plain) != 0) {
            fprintf(stderr, RED "[ERROR] Escritura de salida falló en índice %d: %s\n" RESET,
                    info.text_index, strerror(errno));
        }

        // 5) Marcar slot como libre y devolverlo a la cola de encrypt
        CharacterSlot* buf = get_buffer_pointer(shm);
        if (buf) {
            buf[info.slot_index].is_valid = 0;
            buf[info.slot_index].ascii_value = 0;
            // (slot_index 1-based se recalcula cuando un emisor lo use)
        }

        sem_wait(g_sem_encrypt_queue);
        enqueue_encrypt_slot(shm, info.slot_index);
        sem_post(g_sem_encrypt_queue);
        sem_post(g_sem_encrypt_spaces);

        // 6) Mostrar info
        print_reception_box(shm, info.slot_index, info.text_index, enc, plain,
                            slot.timestamp, slot.emisor_pid);
        chars_recv++;

        // 7) Modo
        if (mode == MODE_MANUAL) {
            printf(CYAN "\nPresione ENTER para continuar (o Ctrl+C para salir)..." RESET);
            char tmp[8];
            if (!fgets(tmp, sizeof tmp, stdin)) break;
        } else {
            sleep_ms(delay_ms);
        }

        // 8) Condición de salida: si ya todo fue procesado globalmente y no quedan items
        //    (opcional; el finalizador usualmente gestionará shutdown_flag)
        if (shm->total_chars_processed >= shm->total_chars_in_file && shm->decrypt_queue.size == 0) {
            // Nada más por hacer de momento; se podría esperar signal de finalizador.
            // Salimos limpiamente para no quedar vivos innecesariamente.
            break;
        }
    }

    // ---- summary ----
    time_t t1 = time(NULL);
    int elapsed = (int)(t1 - t0);
    printf(BOLD YELLOW "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD YELLOW "║             RECEPTOR PID %6d FINALIZANDO               ║\n" RESET, my_pid);
    printf(BOLD YELLOW "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("  • Caracteres recibidos: %d\n", chars_recv);
    printf("  • Tiempo de ejecución: %d s\n", elapsed);
    if (elapsed > 0) printf("  • Velocidad promedio: %.2f chars/s\n", (float)chars_recv / elapsed);

    // ---- cleanup ----
    close_output_file(out_fd);
    unregister_receptor(shm, my_pid, g_sem_global);

    sem_close(g_sem_global);
    sem_close(g_sem_encrypt_queue);
    sem_close(g_sem_decrypt_queue);
    sem_close(g_sem_encrypt_spaces);
    sem_close(g_sem_decrypt_items);

    detach_shared_memory(shm);

    printf(GREEN "\n[RECEPTOR %d] Proceso terminado correctamente\n" RESET, my_pid);
    return EXIT_SUCCESS;
}
