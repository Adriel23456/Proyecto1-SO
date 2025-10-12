#ifndef CONSTANTS_H
#define CONSTANTS_H

// Claves de memoria compartida (System V SHM)
#define SHM_BASE_KEY 0x1234

// Colores para output
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"
#define BOLD    "\x1b[1m"

// Modos de ejecución
#define MODE_AUTO   0
#define MODE_MANUAL 1

/*
 * Tamaños y límites:
 *  - MIN_BUFFER_SIZE: tamaño mínimo del buffer circular.
 *  - MAX_FILE_SIZE: límite de seguridad; puede aumentarse o desactivarse lógicamente.
 *  - MAX_BUFFER_SIZE: valor orientativo. El límite real depende de /dev/shm y shmmax.
 */
#define MIN_BUFFER_SIZE 1
#define MAX_BUFFER_SIZE 100000000
#define MAX_FILE_SIZE   1073741824  // 1 GiB

// Semáforos POSIX nombrados (persisten en /dev/shm/sem.*)
#define SEM_NAME_GLOBAL_MUTEX   "/sem_global_mutex"
#define SEM_NAME_ENCRYPT_QUEUE  "/sem_encrypt_queue"
#define SEM_NAME_DECRYPT_QUEUE  "/sem_decrypt_queue"
#define SEM_NAME_ENCRYPT_SPACES "/sem_encrypt_spaces"
#define SEM_NAME_DECRYPT_ITEMS  "/sem_decrypt_items"

// Permisos para objetos IPC y archivos
#define IPC_PERMS 0666

// Estados de retorno
#define SUCCESS  0
#define ERROR   -1

// Macros útiles
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Tamaño de página:
 *  - Se consulta con sysconf(_SC_PAGESIZE) en tiempo de ejecución.
 *  - Esta constante se conserva para compatibilidad y mensajes.
 */
#define PAGE_SIZE 4096

#endif // CONSTANTS_H
