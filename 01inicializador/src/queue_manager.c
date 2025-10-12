#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue_manager.h"
#include "constants.h"
#include "structures.h"

// Inicializar las colas en memoria compartida
void initialize_queues(SharedMemory* shm, int buffer_size) {
    // Inicializar la cola de encriptación
    initialize_encrypt_queue(shm, buffer_size);
    
    // Inicializar la cola de desencriptación
    initialize_decrypt_queue(shm);
    
    printf("  • Estado de las colas:\n");
    printf("    - QueueEncript: %d posiciones disponibles\n", shm->encrypt_queue.size);
    printf("    - QueueDeencript: %d elementos (vacía)\n", shm->decrypt_queue.size);
}

// Inicializar la cola de encriptación con todas las posiciones disponibles
void initialize_encrypt_queue(SharedMemory* shm, int buffer_size) {
    Queue* queue = &shm->encrypt_queue;
    
    // Inicializar la estructura de la cola
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    queue->max_size = buffer_size;
    
    // Para este ejemplo, vamos a usar un pool de nodos estáticos
    // En lugar de malloc (que no funciona bien en memoria compartida),
    // reservamos espacio después de SharedMemory para los nodos
    
    // Obtener puntero al área de nodos (después del buffer y archivo)
    size_t nodes_offset = shm->file_data_offset + shm->file_data_size;
    QueueNode* nodes_pool = (QueueNode*)((char*)shm + nodes_offset);
    
    // Llenar la cola con todas las posiciones disponibles
    for (int i = 0; i < buffer_size; i++) {
        QueueNode* new_node = &nodes_pool[i];
        new_node->slot_index = i;  // Índice del slot (0-based)
        new_node->text_index = -1;  // No usado en cola de encriptación
        new_node->next = NULL;
        
        if (queue->front == NULL) {
            queue->front = new_node;
            queue->rear = new_node;
        } else {
            queue->rear->next = new_node;
            queue->rear = new_node;
        }
        queue->size++;
    }
    
    // Mostrar algunas posiciones iniciales
    printf("  • Cola de encriptación inicializada:\n");
    QueueNode* current = queue->front;
    printf("    Slots disponibles: ");
    for (int i = 0; i < MIN(5, buffer_size); i++) {
        if (current != NULL) {
            printf("%d ", current->slot_index);
            current = current->next;
        }
    }
    if (buffer_size > 5) {
        printf("... (%d total)", buffer_size);
    }
    printf("\n");
}

// Inicializar la cola de desencriptación (vacía)
void initialize_decrypt_queue(SharedMemory* shm) {
    Queue* queue = &shm->decrypt_queue;
    
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    queue->max_size = shm->buffer_size;
    
    printf("  • Cola de desencriptación inicializada (vacía)\n");
}

// Enqueue para la cola de encriptación (agregar slot disponible)
int enqueue_encrypt_slot(SharedMemory* shm, int slot_index) {
    Queue* queue = &shm->encrypt_queue;
    
    if (queue->size >= queue->max_size) {
        return ERROR;  // Cola llena
    }
    
    // Obtener un nodo del pool
    size_t nodes_offset = shm->file_data_offset + shm->file_data_size;
    QueueNode* nodes_pool = (QueueNode*)((char*)shm + nodes_offset);
    
    // Buscar un nodo libre (usamos el índice del buffer_size para los nodos de decrypt)
    int node_index = shm->buffer_size + queue->size;
    QueueNode* new_node = &nodes_pool[node_index];
    
    new_node->slot_index = slot_index;
    new_node->text_index = -1;
    new_node->next = NULL;
    
    if (queue->rear == NULL) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
    
    queue->size++;
    return SUCCESS;
}

