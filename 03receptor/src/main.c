/**
 * @file main.c
 * @brief Proceso Receptor del Sistema de Comunicación entre Procesos
 * 
 * Este programa implementa la funcionalidad del proceso receptor, que:
 * 1. Se conecta a la memoria compartida creada por el inicializador
 * 2. Lee caracteres encriptados de la cola de desencriptación
 * 3. Desencripta los caracteres usando una clave XOR
 * 4. Escribe los caracteres desencriptados a un archivo de salida
 * 
 * Características principales:
 * - Soporta múltiples receptores ejecutándose en paralelo
 * - Mantiene el orden secuencial del texto original
 * - Modo automático con delay configurable o modo manual paso a paso
 * - Manejo de señales para finalización elegante
 * - Soporte para claves de encriptación personalizadas
 * - Estadísticas detalladas de procesamiento
 * 
 * Sincronización:
 * - Semáforos para protección de secciones críticas
 * - Cola circular para slots de caracteres
 * - Escritura atómica al archivo de salida
 */

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
#include "queue_operations.h"
#include "decoder.h"
#include "process_manager.h"
#include "output_file.h"

// =============================================================================
// VARIABLES GLOBALES (para limpieza ordenada al recibir señales)
// =============================================================================

static volatile sig_atomic_t should_terminate = 0;

// Handles a memoria compartida y semáforos POSIX
static SharedMemory* g_shm = NULL;
static sem_t* g_sem_global        = NULL;
static sem_t* g_sem_encrypt_queue = NULL;
static sem_t* g_sem_decrypt_queue = NULL;
static sem_t* g_sem_encrypt_spaces= NULL;
static sem_t* g_sem_decrypt_items = NULL;

// =============================================================================
// MANEJADORES DE SEÑALES
// =============================================================================

/**
 * Manejador de señales para finalización elegante
 * Captura: SIGINT (Ctrl+C), SIGTERM, SIGUSR1
 */
/**
 * @brief Manejador de señales para finalización elegante
 * 
 * Este manejador es llamado cuando el proceso recibe SIGINT (Ctrl+C),
 * SIGTERM o SIGUSR1. Establece una bandera atómica que indica al bucle
 * principal que debe terminar de manera ordenada.
 * 
 * @param sig Número de señal recibida (no usado)
 */
static void on_signal(int sig) {
    (void)sig;
    should_terminate = 1;  // Bandera atómica para salir del bucle principal
}

// =============================================================================
// FUNCIONES AUXILIARES
// =============================================================================

/**
 * @brief Suspende la ejecución por un número de milisegundos
 * 
 * Utilizada en modo automático para controlar la velocidad de
 * procesamiento de caracteres.
 * 
 * @param ms Milisegundos a esperar (debe ser positivo)
 */
static void sleep_ms(int ms) {
    if (ms <= 0) return;
    usleep((useconds_t)ms * 1000);
}

/**
 * @brief Parsea el modo de ejecución del receptor
 * 
 * Convierte una cadena en el modo de ejecución correspondiente.
 * Modos válidos: "auto" para procesamiento automático con delay,
 * "manual" para procesamiento paso a paso con interacción del usuario.
 * 
 * @param s String a parsear ("auto" o "manual")
 * @return MODE_AUTO, MODE_MANUAL o -1 si es inválido
 */
static int parse_mode(const char* s) {
    if (!s) return -1;
    if (strcmp(s, "auto") == 0)   return MODE_AUTO;
    if (strcmp(s, "manual") == 0) return MODE_MANUAL;
    return -1;
}

/**
 * @brief Parsea la clave de encriptación hexadecimal
 * 
 * Convierte una cadena hexadecimal de 2 caracteres en un byte
 * que se usará como clave de encriptación/desencriptación XOR.
 * 
 * @param s String hexadecimal de 2 caracteres
 * @param has_custom Puntero donde almacenar si se proporcionó una clave válida
 * @return Valor de la clave (0 si es inválida)
 */
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

/**
 * @brief Formatea un timestamp UNIX para display
 * 
 * Convierte un timestamp UNIX en una cadena de hora legible
 * en formato HH:MM:SS usando la zona horaria local.
 * 
 * @param t Timestamp UNIX a formatear
 * @param buf Buffer donde escribir el resultado
 * @param n Tamaño del buffer (debe ser >= 20)
 */
static void pretty_time(time_t t, char* buf, size_t n) {
    if (!buf || n < 20) return;
    struct tm* tm = localtime(&t);
    if (!tm) {
        snprintf(buf, n, "--:--:--");
        return;
    }
    strftime(buf, n, "%H:%M:%S", tm);
}

