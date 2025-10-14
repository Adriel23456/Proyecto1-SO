#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "signal_handler.h"
#include "shared_memory_access.h"

/**
 * Manejo de señales y mecanismo de trigger
 * 
 * Este archivo implementa un mecanismo de trigger basado en archivo para simular
 * un botón físico o switch. También maneja las señales del sistema para una
 * finalización limpia del programa.
 */

/* Archivo usado como trigger para simular un botón físico */
#define TRIGGER_FILE "/tmp/finalizador_trigger"

static SharedMemory* shm = NULL;
volatile sig_atomic_t shutdown_requested = 0;
static int trigger_fd = -1;

/**
 * @brief Configura el mecanismo de trigger basado en archivo
 * 
 * Esta función crea y configura un archivo que actúa como trigger.
 * El archivo se usa como un interruptor: cuando su contenido es '1',
 * se activa la finalización del sistema. Simula un botón físico.
 * 
 * @return 0 si la configuración fue exitosa, -1 en caso de error
 */
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

/**
 * @brief Verifica el estado actual del trigger
 * 
 * Lee el contenido del archivo trigger para determinar si se ha
 * solicitado la finalización del sistema. El archivo contiene
 * un '1' cuando se activa el trigger.
 * 
 * @return 1 si el trigger está activado, 0 en caso contrario
 */
int check_trigger_state(void) {
    char state;
    lseek(trigger_fd, 0, SEEK_SET);
    if (read(trigger_fd, &state, 1) == 1) {
        return (state == '1');
    }
    return 0;
}

/**
 * @brief Limpia los recursos del trigger
 * 
 * Cierra y elimina el archivo usado como trigger.
 * Esta función debe llamarse durante la limpieza del programa
 * para no dejar recursos sin liberar.
 */
void cleanup_trigger(void) {
    if (trigger_fd != -1) {
        close(trigger_fd);
        unlink(TRIGGER_FILE);
    }
}

/**
 * @brief Maneja la limpieza y salida del programa
 * 
 * Esta función se ejecuta cuando se recibe una señal de terminación
 * (como Ctrl+C). Realiza una limpieza ordenada liberando todos
 * los recursos antes de terminar el programa.
 * 
 * @param signo Número de la señal recibida
 */
void cleanup_and_exit(int signo) {
    cleanup_trigger();
    if (shm) {
        detach_shared_memory(shm);
    }
    exit(0);
}

/**
 * @brief Configura los manejadores de señales del sistema
 * 
 * Inicializa la conexión a la memoria compartida y configura
 * el manejador para la señal SIGINT (Ctrl+C). Esto asegura
 * una terminación limpia incluso si el usuario interrumpe
 * el programa manualmente.
 */
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