// Dequeue para la cola de encriptación (obtener slot disponible)
int dequeue_encrypt_slot(SharedMemory* shm) {
    Queue* queue = &shm->encrypt_queue;
    
    if (queue->front == NULL) {
        return -1;  // Cola vacía
    }
    
    QueueNode* node_to_remove = queue->front;
    int slot_index = node_to_remove->slot_index;
    
    queue->front = node_to_remove->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    
    queue->size--;
    
    // No liberamos memoria porque está en el pool
    node_to_remove->next = NULL;
    
    return slot_index;
}

// Enqueue para la cola de desencriptación (agregar elemento con datos)
int enqueue_decrypt_slot(SharedMemory* shm, int slot_index, int text_index) {
    Queue* queue = &shm->decrypt_queue;
    
    if (queue->size >= queue->max_size) {
        return ERROR;  // Cola llena
    }
    
    // Obtener un nodo del pool (usamos la segunda mitad para decrypt)
    size_t nodes_offset = shm->file_data_offset + shm->file_data_size;
    QueueNode* nodes_pool = (QueueNode*)((char*)shm + nodes_offset);
    
    // Los nodos para decrypt empiezan después de los de encrypt
    int node_index = shm->buffer_size * 2 + queue->size;
    QueueNode* new_node = &nodes_pool[node_index];
    
    new_node->slot_index = slot_index;
    new_node->text_index = text_index;
    new_node->next = NULL;
    
    if (queue->rear == NULL) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
    
    queue->size++;
    return SUCCESS;
}

// Dequeue para la cola de desencriptación (obtener elemento con datos)
SlotInfo dequeue_decrypt_slot(SharedMemory* shm) {
    Queue* queue = &shm->decrypt_queue;
    SlotInfo info = {-1, -1};
    
    if (queue->front == NULL) {
        return info;  // Cola vacía
    }
    
    QueueNode* node_to_remove = queue->front;
    info.slot_index = node_to_remove->slot_index;
    info.text_index = node_to_remove->text_index;
    
    queue->front = node_to_remove->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    
    queue->size--;
    
    // No liberamos memoria porque está en el pool
    node_to_remove->next = NULL;
    
    return info;
}

// Dequeue ordenado para mantener secuencialidad (para receptores)
SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm) {
    Queue* queue = &shm->decrypt_queue;
    SlotInfo info = {-1, -1};
    
    if (queue->front == NULL) {
        return info;  // Cola vacía
    }
    
    // Buscar el nodo con el text_index más bajo
    QueueNode* current = queue->front;
    QueueNode* prev = NULL;
    QueueNode* min_prev = NULL;
    QueueNode* min_node = queue->front;
    int min_text_index = min_node->text_index;
    
    // Recorrer la cola buscando el mínimo
    while (current != NULL) {
        if (current->text_index < min_text_index) {
            min_text_index = current->text_index;
            min_node = current;
            min_prev = prev;
        }
        prev = current;
        current = current->next;
    }
    
    // Extraer el nodo con menor text_index
    info.slot_index = min_node->slot_index;
    info.text_index = min_node->text_index;
    
    // Remover el nodo de la cola
    if (min_prev == NULL) {
        // El mínimo es el primero
        queue->front = min_node->next;
    } else {
        min_prev->next = min_node->next;
    }
    
    if (min_node == queue->rear) {
        queue->rear = min_prev;
    }
    
    queue->size--;
    min_node->next = NULL;
    
    return info;
}

// Obtener el estado actual de las colas
void print_queue_status(SharedMemory* shm) {
    printf("Estado de las colas:\n");
    printf("  • QueueEncript: %d/%d slots disponibles\n", 
           shm->encrypt_queue.size, shm->encrypt_queue.max_size);
    printf("  • QueueDeencript: %d/%d elementos con datos\n", 
           shm->decrypt_queue.size, shm->decrypt_queue.max_size);
}

// Verificar si la cola de encriptación está vacía
int is_encrypt_queue_empty(SharedMemory* shm) {
    return shm->encrypt_queue.size == 0;
}

// Verificar si la cola de desencriptación está vacía
int is_decrypt_queue_empty(SharedMemory* shm) {
    return shm->decrypt_queue.size == 0;
}