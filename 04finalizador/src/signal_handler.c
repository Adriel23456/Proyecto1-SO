#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/select.h>
#include "signal_handler.h"

/**
 * Manejo de señales y entrada de teclado
 * - Handlers async-safe: solo marcan flag
 * - Teclado en modo raw, lectura no bloqueante
 * - Espera bloqueante sin busy-wait usando select()
 */

volatile sig_atomic_t shutdown_requested = 0;
static struct termios old_tio, new_tio;

int setup_keyboard_input(void) {
    if (tcgetattr(STDIN_FILENO, &old_tio) == -1) {
        perror("Error obteniendo configuración del terminal");
        return -1;
    }
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) == -1) {
        perror("Error configurando terminal");
        return -1;
    }
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf("\033[1;32m✓ Terminal configurado (modo raw)\033[0m\n");
    return 0;
}

int check_keyboard_input(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        return (c == 'q' || c == 'Q');
    }
    return 0;
}

void cleanup_keyboard(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_tio);
    tcflush(STDIN_FILENO, TCIOFLUSH);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

/* Handler simple y async-safe: sólo marca flag */
static void handle_signal(int signo) {
    (void)signo;
    shutdown_requested = 1;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Compat: sólo marca la salida */
void cleanup_and_exit(int signo) {
    (void)signo;
    shutdown_requested = 1;
}

/**
 * Espera bloqueante sin busy-wait hasta:
 *  - presionar 'q'/'Q', o
 *  - recibir SIGINT/SIGTERM (shutdown_requested)
 * Implementación: select() sobre stdin (sin tiempo), interrumpible por señales.
 */
int wait_for_quit_or_signal(void) {
    printf("\033[1;33m→ Esperando que presione 'q' para finalizar (bloqueante, sin busy-wait)...\033[0m\n");
    fflush(stdout);

    while (!shutdown_requested) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);

        int r = select(STDIN_FILENO + 1, &rfds, NULL, NULL, NULL);
        if (r < 0) {
            if (errno == EINTR) {
                if (shutdown_requested) return 1;
                continue;
            }
            /* En error inesperado, salir del bucle para no colgar */
            return 1;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            if (check_keyboard_input()) return 1;
        }
    }
    return 1;
}