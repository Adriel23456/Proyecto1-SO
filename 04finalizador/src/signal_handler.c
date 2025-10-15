#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "signal_handler.h"
#include "shared_memory_access.h"

/**
 * Manejo de señales y entrada de teclado
 * 
 * Este archivo implementa un mecanismo de finalización basado en la
 * detección de una tecla específica ('q' o 'Q'). También maneja las
 * señales del sistema para una finalización limpia del programa.
 */

static SharedMemory* shm = NULL;
volatile sig_atomic_t shutdown_requested = 0;
static struct termios old_tio, new_tio;

/**
 * @brief Configura el mecanismo de trigger basado en archivo
 * 
 * Esta función crea y configura un archivo que actúa como trigger.
 * El archivo se usa como un interruptor: cuando su contenido es '1',
 * se activa la finalización del sistema. Simula un botón físico.
 * 
 * @return 0 si la configuración fue exitosa, -1 en caso de error
 */
/**
 * @brief Configura la entrada del teclado en modo raw
 * 
 * Configura el terminal para leer teclas individuales sin necesidad
 * de presionar Enter y sin mostrar la entrada en pantalla.
 * 
 * @return 0 si la configuración fue exitosa, -1 en caso de error
 */
int setup_keyboard_input(void) {
    // Obtener la configuración actual del terminal
    if (tcgetattr(STDIN_FILENO, &old_tio) == -1) {
        perror("Error obteniendo configuración del terminal");
        return -1;
    }
    
    // Hacer una copia para modificar
    new_tio = old_tio;
    
    // Configurar modo raw (sin eco y sin buffering)
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) == -1) {
        perror("Error configurando terminal");
        return -1;
    }
    
    // Configurar stdin en modo no bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    printf("\033[1;32m✓ Terminal configurado\033[0m\n");
    printf("\033[1;33m→ Presione 'q' para activar la finalización\033[0m\n");
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
/**
 * @brief Verifica si se presionó la tecla de finalización
 * 
 * Lee una tecla del teclado y verifica si es 'q' o 'Q'.
 * La lectura es no bloqueante, así que retorna inmediatamente
 * si no hay tecla disponible.
 * 
 * @return 1 si se presionó 'q'/'Q', 0 en caso contrario
 */
int check_keyboard_input(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        return (c == 'q' || c == 'Q');
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
/**
 * @brief Restaura la configuración original del terminal
 * 
 * Devuelve el terminal a su modo normal de operación,
 * restaurando el modo canónico y el eco.
 */
void cleanup_keyboard(void) {
    // Restaurar la configuración original del terminal
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_tio);
    
    // Limpiar cualquier entrada pendiente
    tcflush(STDIN_FILENO, TCIOFLUSH);
    
    // Restaurar el modo bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
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
    cleanup_keyboard();
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
/**
 * @brief Configura los manejadores de señales del sistema
 * 
 * Inicializa la conexión a la memoria compartida y configura
 * manejadores para SIGINT y SIGTERM. Esto asegura una terminación
 * limpia incluso si el usuario interrumpe el programa.
 */
void setup_signal_handlers(void) {
    shm = attach_shared_memory();
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
}