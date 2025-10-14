#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void setup_signal_handlers(void);
int setup_input_trigger(void);
void cleanup_trigger(void);
int check_trigger_state(void);
void cleanup_and_exit(int signo);

#endif // SIGNAL_HANDLER_H