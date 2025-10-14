#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "signal_handler.h"
#include "shared_memory_access.h"

extern volatile sig_atomic_t shutdown_requested;

int main() {
    printf("\033[1;36m╔════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║                     FINALIZADOR                            ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════════════════╝\033[0m\n\n");

    setup_signal_handlers();
    if (setup_input_trigger() < 0) {
        fprintf(stderr, "Error configurando trigger\n");
        return 1;
    }
    
    SharedMemory* shm = attach_shared_memory();

    printf("\033[1;33m→ Esperando activación del trigger...\033[0m\n");
    // Esperar activación del trigger
    while (!check_trigger_state()) {
        sleep(1);
    }

    // Activar flag de finalización
    shm->shutdown_flag = 1;
    printf("\033[1;33m→ Solicitando finalización de procesos...\033[0m\n");

    // Esperar a que todos los procesos terminen
    while (shm->active_emisores > 0 || shm->active_receptores > 0) {
        printf("\033[1;34m→ Esperando finalización (%d emisores, %d receptores activos)\033[0m\r",
               shm->active_emisores, shm->active_receptores);
        fflush(stdout);
        sleep(1);
    }
    printf("\n\033[1;32m✓ Todos los procesos han finalizado\033[0m\n\n");

    // Mostrar estadísticas
    print_statistics(shm);

    // Limpiar y salir
    detach_shared_memory(shm);
    printf("\n\033[1;32m✓ Finalización completada\033[0m\n");
    
    return 0;
}