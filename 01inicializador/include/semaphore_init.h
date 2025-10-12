#ifndef SEMAPHORE_INIT_H
#define SEMAPHORE_INIT_H

#include <semaphore.h>

/*
 * Inicialización y utilidades con semáforos POSIX nombrados:
 *
 * initialize_semaphores(buffer_size)
 *   - Crea (o recrea) los semáforos nombrados del sistema y los deja con
 *     sus valores iniciales:
 *       * GLOBAL_MUTEX: 1
 *       * ENCRYPT_QUEUE: 1
 *       * DECRYPT_QUEUE: 1
 *       * ENCRYPT_SPACES: buffer_size
 *       * DECRYPT_ITEMS: 0
 *   - No realiza unlink; otros procesos deben poder abrirlos.
 *
 * cleanup_semaphores()
 *   - Elimina los semáforos nombrados (sem_unlink) si existen.
 *   - Está pensado para el finalizador, no para el inicializador.
 *
 * print_semaphore_values()
 *   - Muestra los valores actuales de cada semáforo vía sem_getvalue().
 *
 * wake_all_blocked_processes(buffer_size)
 *   - Publica múltiples veces en los contadores para despertar procesos
 *     potencialmente bloqueados. Útil durante el apagado ordenado.
 */

int initialize_semaphores(int buffer_size);
int cleanup_semaphores(void);
void print_semaphore_values(void);
void wake_all_blocked_processes(int buffer_size);

#endif // SEMAPHORE_INIT_H
