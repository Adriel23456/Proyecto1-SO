#ifndef CONSTANTS_H
#define CONSTANTS_H

// Claves de memoria compartida y semáforos
#define SHM_BASE_KEY 0x1234
#define SEM_BASE_KEY 0x5000

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
#define MODE_AUTO 0
#define MODE_MANUAL 1

// Tamaños y límites
#define MIN_BUFFER_SIZE 5
#define MAX_BUFFER_SIZE 1000
#define MAX_FILE_SIZE 1048576    // 1MB
#define MAX_EMISORES 100
#define MAX_RECEPTORES 100
#define DEFAULT_DELAY_MS 100

// Nombres de semáforos para System V
#define SEM_GLOBAL_MUTEX 0
#define SEM_ENCRYPT_QUEUE 1
#define SEM_DECRYPT_QUEUE 2
#define SEM_ENCRYPT_SPACES 3
#define SEM_DECRYPT_ITEMS 4
#define NUM_SEMAPHORES 5

// Permisos IPC
#define IPC_PERMS 0666

// Estados de retorno
#define SUCCESS 0
#define ERROR -1

// Macros útiles
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// Tamaño de página para alineación
#define PAGE_SIZE 4096

#endif // CONSTANTS_H