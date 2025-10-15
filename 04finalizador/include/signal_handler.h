#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <termios.h>

void setup_signal_handlers(void);
int setup_keyboard_input(void);
void cleanup_keyboard(void);
int check_keyboard_input(void);
void cleanup_and_exit(int signo);

#endif // SIGNAL_HANDLER_H