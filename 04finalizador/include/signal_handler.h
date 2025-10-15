#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <termios.h>

/**
 * Manejo de señales y entrada de teclado
 *
 * setup_signal_handlers()      - Instala handlers (SIGINT/SIGTERM) async-safe
 * setup_keyboard_input()       - Configura terminal en modo raw + no bloqueante
 * cleanup_keyboard()           - Restaura terminal
 * check_keyboard_input()       - Lee una tecla; 1 si 'q'/'Q', 0 si otra/ninguna
 * wait_for_quit_or_signal()    - Espera bloqueante sin busy-wait: 'q' o señal
 * cleanup_and_exit()           - Compatibilidad; solo marca flag de salida
 */

void setup_signal_handlers(void);
int  setup_keyboard_input(void);
void cleanup_keyboard(void);
int  check_keyboard_input(void);
int  wait_for_quit_or_signal(void);
void cleanup_and_exit(int signo);

/* Flag global (marcada por handler) */
extern volatile sig_atomic_t shutdown_requested;

#endif // SIGNAL_HANDLER_H