// =============================================================================
// DISPLAY
// =============================================================================

/**
 * @brief Muestra el banner inicial del receptor
 * 
 * Imprime un banner estilizado que identifica el programa
 * y su propósito en el sistema de comunicación.
 */
static void print_banner(void) {
    printf(BOLD GREEN "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║                        RECEPTOR                          ║\n" RESET);
    printf(BOLD GREEN "║         Sistema de Comunicación entre Procesos           ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

/**
 * @brief Muestra información detallada del carácter recibido
 * 
 * Imprime un cuadro informativo que muestra:
 * - PID del receptor
 * - Índice del carácter en el texto original
 * - Slot de memoria usado
 * - Valor encriptado y desencriptado del carácter
 * - Timestamp de inserción y PID del emisor
 * - Estado actual de las colas
 * 
 * @param shm Puntero a la memoria compartida
 */
static void print_reception_box(SharedMemory* shm,
                                int slot_index,
                                int text_index,
                                unsigned char encrypted,
                                char plain,
                                time_t inserted_at,
                                pid_t emisor_pid)
{
    // Formatear timestamp y representación del carácter
    char ts[32];
    pretty_time(inserted_at, ts, sizeof ts);
    
    char disp[8];
    safe_char_repr(plain, disp, sizeof disp);
    
    // Estado actual de las colas
    int enc_free = shm->encrypt_queue.size;
    int dec_items = shm->decrypt_queue.size;
    
    const char* color = BLUE;
    
    printf("%s╔════════════════════════════════════════════════════╗\n", color);
    printf(  "║               CARÁCTER RECIBIDO                    ║\n");
    printf(  "╠════════════════════════════════════════════════════╣\n");
    printf(  "║%s PID Receptor: %-6d                               %s║\n", 
             RESET, getpid(), color);
    printf(  "║%s Índice texto: %-6d / %-6d                      %s║\n", 
             RESET, text_index, shm->total_chars_in_file, color);
    printf(  "║%s Slot memoria: %-3d                                  %s║\n", 
             RESET, slot_index + 1, color);
    printf(  "║%s Encriptado:  0x%02X                                  %s║\n", 
             RESET, encrypted, color);
    printf(  "║%s Desencript.: '%-4s' (0x%02X)                         %s║\n", 
             RESET, disp, (unsigned char)plain, color);
    printf(  "║%s Insertado:   %-8s  Emisor PID: %-6d          %s║\n", 
             RESET, ts, (int)emisor_pid, color);
    printf(  "║%s Colas: [Libres: %3d] [Con datos: %3d]              %s║\n", 
             RESET, enc_free, dec_items, color);
    printf(  "╚════════════════════════════════════════════════════╝\n" RESET);
}

// =============================================================================
// FUNCIÓN PRINCIPAL
// =============================================================================

int main(int argc, char* argv[]) {
    print_banner();
    
    // =========================================================================
    // PARSEO DE ARGUMENTOS
    // =========================================================================
    
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
    
    // =========================================================================
    // CONFIGURACIÓN DE SEÑALES
    // =========================================================================
    
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    
    // =========================================================================
    // CONEXIÓN A MEMORIA COMPARTIDA
    // =========================================================================
    
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
    printf("  • Clave de desencriptación: 0x%02X\n", effective_key);
    printf("  • Modo: %s\n", mode == MODE_AUTO ? "AUTOMÁTICO" : "MANUAL");
    if (mode == MODE_AUTO) {
        printf("  • Delay: %d ms\n", delay_ms);
    }
    
    // =========================================================================
    // APERTURA DE SEMÁFOROS POSIX
    // =========================================================================
    
    printf(CYAN "ℹ [RECEPTOR] Abriendo semáforos POSIX...\n" RESET);
    
    g_sem_global         = sem_open(SEM_NAME_GLOBAL_MUTEX, 0);
    g_sem_encrypt_queue  = sem_open(SEM_NAME_ENCRYPT_QUEUE, 0);
    g_sem_decrypt_queue  = sem_open(SEM_NAME_DECRYPT_QUEUE, 0);
    g_sem_encrypt_spaces = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    g_sem_decrypt_items  = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);
    
    if (g_sem_global == SEM_FAILED || g_sem_encrypt_queue == SEM_FAILED ||
        g_sem_decrypt_queue == SEM_FAILED || g_sem_encrypt_spaces == SEM_FAILED ||
        g_sem_decrypt_items == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] No se pudieron abrir todos los semáforos: %s\n" RESET, 
                strerror(errno));
        
        // Cleanup parcial
        if (g_sem_global         && g_sem_global         != SEM_FAILED) sem_close(g_sem_global);
        if (g_sem_encrypt_queue  && g_sem_encrypt_queue  != SEM_FAILED) sem_close(g_sem_encrypt_queue);
        if (g_sem_decrypt_queue  && g_sem_decrypt_queue  != SEM_FAILED) sem_close(g_sem_decrypt_queue);
        if (g_sem_encrypt_spaces && g_sem_encrypt_spaces != SEM_FAILED) sem_close(g_sem_encrypt_spaces);
        if (g_sem_decrypt_items  && g_sem_decrypt_items  != SEM_FAILED) sem_close(g_sem_decrypt_items);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    
    printf(GREEN "✓ Semáforos abiertos\n" RESET);
    
    // =========================================================================
    // REGISTRO DEL RECEPTOR
    // =========================================================================
    
    pid_t my_pid = getpid();
    if (register_receptor(shm, my_pid, g_sem_global) != SUCCESS) {
        fprintf(stderr, RED "[ERROR] No se pudo registrar el receptor\n" RESET);
        
        // Cleanup
        sem_close(g_sem_global);
        sem_close(g_sem_encrypt_queue);
        sem_close(g_sem_decrypt_queue);
        sem_close(g_sem_encrypt_spaces);
        sem_close(g_sem_decrypt_items);
        detach_shared_memory(shm);
        return EXIT_FAILURE;
    }
    
    // =========================================================================
    // APERTURA DE ARCHIVO DE SALIDA
    // =========================================================================
    
    char out_path[PATH_MAX];
    int out_fd = open_output_file(shm->input_filename, shm->total_chars_in_file,
                                  out_path, sizeof out_path);
    if (out_fd == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo preparar archivo de salida: %s\n" RESET, 
                strerror(errno));
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
    
    printf(BOLD GREEN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║             RECEPTOR PID %6d INICIADO                  ║\n" RESET, my_pid);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
    
    // =========================================================================
    // BUCLE PRINCIPAL DE RECEPCIÓN
    // =========================================================================
    
    int chars_recv = 0;
    time_t t0 = time(NULL);
    
    while (!should_terminate && !shm->shutdown_flag) {
        
        // =====================================================================
        // VERIFICACIÓN DE FINALIZACIÓN #1 (antes de bloquear)
        // Esta verificación previene que el proceso quede bloqueado esperando
        // datos que nunca llegarán cuando ya se procesó todo el archivo.
        // NO es busy waiting porque se ejecuta solo UNA VEZ por iteración.
        // =====================================================================
        
        int should_exit = 0;
        
        sem_wait(g_sem_global);
        if (shm->total_chars_processed >= shm->total_chars_in_file) {
            // El archivo completo ya fue procesado por los emisores
            sem_wait(g_sem_decrypt_queue);
            int queue_empty = (shm->decrypt_queue.size == 0);
            sem_post(g_sem_decrypt_queue);
            
            if (queue_empty) {
                // No hay datos pendientes por leer
                should_exit = 1;
            }
        }
        sem_post(g_sem_global);
        
        if (should_exit) {
            printf(YELLOW "\n[RECEPTOR %d] Todos los caracteres procesados y cola vacía\n" RESET, 
                   getpid());
            printf(CYAN "  • Total procesado globalmente: %d/%d\n" RESET, 
                   shm->total_chars_processed, shm->total_chars_in_file);
            printf(CYAN "  • Recibidos por este receptor: %d\n" RESET, chars_recv);
            break;
        }
        
        // =====================================================================
        // PASO 1: Esperar a que haya un item disponible
        // sem_wait() BLOQUEA sin busy waiting hasta que un emisor publique
        // =====================================================================
        
        if (sem_wait(g_sem_decrypt_items) != 0) {
            if (errno == EINTR) {
                // Interrumpido por señal
                if (should_terminate || shm->shutdown_flag) break;
                continue;  // Reintentar
            }
            fprintf(stderr, RED "[ERROR] sem_wait(decrypt_items): %s\n" RESET, strerror(errno));
            break;
        }
        
        // =====================================================================
        // PASO 2: Extraer elemento de la cola (sección crítica)
        // =====================================================================
        
        sem_wait(g_sem_decrypt_queue);
        SlotInfo info = dequeue_decrypt_slot_ordered(shm);
        sem_post(g_sem_decrypt_queue);
        
        if (info.slot_index < 0) {
            // Inconsistencia: el semáforo indicó item pero la cola estaba vacía
            continue;
        }
        
        // =====================================================================
        // PASO 3: Leer el slot
        // =====================================================================
        
        CharacterSlot slot;
        if (get_slot_info(shm, info.slot_index, &slot) != SUCCESS || !slot.is_valid) {
            // Slot inválido: liberarlo y continuar
            sem_wait(g_sem_encrypt_queue);
            enqueue_encrypt_slot(shm, info.slot_index);
            sem_post(g_sem_encrypt_queue);
            sem_post(g_sem_encrypt_spaces);
            continue;
        }
        
        // =====================================================================
        // PASO 4: Desencriptar el carácter
        // =====================================================================
        
        unsigned char enc = slot.ascii_value;
        char plain = (char)xor_apply(enc, effective_key);
        
        // =====================================================================
        // PASO 5: Escribir el byte desencriptado al archivo de salida
        // pwrite() permite escritura posicional segura entre múltiples receptores
        // =====================================================================
        
        if (write_decoded_char(out_fd, info.text_index, (unsigned char)plain) != 0) {
            fprintf(stderr, RED "[ERROR] Escritura de salida falló en índice %d: %s\n" RESET,
                    info.text_index, strerror(errno));
        }
        
        // =====================================================================
        // PASO 6: Marcar el slot como libre
        // =====================================================================
        
        CharacterSlot* buf = get_buffer_pointer(shm);
        if (buf) {
            buf[info.slot_index].is_valid = 0;
            buf[info.slot_index].ascii_value = 0;
        }
        
        // =====================================================================
        // PASO 7: Devolver el slot a la cola de encriptación
        // =====================================================================
        
        sem_wait(g_sem_encrypt_queue);
        enqueue_encrypt_slot(shm, info.slot_index);
        sem_post(g_sem_encrypt_queue);
        sem_post(g_sem_encrypt_spaces);  // Avisar al emisor que hay espacio
        
        // =====================================================================
        // PASO 8: Mostrar información del carácter recibido
        // =====================================================================
        
        print_reception_box(shm, info.slot_index, info.text_index, enc, plain,
                            slot.timestamp, slot.emisor_pid);
        chars_recv++;
        
        // =====================================================================
        // VERIFICACIÓN DE FINALIZACIÓN #2 (después de procesar)
        // Segunda oportunidad de salida limpia si acabamos de procesar el
        // último carácter del archivo.
        // =====================================================================
        
        sem_wait(g_sem_global);
        int all_processed = (shm->total_chars_processed >= shm->total_chars_in_file);
        sem_post(g_sem_global);
        
        if (all_processed) {
            sem_wait(g_sem_decrypt_queue);
            int queue_empty = (shm->decrypt_queue.size == 0);
            sem_post(g_sem_decrypt_queue);
            
            if (queue_empty) {
                printf(YELLOW "\n[RECEPTOR %d] Archivo completo procesado\n" RESET, getpid());
                break;
            }
        }
        
        // =====================================================================
        // PASO 9: Control de modo (auto/manual)
        // =====================================================================
        
        if (mode == MODE_MANUAL) {
            printf(CYAN "\nPresione ENTER para continuar (o Ctrl+C para salir)..." RESET);
            char tmp[8];
            if (!fgets(tmp, sizeof tmp, stdin)) break;
        } else {
            sleep_ms(delay_ms);
        }
    }
    
    // =========================================================================
    // RESUMEN Y ESTADÍSTICAS
    // =========================================================================
    
    time_t t1 = time(NULL);
    int elapsed = (int)(t1 - t0);

    // NUEVO: Guardar estadísticas antes de desregistrar
    save_receptor_stats(shm, my_pid, chars_recv, t0, t1, g_sem_global);
    
    printf(BOLD YELLOW "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD YELLOW "║             RECEPTOR PID %6d FINALIZANDO               ║\n" RESET, my_pid);
    printf(BOLD YELLOW "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("  • Caracteres recibidos: %d\n", chars_recv);
    printf("  • Tiempo de ejecución: %d s\n", elapsed);
    if (elapsed > 0) {
        printf("  • Velocidad promedio: %.2f chars/s\n", (float)chars_recv / elapsed);
    }
    
    // =========================================================================
    // LIMPIEZA Y CIERRE
    // =========================================================================
    
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