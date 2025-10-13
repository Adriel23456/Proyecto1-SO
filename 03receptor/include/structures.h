#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <time.h>
#include <sys/types.h>

typedef struct {
    unsigned char ascii_value;
    int           slot_index;
    time_t        timestamp;
    int           is_valid;
    int           text_index;
    pid_t         emisor_pid;
} CharacterSlot;

typedef struct {
    int slot_index;
    int text_index;
} SlotRef;

typedef struct {
    int     head;
    int     tail;
    int     size;
    int     capacity;
    size_t  array_offset;
} Queue;

// NUEVO: Estructura para estadísticas de procesos finalizados
typedef struct {
    pid_t  pid;
    int    chars_processed;
    time_t start_time;
    time_t end_time;
} ProcessStats;

typedef struct {
    int            shm_id;
    int            buffer_size;
    unsigned char  encryption_key;

    int current_txt_index;
    int total_chars_in_file;
    int total_chars_processed;

    int  total_emisores;
    int  active_emisores;
    int  total_receptores;
    int  active_receptores;

    int  shutdown_flag;

    char  input_filename[256];
    int   file_data_size;

    pid_t emisor_pids[100];
    pid_t receptor_pids[100];

    // NUEVO: Arrays para estadísticas de procesos finalizados
    ProcessStats emisor_stats[100];
    ProcessStats receptor_stats[100];
    int emisor_stats_count;
    int receptor_stats_count;

    int sem_global_mutex;
    int sem_encrypt_queue;
    int sem_decrypt_queue;
    int sem_encrypt_spaces;
    int sem_decrypt_items;

    Queue encrypt_queue;
    Queue decrypt_queue;

    size_t buffer_offset;
    size_t file_data_offset;

} SharedMemory;

#endif // STRUCTURES_H