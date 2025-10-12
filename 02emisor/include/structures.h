#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <time.h>
#include <sys/types.h>

/*
 * Estructura para cada carácter en memoria compartida:
 *  - ascii_value: valor encriptado cuando el slot está en uso.
 *  - slot_index: índice fijo del slot [1..buffer_size].
 *  - timestamp: marca de tiempo de inserción (0 si vacío).
 *  - is_valid: 0 = vacío, 1 = ocupado.
 *  - text_index: índice del texto original.
 *  - emisor_pid: PID del emisor que escribió el slot.
 */
typedef struct {
    unsigned char ascii_value;
    int           slot_index;
    time_t        timestamp;
    int           is_valid;
    int           text_index;
    pid_t         emisor_pid;
} CharacterSlot;

/*
 * Referencia de slot para colas:
 *  - slot_index: índice del slot del buffer [0..buffer_size-1].
 *  - text_index: índice del texto (para decrypt); -1 cuando no aplica.
 */
typedef struct {
    int slot_index;
    int text_index;
} SlotRef;

/*
 * Cola en memoria compartida basada en ring buffer:
 *  - head/tail/size/capacity: estado de la cola (circular).
 *  - array_offset: offset dentro de la SHM del array SlotRef[capacity].
 */
typedef struct {
    int     head;
    int     tail;
    int     size;
    int     capacity;
    size_t  array_offset;
} Queue;

/*
 * Estructura principal de memoria compartida:
 *  - Contiene metadatos, colas y offsets para acceder a regiones dinámicas.
 *  - Los datos dinámicos van al final del segmento:
 *      [CharacterSlot buffer[buffer_size]]
 *      [unsigned char file_data[file_data_size]]
 *      [SlotRef encrypt_queue_array[buffer_size]]
 *      [SlotRef decrypt_queue_array[buffer_size]]
 */
typedef struct {
    // Información del sistema
    int            shm_id;
    int            buffer_size;
    unsigned char  encryption_key;

    // Índices y contadores
    int current_txt_index;
    int total_chars_in_file;
    int total_chars_processed;

    // Estadísticas
    int  total_emisores;
    int  active_emisores;
    int  total_receptores;
    int  active_receptores;

    // Control de finalización
    int  shutdown_flag;

    // Información del archivo
    char  input_filename[256];
    int   file_data_size;

    // PIDs de procesos activos (registro básico)
    pid_t emisor_pids[100];
    pid_t receptor_pids[100];

    // Índices de semáforos (no usados con POSIX, pero mantenidos por compatibilidad)
    int sem_global_mutex;
    int sem_encrypt_queue;
    int sem_decrypt_queue;
    int sem_encrypt_spaces;
    int sem_decrypt_items;

    // Colas embebidas
    Queue encrypt_queue;  // Slots disponibles para escribir
    Queue decrypt_queue;  // Slots con datos para leer

    // Offsets de regiones dinámicas
    size_t buffer_offset;     // Base de CharacterSlot[buffer_size]
    size_t file_data_offset;  // Base de file_data[file_data_size]
    // Los arrays de colas se ubican por array_offset en cada Queue

} SharedMemory;

#endif // STRUCTURES_H