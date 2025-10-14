#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "signal_handler.h"
#include "shared_memory_access.h"

#define TRIGGER_FILE "/tmp/finalizador_trigger"

static SharedMemory* shm = NULL;
volatile sig_atomic_t shutdown_requested = 0;
static int trigger_fd = -1;

int setup_input_trigger(void) {
    // Crear archivo de trigger si no existe
    trigger_fd = open(TRIGGER_FILE, O_CREAT | O_RDWR, 0666);
    if (trigger_fd == -1) {
        perror("Error creando archivo trigger");
        return -1;
    }
    
    // Escribir estado inicial
    write(trigger_fd, "0", 1);
    lseek(trigger_fd, 0, SEEK_SET);
    
    printf("\033[1;32m✓ Trigger configurado\033[0m\n");
    printf("\033[1;33m→ Para activar la finalización, ejecute:\033[0m\n");
    printf("   echo 1 > %s\n", TRIGGER_FILE);
    return 0;
}

int check_trigger_state(void) {
    char state;
    lseek(trigger_fd, 0, SEEK_SET);
    if (read(trigger_fd, &state, 1) == 1) {
        return (state == '1');
    }
    return 0;
}

void cleanup_trigger(void) {
    if (trigger_fd != -1) {
        close(trigger_fd);
        unlink(TRIGGER_FILE);
    }
}

void cleanup_and_exit(int signo) {
    cleanup_trigger();
    if (shm) {
        detach_shared_memory(shm);
    }
    exit(0);
}

void setup_signal_handlers(void) {
    shm = attach_shared_memory();
    
    // Configurar manejador para SIGINT (Ctrl+C)
    struct sigaction sa;
    sa.sa_handler = cleanup_and_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}