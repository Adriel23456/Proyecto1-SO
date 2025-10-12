#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <time.h>
#include <sys/types.h>

// Estructura para cada carácter en memoria compartida
typedef struct {
    unsigned char ascii_value;    // Valor ASCII (encriptado cuando esté en uso)
    int slot_index;               // Índice del slot (1, 2, 3, ..., estático)
    time_t timestamp;             // Hora de introducción (0 = vacío)
    int is_valid;                 // Flag de validez (0 = vacío, 1 = ocupado)
    int text_index;               // Índice en el texto original
    pid_t emisor_pid;             // PID del emisor que lo escribió
} CharacterSlot;

// Nodo para las colas
typedef struct QueueNode {
    int slot_index;               // Índice del slot en el buffer
    int text_index;               // Índice del texto (solo para QueueDeencript)
    struct QueueNode* next;       // Siguiente nodo
} QueueNode;

// Estructura de cola
typedef struct {
    QueueNode* front;             // Frente de la cola
    QueueNode* rear;              // Final de la cola
    int size;                     // Tamaño actual
    int max_size;                 // Tamaño máximo
} Queue;

// Estructura principal de memoria compartida
typedef struct {
    // Información del sistema
    int shm_id;                   // ID de memoria compartida
    int buffer_size;              // Tamaño del buffer circular
    unsigned char encryption_key; // Clave de encriptación XOR
    
    // Índices y contadores
    int current_txt_index;        // Índice actual del texto
    int total_chars_in_file;      // Total de caracteres en archivo
    int total_chars_processed;    // Total de caracteres procesados
    
    // Estadísticas
    int total_emisores;           // Total de emisores creados
    int active_emisores;          // Emisores activos actualmente
    int total_receptores;         // Total de receptores creados
    int active_receptores;        // Receptores activos actualmente
    
    // Control de finalización
    int shutdown_flag;            // Señal de cierre (0 = activo, 1 = finalizar)
    
    // Información del archivo
    char input_filename[256];     // Nombre del archivo de entrada
    int file_data_size;          // Tamaño del archivo binario
    
    // PIDs de procesos activos
    pid_t emisor_pids[100];       // PIDs de emisores activos
    pid_t receptor_pids[100];     // PIDs de receptores activos
    
    // Semáforos (usando índices para memoria compartida)
    int sem_global_mutex;         // Semáforo para acceso global
    int sem_encrypt_queue;        // Semáforo para cola de encriptación
    int sem_decrypt_queue;        // Semáforo para cola de desencriptación
    int sem_encrypt_spaces;       // Contador de espacios disponibles
    int sem_decrypt_items;        // Contador de items disponibles
    
    // Colas embebidas
    Queue encrypt_queue;          // Cola de posiciones disponibles
    Queue decrypt_queue;          // Cola de posiciones con datos
    
    // Offset donde comienza el buffer de caracteres
    size_t buffer_offset;         // Offset al buffer de CharacterSlot
    
    // Offset donde comienza el archivo binario
    size_t file_data_offset;      // Offset al contenido del archivo
    
    // Los datos dinámicos van al final (buffer + archivo binario)
    // CharacterSlot buffer[buffer_size];
    // unsigned char file_data[file_data_size];
    
} SharedMemory;

#endif // STRUCTURES_H