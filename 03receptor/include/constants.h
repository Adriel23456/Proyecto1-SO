#ifndef CONSTANTS_H
#define CONSTANTS_H

// Clave de memoria compartida (System V SHM)
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

// Delays para modo automático
// Cambios: DEFAULT_DELAY_MS = 0 (máxima velocidad por omisión)
//          MIN_DELAY_MS = 0 (permitir desactivar slowdown explícitamente)
#define DEFAULT_DELAY_MS 0
#define MIN_DELAY_MS     0
#define MAX_DELAY_MS     5000

// Macros útiles
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif // CONSTANTS_H