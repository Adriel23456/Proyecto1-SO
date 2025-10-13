#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "display.h"
#include "constants.h"

static int is_printable_char(char c) {
    return (c >= 32 && c < 127);
}

static void get_safe_char_display(char c, char* buffer, size_t size) {
    if (buffer == NULL || size < 5) return;
    
    if (c == '\n') snprintf(buffer, size, "\\n");
    else if (c == '\r') snprintf(buffer, size, "\\r");
    else if (c == '\t') snprintf(buffer, size, "\\t");
    else if (c == '\0') snprintf(buffer, size, "\\0");
    else if (is_printable_char(c)) snprintf(buffer, size, "%c", c);
    else snprintf(buffer, size, "\\x%02X", (unsigned char)c);
}

void print_emisor_banner() {
    printf(BOLD GREEN "╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD GREEN "║                       EMISOR                            ║\n" RESET);
    printf(BOLD GREEN "║         Sistema de Comunicación entre Procesos          ║\n" RESET);
    printf(BOLD GREEN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void print_emission_status(SharedMemory* shm, int slot_index, char original, 
                           unsigned char encrypted, int text_index) {
    if (shm == NULL) return;
    
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);
    CharacterSlot* slot = &buffer[slot_index];
    
    char time_str[32];
    struct tm* timeinfo = localtime(&slot->timestamp);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    
    char safe_display[10];
    get_safe_char_display(original, safe_display, sizeof(safe_display));
    
    const char* color = (original == '\n' || original == '\r') ? YELLOW : 
                        (!is_printable_char(original)) ? CYAN : GREEN;
    
    int encrypt_slots = shm->encrypt_queue.size;
    int decrypt_items = shm->decrypt_queue.size;
    
    printf("%s╔════════════════════════════════════════════════════╗\n", color);
    printf("║               CARÁCTER ENVIADO                     ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");
    printf("║%s PID Emisor: %-6d                                 %s║\n", RESET, getpid(), color);
    printf("║%s Índice texto: %-6d / %-6d                      %s║\n", RESET, 
           text_index, shm->total_chars_in_file, color);
    printf("║%s Slot memoria: %-3d                                  %s║\n", RESET, 
           slot_index + 1, color);
    printf("║%s Original: '%-5s' (0x%02X)                           %s║\n", RESET, 
           safe_display, (unsigned char)original, color);
    printf("║%s Encriptado: 0x%02X                                   %s║\n", RESET, 
           encrypted, color);
    printf("║%s Hora: %-8s                                     %s║\n", RESET, time_str, color);
    printf("║%s Colas: [Libres: %3d] [Con datos: %3d]              %s║\n", RESET,
           encrypt_slots, decrypt_items, color);
    printf("╚════════════════════════════════════════════════════╝\n%s", RESET);
}