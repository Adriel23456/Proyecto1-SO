#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>  // Para getpid()
#include "display.h"
#include "encoder.h"
#include "constants.h"
#include "structures.h"

/*
 * Imprimir banner del emisor.
 */
void print_emisor_banner() {
    printf(BOLD GREEN "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║                       EMISOR                            ║\n" RESET);
    printf(BOLD GREEN "║         Sistema de Comunicación entre Procesos          ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

/*
 * Formatear timestamp para display.
 */
static void format_timestamp(time_t timestamp, char* buffer, size_t size) {
    if (buffer == NULL || size < 20) return;
    
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, size, "%H:%M:%S", timeinfo);
}

/*
 * Imprimir estado de emisión de un carácter.
 */
void print_emission_status(SharedMemory* shm, int slot_index, char original, 
                           unsigned char encrypted, int text_index) {
    if (shm == NULL) return;
    
    // Obtener información del slot
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);
    CharacterSlot* slot = &buffer[slot_index];
    
    // Formatear timestamp
    char time_str[32];
    format_timestamp(slot->timestamp, time_str, sizeof(time_str));
    
    // Obtener representación segura del carácter
    char safe_display[10];
    get_safe_char_display(original, safe_display, sizeof(safe_display));
    
    // Usar colores diferentes según el tipo de carácter
    const char* color = GREEN;
    if (original == '\n' || original == '\r') {
        color = YELLOW;
    } else if (!is_printable_char(original)) {
        color = CYAN;
    }
    
    // Estado de colas
    int encrypt_slots = shm->encrypt_queue.size;
    int decrypt_items = shm->decrypt_queue.size;
    
    // Imprimir box con colores
    printf("%s╔════════════════════════════════════════════════════╗\n", color);
    printf("║               CARÁCTER ENVIADO                    ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");
    printf("║%s PID Emisor: %-6d                                %s║\n", RESET, getpid(), color);
    printf("║%s Índice texto: %-6d / %-6d                    %s║\n", RESET, 
           text_index, shm->total_chars_in_file, color);
    printf("║%s Slot memoria: %-3d                                 %s║\n", RESET, 
           slot_index + 1, color);
    printf("║%s Original: '%-5s' (0x%02X)                         %s║\n", RESET, 
           safe_display, (unsigned char)original, color);
    printf("║%s Encriptado: 0x%02X                                  %s║\n", RESET, 
           encrypted, color);
    printf("║%s Hora: %-8s                                  %s║\n", RESET, time_str, color);
    printf("║%s Colas: [Libres: %3d] [Con datos: %3d]             %s║\n", RESET,
           encrypt_slots, decrypt_items, color);
    printf("╚════════════════════════════════════════════════════╝\n%s", RESET);
}

/*
 * Imprimir progreso de emisión.
 */
void print_emission_progress(SharedMemory* shm, int chars_sent) {
    if (shm == NULL) return;
    
    int total = shm->total_chars_in_file;
    int current = shm->current_txt_index;
    float percentage = (total > 0) ? (float)current * 100.0f / total : 0;
    
    printf(CYAN "\n=== Progreso de Emisión ===\n" RESET);
    printf("Total archivo: %d caracteres\n", total);
    printf("Procesados globalmente: %d (%.1f%%)\n", current, percentage);
    printf("Enviados por este emisor: %d\n", chars_sent);
    
    // Barra de progreso
    printf("Progreso: [");
    int bar_width = 40;
    int filled = (int)(percentage * bar_width / 100);
    
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("=");
        } else if (i == filled) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf("] %.1f%%\n", percentage);
}

/*
 * Imprimir resumen de actividad.
 */
void print_activity_summary(SharedMemory* shm, int chars_sent, time_t start_time) {
    if (shm == NULL) return;
    
    time_t current_time = time(NULL);
    int elapsed = (int)(current_time - start_time);
    
    printf(BOLD CYAN "\n╔════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD CYAN "║              RESUMEN DE ACTIVIDAD                 ║\n" RESET);
    printf(BOLD CYAN "╚════════════════════════════════════════════════════╝\n" RESET);
    
    printf("  • PID del emisor: %d\n", getpid());
    printf("  • Caracteres enviados: %d\n", chars_sent);
    printf("  • Tiempo de ejecución: %d segundos\n", elapsed);
    
    if (elapsed > 0) {
        float rate = (float)chars_sent / elapsed;
        printf("  • Velocidad promedio: %.2f chars/segundo\n", rate);
    }
    
    // Estado del sistema
    printf("\n  Estado del sistema:\n");
    printf("  • Emisores activos: %d\n", shm->active_emisores);
    printf("  • Receptores activos: %d\n", shm->active_receptores);
    printf("  • Total procesado globalmente: %d/%d\n", 
           shm->total_chars_processed, shm->total_chars_in_file);
}

/*
 * Imprimir advertencia.
 */
void print_warning(const char* message) {
    if (message == NULL) return;
    
    printf(YELLOW "\n⚠ ADVERTENCIA: %s\n" RESET, message);
}

/*
 * Imprimir error.
 */
void print_error(const char* message) {
    if (message == NULL) return;
    
    fprintf(stderr, RED "\n✗ ERROR: %s\n" RESET, message);
}

/*
 * Imprimir información.
 */
void print_info(const char* message) {
    if (message == NULL) return;
    
    printf(CYAN "\nℹ INFO: %s\n" RESET, message);
}

/*
 * Limpiar pantalla (opcional).
 */
void clear_screen() {
    printf("\033[2J\033[1;1H");  // ANSI escape codes
}

/*
 * Imprimir separador.
 */
void print_separator() {
    printf(WHITE "════════════════════════════════════════════════════════\n" RESET);
}