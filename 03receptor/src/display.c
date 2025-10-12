#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "display.h"
#include "constants.h"
#include "decoder.h"

static void fmt_time(time_t ts, char* out, size_t n) {
    if (!out || n < 20) return;
    struct tm* ti = localtime(&ts);
    if (!ti) { snprintf(out, n, "--:--:--"); return; }
    strftime(out, n, "%H:%M:%S", ti);
}

void print_receptor_banner(void) {
    printf(BOLD BLUE "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD BLUE "║                        RECEPTOR                          ║\n" RESET);
    printf(BOLD BLUE "║         Sistema de Comunicación entre Procesos           ║\n" RESET);
    printf(BOLD BLUE "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void print_receive_status(SharedMemory* shm,
                          int slot_index,
                          unsigned char encrypted,
                          char decrypted,
                          int text_index,
                          time_t produced_at)
{
    char t_prod[32]; fmt_time(produced_at, t_prod, sizeof t_prod);
    time_t now = time(NULL);
    char t_now[32]; fmt_time(now, t_now, sizeof t_now);

    char drepr[16]; safe_char_repr(decrypted, drepr, sizeof drepr);

    const char* color = GREEN;
    if (!is_printable_char(decrypted)) color = CYAN;
    if (decrypted == '\n' || decrypted == '\r' || decrypted == '\t') color = YELLOW;

    int enc_free = shm->encrypt_queue.size;
    int dec_items= shm->decrypt_queue.size;

    printf("%s╔════════════════════════════════════════════════════╗\n", color);
    printf("║               CARÁCTER RECIBIDO                   ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");
    printf("║%s PID Receptor: %-6d                              %s║\n", RESET, getpid(), color);
    printf("║%s Índice texto: %-6d / %-6d                  %s║\n", RESET, text_index, shm->total_chars_in_file, color);
    printf("║%s Slot memoria: %-3d                               %s║\n", RESET, slot_index + 1, color);
    printf("║%s Encriptado:  0x%02X                                %s║\n", RESET, encrypted, color);
    printf("║%s Desencrip.: '%-5s' (0x%02X)                      %s║\n", RESET, drepr, (unsigned char)decrypted, color);
    printf("║%s Producido:   %-8s  |  Leído: %-8s        %s║\n", RESET, t_prod, t_now, color);
    printf("║%s Colas: [Libres: %3d] [Con datos: %3d]           %s║\n", RESET, enc_free, dec_items, color);
    printf("╚════════════════════════════════════════════════════╝\n%s", RESET);
}

void print_reception_progress(SharedMemory* shm, int local_count) {
    if (!shm) return;
    int total = shm->total_chars_in_file;
    // Se puede estimar con current_txt_index si los emisores siguen activos,
    // pero aquí mostramos progreso local de este receptor:
    printf(CYAN "\n=== Progreso Receptor PID %d ===\n" RESET, getpid());
    printf("  • Total archivo: %d bytes\n", total);
    printf("  • Leídos por este receptor: %d\n", local_count);
}

void print_receptor_summary(SharedMemory* shm, int local_count, time_t start_ts) {
    time_t end = time(NULL);
    int sec = (int)(end - start_ts);
    printf(BOLD CYAN "\n╔════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD CYAN   "║                RESUMEN DEL RECEPTOR               ║\n" RESET);
    printf(BOLD CYAN   "╚════════════════════════════════════════════════════╝\n" RESET);
    printf("  • PID: %d\n", getpid());
    printf("  • Caracteres leídos: %d\n", local_count);
    printf("  • Tiempo de ejecución: %d s\n", sec);
    if (sec > 0) printf("  • Velocidad promedio: %.2f chars/s\n", (float)local_count/sec);
    printf("  • Emisores activos: %d | Receptores activos: %d\n",
           shm->active_emisores, shm->active_receptores);
}

void print_info(const char* msg) {
    if (msg) printf(CYAN "ℹ %s\n" RESET, msg);
}
void print_warn(const char* msg) {
    if (msg) printf(YELLOW "⚠ %s\n" RESET, msg);
}
void print_err(const char* msg) {
    if (msg) fprintf(stderr, RED "✗ %s\n" RESET, msg);
}
