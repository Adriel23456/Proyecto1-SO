#ifndef SEMAPHORE_INIT_H
#define SEMAPHORE_INIT_H

// Funciones principales de semáforos
int initialize_semaphores(int buffer_size);
int get_semaphore_set();
int cleanup_semaphores();

// Operaciones de semáforos
int sem_wait_custom(int sem_id, int sem_num);
int sem_post_custom(int sem_id, int sem_num);
int sem_trywait_custom(int sem_id, int sem_num);

// Funciones de utilidad
int get_semaphore_value(int sem_id, int sem_num);
void print_semaphore_values(int sem_id, int buffer_size);
void wake_all_blocked_processes(int sem_id, int buffer_size);

#endif // SEMAPHORE_INIT_H