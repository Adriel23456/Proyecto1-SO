# 🚀 Roadmap - Sistema de Comunicación entre Procesos con Memoria Compartida

## 📋 Resumen Ejecutivo

Sistema compuesto por **4 PROGRAMAS INDEPENDIENTES** que se comunican y sincronizan mediante memoria compartida, implementando un esquema productor-consumidor sin busy waiting, con encriptación XOR y garantía de orden secuencial.

---

## 🏗️ Arquitectura del Sistema

### Programas Independientes
```
┌─────────────────────────────────────────────────────────────┐
│                    MEMORIA COMPARTIDA (IPC)                 │
│  ┌──────────┬──────────┬───────────┬──────────────────┐   │
│  │ Buffers  │  Colas   │ Semáforos │ Metadatos       │   │
│  └──────────┴──────────┴───────────┴──────────────────┘   │
└─────────────────────────────────────────────────────────────┘
        ▲              ▲              ▲              ▲
        │              │              │              │
   [Programa 1]   [Programa 2]   [Programa 3]   [Programa 4]
  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐
  │Inicializador│ │   Emisor   │ │  Receptor  │ │Finalizador │
  └────────────┘ └────────────┘ └────────────┘ └────────────┘
       │              ▲              ▼              │
       │         input.txt      output.txt         │
       │                                           │
   [Ejecuta una vez]  [N instancias]  [N instancias]  [Control]
```

---

## 📁 Estructura de Directorios del Proyecto

```
proyecto_sincronizacion_so/
│
├── inicializador/
│   ├── src/
│   │   ├── main.c
│   │   ├── shared_memory_init.c
│   │   └── shared_memory_init.h
│   ├── include/
│   │   ├── constants.h
│   │   └── structures.h
│   ├── data/
│   │   └── input.txt
│   ├── Makefile
│   └── README.md
│
├── emisor/
│   ├── src/
│   │   ├── main.c
│   │   ├── encoder.c
│   │   ├── encoder.h
│   │   ├── shared_memory_access.c
│   │   └── shared_memory_access.h
│   ├── include/
│   │   ├── constants.h
│   │   ├── structures.h
│   │   └── colors.h
│   ├── Makefile
│   └── README.md
│
├── receptor/
│   ├── src/
│   │   ├── main.c
│   │   ├── decoder.c
│   │   ├── decoder.h
│   │   ├── shared_memory_access.c
│   │   └── shared_memory_access.h
│   ├── include/
│   │   ├── constants.h
│   │   ├── structures.h
│   │   └── colors.h
│   ├── data/
│   │   └── output.txt
│   ├── Makefile
│   └── README.md
│
├── finalizador/
│   ├── src/
│   │   ├── main.c
│   │   ├── statistics.c
│   │   ├── statistics.h
│   │   ├── shared_memory_cleanup.c
│   │   └── shared_memory_cleanup.h
│   ├── include/
│   │   ├── constants.h
│   │   ├── structures.h
│   │   └── colors.h
│   ├── Makefile
│   └── README.md
│
├── shared/
│   ├── constants.h      # Constantes compartidas entre todos
│   └── structures.h     # Estructuras compartidas entre todos
│
└── README_GENERAL.md
```

---

## 🔧 Estructuras de Datos Compartidas

### **shared/structures.h** - Copiado en cada programa

```c
#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H

#include <semaphore.h>
#include <time.h>

// Clave de memoria compartida
#define SHM_KEY 0x1234
#define SEM_KEY_BASE 0x5000

// Estructura para cada carácter en memoria compartida
typedef struct {
    char ascii_value;        // Valor ASCII encriptado
    int text_index;         // Índice en el texto original
    int memory_index;       // Índice en memoria compartida
    time_t timestamp;       // Hora de introducción
    pid_t emisor_pid;       // PID del emisor que lo escribió
    int is_valid;          // Flag de validez (0=vacío, 1=ocupado)
} CharacterSlot;

// Estructura de nodo para las colas
typedef struct QueueNode {
    int slot_index;         // Índice del slot en el buffer
    int text_index;         // Índice del texto
    struct QueueNode* next;
} QueueNode;

// Estructura de cola
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
    int max_size;
} Queue;

// Estructura de memoria compartida global
typedef struct {
    // Semáforos (usando nombres para IPC)
    char sem_global_mutex[32];
    char sem_encrypt_queue[32];
    char sem_decrypt_queue[32];
    char sem_encrypt_spaces[32];
    char sem_decrypt_items[32];
    
    // Índices y contadores
    int current_txt_index;        // Índice actual del texto
    int total_chars_in_file;      // Total de caracteres en archivo
    int total_chars_processed;    // Total de caracteres procesados
    int buffer_size;              // Tamaño del buffer circular
    
    // Estadísticas
    int total_emisores;
    int active_emisores;
    int total_receptores;
    int active_receptores;
    pid_t emisor_pids[100];       // PIDs de emisores activos
    pid_t receptor_pids[100];     // PIDs de receptores activos
    
    // Colas embebidas en memoria compartida
    Queue encrypt_queue;          // Cola de posiciones disponibles
    Queue decrypt_queue;          // Cola de posiciones con datos
    
    // Control de finalización
    int shutdown_flag;            // Señal de cierre
    
    // Información del archivo
    char input_filename[256];     // Nombre del archivo de entrada
    
    // Clave de encriptación
    unsigned char encryption_key;
    
    // Buffer de caracteres (al final para alineación)
    CharacterSlot buffer[];       // Array flexible de slots
    
} SharedMemory;

// Constantes compartidas
#define MAX_BUFFER_SIZE 1000
#define MAX_EMISORES 100
#define MAX_RECEPTORES 100
#define DEFAULT_DELAY_MS 100

#endif
```

### **shared/constants.h** - Constantes comunes

```c
#ifndef SHARED_CONSTANTS_H
#define SHARED_CONSTANTS_H

// Colores para output
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

// Modos de ejecución
#define MODE_AUTO 0
#define MODE_MANUAL 1

// Tamaños y límites
#define MIN_BUFFER_SIZE 5
#define MAX_FILE_SIZE 1048576  // 1MB

// Nombres de semáforos
#define SEM_GLOBAL_MUTEX "/sem_global_mutex"
#define SEM_ENCRYPT_QUEUE "/sem_encrypt_queue"
#define SEM_DECRYPT_QUEUE "/sem_decrypt_queue"
#define SEM_ENCRYPT_SPACES "/sem_encrypt_spaces"
#define SEM_DECRYPT_ITEMS "/sem_decrypt_items"

#endif
```

---

## 📝 Implementación por Programa

## 🔵 **PROGRAMA 1: INICIALIZADOR**

### Estructura del Proyecto
```
inicializador/
├── src/
│   ├── main.c
│   ├── shared_memory_init.c
│   └── shared_memory_init.h
├── include/
│   ├── constants.h (copia de shared/)
│   └── structures.h (copia de shared/)
├── Makefile
└── README.md
```

### **main.c** - Punto de entrada
```c
int main(int argc, char* argv[]) {
    // Validar argumentos
    if (argc != 5) {
        printf("Uso: %s <shm_id> <buffer_size> <encryption_key> <input_file>\n", argv[0]);
        exit(1);
    }
    
    int shm_id = atoi(argv[1]);
    int buffer_size = atoi(argv[2]);
    unsigned char key = (unsigned char)strtol(argv[3], NULL, 16);
    char* input_file = argv[4];
    
    // Crear y configurar memoria compartida
    SharedMemory* shm = create_shared_memory(shm_id, buffer_size);
    
    // Inicializar semáforos
    initialize_semaphores(shm, buffer_size);
    
    // Configurar colas
    setup_queues(shm, buffer_size);
    
    // Configurar parámetros
    shm->encryption_key = key;
    strcpy(shm->input_filename, input_file);
    shm->total_chars_in_file = count_file_chars(input_file);
    
    printf(GREEN "[INICIALIZADOR] Sistema configurado exitosamente\n" RESET);
    printf("- Memoria compartida ID: %d\n", shm_id);
    printf("- Buffer size: %d\n", buffer_size);
    printf("- Archivo: %s (%d caracteres)\n", input_file, shm->total_chars_in_file);
    printf("- Clave: 0x%02X\n", key);
    
    // NO liberar memoria - otros procesos la usarán
    return 0;
}
```

### **Makefile**
```makefile
CC = gcc
CFLAGS = -Wall -pthread -lrt -g
SRCDIR = src
INCDIR = include
BINDIR = bin

TARGET = inicializador

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) $(OBJECTS) -o $(BINDIR)/$(TARGET) $(CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(BINDIR)/$(TARGET)

run: $(TARGET)
	./$(BINDIR)/$(TARGET) 1234 20 0xAB ../shared/input.txt
```

---

## 🟢 **PROGRAMA 2: EMISOR**

### Estructura del Proyecto
```
emisor/
├── src/
│   ├── main.c
│   ├── encoder.c
│   ├── encoder.h
│   ├── shared_memory_access.c
│   └── shared_memory_access.h
├── include/
│   ├── constants.h
│   ├── structures.h
│   └── colors.h
├── Makefile
└── README.md
```

### **main.c** - Punto de entrada
```c
int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        printf("Uso: %s <auto|manual> [encryption_key]\n", argv[0]);
        exit(1);
    }
    
    int mode = strcmp(argv[1], "auto") == 0 ? MODE_AUTO : MODE_MANUAL;
    
    // Conectar a memoria compartida existente
    SharedMemory* shm = attach_shared_memory(SHM_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a memoria compartida\n" RESET);
        exit(1);
    }
    
    // Abrir semáforos existentes
    sem_t* sem_global = sem_open(SEM_GLOBAL_MUTEX, 0);
    sem_t* sem_encrypt_spaces = sem_open(SEM_ENCRYPT_SPACES, 0);
    sem_t* sem_decrypt_items = sem_open(SEM_DECRYPT_ITEMS, 0);
    
    // Registrar emisor
    register_emisor(shm, getpid());
    
    // Abrir archivo de entrada
    FILE* input_file = fopen(shm->input_filename, "r");
    
    // Bucle principal de emisión
    while (!shm->shutdown_flag) {
        // Obtener siguiente índice a procesar
        int txt_index = atomic_get_next_index(shm, sem_global);
        
        if (txt_index >= shm->total_chars_in_file) {
            break; // Fin del archivo
        }
        
        // Leer carácter del archivo
        char ch = read_char_at_position(input_file, txt_index);
        
        // Esperar espacio disponible (BLOQUEO SIN BUSY WAITING)
        sem_wait(sem_encrypt_spaces);
        
        // Obtener slot disponible
        int slot_index = dequeue_encrypt_slot(shm);
        
        // Encriptar y almacenar
        process_character(shm, slot_index, ch, txt_index);
        
        // Notificar dato disponible
        enqueue_decrypt_slot(shm, slot_index, txt_index);
        sem_post(sem_decrypt_items);
        
        // Control de modo
        if (mode == MODE_MANUAL) {
            printf("Presione ENTER para continuar...");
            getchar();
        } else {
            usleep(DEFAULT_DELAY_MS * 1000);
        }
    }
    
    // Desregistrar emisor
    unregister_emisor(shm, getpid());
    
    printf(YELLOW "[EMISOR %d] Finalizado\n" RESET, getpid());
    return 0;
}
```

### **encoder.c** - Lógica de encriptación
```c
void process_character(SharedMemory* shm, int slot_index, char ch, int txt_index) {
    CharacterSlot* slot = &shm->buffer[slot_index];
    
    // Encriptar
    slot->ascii_value = ch ^ shm->encryption_key;
    slot->text_index = txt_index;
    slot->memory_index = slot_index;
    slot->timestamp = time(NULL);
    slot->emisor_pid = getpid();
    slot->is_valid = 1;
    
    // Imprimir estado
    print_emission_status(slot, ch);
}

void print_emission_status(CharacterSlot* slot, char original) {
    struct tm* timeinfo = localtime(&slot->timestamp);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
    
    printf(GREEN "╔════════════════════════════════════════╗\n" RESET);
    printf(GREEN "║ EMISOR PID: %-6d                     ║\n" RESET, getpid());
    printf(GREEN "╠════════════════════════════════════════╣\n" RESET);
    printf(GREEN "║ Char: '%c' → 0x%02X (encriptado)       ║\n" RESET, 
           original, (unsigned char)slot->ascii_value);
    printf(GREEN "║ Índice texto: %-4d                     ║\n" RESET, slot->text_index);
    printf(GREEN "║ Slot memoria: %-4d                     ║\n" RESET, slot->memory_index);
    printf(GREEN "║ Hora: %s                      ║\n" RESET, time_buffer);
    printf(GREEN "╚════════════════════════════════════════╝\n" RESET);
}
```

### **Makefile**
```makefile
CC = gcc
CFLAGS = -Wall -pthread -lrt -g
SRCDIR = src
INCDIR = include
BINDIR = bin

TARGET = emisor

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) $(OBJECTS) -o $(BINDIR)/$(TARGET) $(CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(BINDIR)/$(TARGET)

run-auto: $(TARGET)
	./$(BINDIR)/$(TARGET) auto

run-manual: $(TARGET)
	./$(BINDIR)/$(TARGET) manual
```

---

## 🔴 **PROGRAMA 3: RECEPTOR**

### Estructura del Proyecto
```
receptor/
├── src/
│   ├── main.c
│   ├── decoder.c
│   ├── decoder.h
│   ├── shared_memory_access.c
│   └── shared_memory_access.h
├── include/
│   ├── constants.h
│   ├── structures.h
│   └── colors.h
├── data/
│   └── output.txt
├── Makefile
└── README.md
```

### **main.c** - Punto de entrada
```c
int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        printf("Uso: %s <auto|manual> [decryption_key]\n", argv[0]);
        exit(1);
    }
    
    int mode = strcmp(argv[1], "auto") == 0 ? MODE_AUTO : MODE_MANUAL;
    
    // Conectar a memoria compartida
    SharedMemory* shm = attach_shared_memory(SHM_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a memoria compartida\n" RESET);
        exit(1);
    }
    
    // Abrir semáforos
    sem_t* sem_decrypt_items = sem_open(SEM_DECRYPT_ITEMS, 0);
    sem_t* sem_encrypt_spaces = sem_open(SEM_ENCRYPT_SPACES, 0);
    
    // Registrar receptor
    register_receptor(shm, getpid());
    
    // Buffer para reconstruir texto
    char* output_buffer = calloc(shm->total_chars_in_file + 1, sizeof(char));
    int chars_received = 0;
    
    // Abrir archivo de salida
    FILE* output_file = fopen("data/output.txt", "w");
    
    // Array para rastrear caracteres recibidos
    int* received_indices = calloc(shm->total_chars_in_file, sizeof(int));
    
    while (!shm->shutdown_flag && chars_received < shm->total_chars_in_file) {
        // Esperar datos disponibles (BLOQUEO SIN BUSY WAITING)
        if (sem_trywait(sem_decrypt_items) != 0) {
            if (shm->shutdown_flag) break;
            sem_wait(sem_decrypt_items);
        }
        
        // Obtener siguiente slot con datos
        SlotInfo info = dequeue_decrypt_slot_ordered(shm);
        
        if (info.slot_index == -1) continue;
        
        // Procesar carácter
        CharacterSlot* slot = &shm->buffer[info.slot_index];
        char decrypted = slot->ascii_value ^ shm->encryption_key;
        
        // Almacenar en posición correcta
        output_buffer[slot->text_index] = decrypted;
        received_indices[slot->text_index] = 1;
        chars_received++;
        
        // Imprimir estado
        print_reception_status(slot, decrypted);
        
        // Marcar slot como disponible
        slot->is_valid = 0;
        enqueue_encrypt_slot(shm, info.slot_index);
        sem_post(sem_encrypt_spaces);
        
        // Escribir a archivo si tenemos secuencia continua
        write_continuous_sequence(output_file, output_buffer, received_indices);
        
        // Control de modo
        if (mode == MODE_MANUAL) {
            printf("Presione ENTER para continuar...");
            getchar();
        } else {
            usleep(DEFAULT_DELAY_MS * 1000);
        }
    }
    
    // Escribir resto del buffer
    fprintf(output_file, "%s", output_buffer);
    fclose(output_file);
    
    // Desregistrar receptor
    unregister_receptor(shm, getpid());
    
    printf(CYAN "[RECEPTOR %d] Finalizado. Texto guardado en output.txt\n" RESET, getpid());
    
    free(output_buffer);
    free(received_indices);
    
    return 0;
}
```

### **decoder.c** - Lógica de decodificación
```c
void print_reception_status(CharacterSlot* slot, char decrypted) {
    struct tm* timeinfo = localtime(&slot->timestamp);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
    
    printf(CYAN "╔════════════════════════════════════════╗\n" RESET);
    printf(CYAN "║ RECEPTOR PID: %-6d                   ║\n" RESET, getpid());
    printf(CYAN "╠════════════════════════════════════════╣\n" RESET);
    printf(CYAN "║ Char desencriptado: '%c'               ║\n" RESET, decrypted);
    printf(CYAN "║ Valor encriptado: 0x%02X                ║\n" RESET, 
           (unsigned char)slot->ascii_value);
    printf(CYAN "║ Índice texto: %-4d                     ║\n" RESET, slot->text_index);
    printf(CYAN "║ Slot leído: %-4d                       ║\n" RESET, slot->memory_index);
    printf(CYAN "║ Emisor PID: %-6d                     ║\n" RESET, slot->emisor_pid);
    printf(CYAN "║ Hora emisión: %s              ║\n" RESET, time_buffer);
    printf(CYAN "╚════════════════════════════════════════╝\n" RESET);
}

SlotInfo dequeue_decrypt_slot_ordered(SharedMemory* shm) {
    // Buscar el slot con el índice de texto más bajo disponible
    sem_wait(sem_decrypt_queue);
    
    SlotInfo result = {-1, -1};
    int min_text_index = INT_MAX;
    int best_slot = -1;
    
    QueueNode* current = shm->decrypt_queue.front;
    QueueNode* prev = NULL;
    QueueNode* best_prev = NULL;
    
    // Buscar el nodo con menor text_index
    while (current != NULL) {
        if (current->text_index < min_text_index) {
            min_text_index = current->text_index;
            best_slot = current->slot_index;
            best_prev = prev;
        }
        prev = current;
        current = current->next;
    }
    
    // Remover el nodo encontrado
    if (best_slot != -1) {
        result.slot_index = best_slot;
        result.text_index = min_text_index;
        
        // Remover de la cola
        QueueNode* to_remove;
        if (best_prev == NULL) {
            to_remove = shm->decrypt_queue.front;
            shm->decrypt_queue.front = to_remove->next;
        } else {
            to_remove = best_prev->next;
            best_prev->next = to_remove->next;
        }
        
        if (to_remove == shm->decrypt_queue.rear) {
            shm->decrypt_queue.rear = best_prev;
        }
        
        shm->decrypt_queue.size--;
    }
    
    sem_post(sem_decrypt_queue);
    return result;
}
```

---

## 🟣 **PROGRAMA 4: FINALIZADOR**

### Estructura del Proyecto
```
finalizador/
├── src/
│   ├── main.c
│   ├── statistics.c
│   ├── statistics.h
│   ├── shared_memory_cleanup.c
│   └── shared_memory_cleanup.h
├── include/
│   ├── constants.h
│   ├── structures.h
│   └── colors.h
├── Makefile
└── README.md
```

### **main.c** - Punto de entrada
```c
volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int sig) {
    shutdown_requested = 1;
}

int main(int argc, char* argv[]) {
    // Configurar manejadores de señales
    signal(SIGINT, signal_handler);  // Ctrl+C
    signal(SIGUSR1, signal_handler); // Señal personalizada
    
    printf(MAGENTA "[FINALIZADOR] Iniciado. Presione Ctrl+C para finalizar sistema\n" RESET);
    
    // Conectar a memoria compartida
    SharedMemory* shm = attach_shared_memory(SHM_KEY);
    if (!shm) {
        fprintf(stderr, RED "[ERROR] No se pudo conectar a memoria compartida\n" RESET);
        exit(1);
    }
    
    // Esperar señal de finalización
    while (!shutdown_requested) {
        sleep(1);
        
        // Opcionalmente, mostrar estado periódico
        if (time(NULL) % 10 == 0) {
            print_current_status(shm);
        }
    }
    
    printf(MAGENTA "\n[FINALIZADOR] Señal recibida. Iniciando cierre ordenado...\n" RESET);
    
    // Activar flag de shutdown
    shm->shutdown_flag = 1;
    
    // Despertar todos los procesos bloqueados
    sem_t* sem_encrypt_spaces = sem_open(SEM_ENCRYPT_SPACES, 0);
    sem_t* sem_decrypt_items = sem_open(SEM_DECRYPT_ITEMS, 0);
    
    for (int i = 0; i < shm->buffer_size; i++) {
        sem_post(sem_encrypt_spaces);
        sem_post(sem_decrypt_items);
    }
    
    // Enviar SIGUSR1 a todos los procesos registrados
    send_signal_to_all_processes(shm);
    
    // Esperar que todos los procesos terminen
    printf(MAGENTA "[FINALIZADOR] Esperando que terminen todos los procesos...\n" RESET);
    wait_for_processes_to_finish(shm);
    
    // Recopilar y mostrar estadísticas finales
    print_final_statistics(shm);
    
    // Limpiar recursos
    cleanup_all_resources(shm);
    
    printf(MAGENTA "[FINALIZADOR] Sistema cerrado correctamente\n" RESET);
    
    return 0;
}
```

### **statistics.c** - Estadísticas finales
```c
void print_final_statistics(SharedMemory* shm) {
    printf("\n");
    printf(MAGENTA "╔════════════════════════════════════════════╗\n" RESET);
    printf(MAGENTA "║          ESTADÍSTICAS FINALES              ║\n" RESET);
    printf(MAGENTA "╠════════════════════════════════════════════╣\n" RESET);
    printf(MAGENTA "║ Caracteres totales en archivo: %-6d     ║\n" RESET, 
           shm->total_chars_in_file);
    printf(MAGENTA "║ Caracteres procesados: %-6d             ║\n" RESET, 
           shm->total_chars_processed);
    printf(MAGENTA "║ Caracteres en memoria: %-6d             ║\n" RESET, 
           count_valid_slots(shm));
    printf(MAGENTA "╠════════════════════════════════════════════╣\n" RESET);
    printf(MAGENTA "║ Emisores totales: %-3d                     ║\n" RESET, 
           shm->total_emisores);
    printf(MAGENTA "║ Emisores activos al cierre: %-3d          ║\n" RESET, 
           shm->active_emisores);
    printf(MAGENTA "║ Receptores totales: %-3d                   ║\n" RESET, 
           shm->total_receptores);
    printf(MAGENTA "║ Receptores activos al cierre: %-3d        ║\n" RESET, 
           shm->active_receptores);
    printf(MAGENTA "╠════════════════════════════════════════════╣\n" RESET);
    printf(MAGENTA "║ Tamaño buffer: %-6d slots               ║\n" RESET, 
           shm->buffer_size);
    printf(MAGENTA "║ Memoria utilizada: %-6ld bytes           ║\n" RESET, 
           sizeof(SharedMemory) + shm->buffer_size * sizeof(CharacterSlot));
    printf(MAGENTA "╚════════════════════════════════════════════╝\n" RESET);
}
```

---

## 🚦 Flujo de Ejecución del Sistema

### Secuencia de Comandos:
```bash
# Terminal 1 - Inicializar sistema
cd inicializador/bin
./inicializador 1234 20 0xAB ../shared/input.txt

# Terminal 2 - Primer emisor
cd emisor/bin
./emisor auto

# Terminal 3 - Segundo emisor  
cd emisor/bin
./emisor manual

# Terminal 4 - Primer receptor
cd receptor/bin
./receptor auto

# Terminal 5 - Segundo receptor
cd receptor/bin
./receptor manual

# Terminal 6 - Finalizador
cd finalizador/bin
./finalizador
# Presionar Ctrl+C cuando se desee finalizar todo
```

---

## ⚡ Sincronización Sin Busy Waiting

### Mecanismo de Bloqueo con Semáforos POSIX:

```c
// EMISOR - Se bloquea cuando no hay espacio
sem_wait(sem_encrypt_spaces);  // El kernel suspende el proceso si valor = 0
// ... proceso slot ...
sem_post(sem_decrypt_items);   // Despierta un receptor bloqueado

// RECEPTOR - Se bloquea cuando no hay datos
sem_wait(sem_decrypt_items);   // El kernel suspende el proceso si valor = 0  
// ... proceso slot ...
sem_post(sem_encrypt_spaces);  // Despierta un emisor bloqueado
```

**Ventajas:**
- El proceso NO consume CPU mientras espera
- El kernel maneja la cola de procesos en espera
- Despertar es automático cuando hay recursos

---

## 🎯 Garantía de Orden Secuencial

### Estrategia de Ordenamiento:

1. **Índice Global Compartido**: Cada emisor toma el siguiente índice atómicamente
2. **Cola con Índices**: Los slots en decrypt_queue mantienen el text_index
3. **Dequeue Ordenado**: El receptor siempre toma el menor text_index disponible
4. **Buffer de Reconstrucción**: El receptor ensambla el texto en orden correcto

---

## ⚠️ Consideraciones Críticas

### Para Cada Programa:

1. **Validación de Memoria Compartida**
   - Verificar que existe antes de conectar
   - Manejar errores de conexión

2. **Manejo de Señales**
   - No usar `kill()` para finalización
   - Usar flags y semáforos para coordinación

3. **Prevención de Deadlocks**
   - Orden consistente de adquisición de semáforos
   - Timeouts en operaciones críticas

4. **Limpieza de Recursos**
   - Solo el finalizador libera memoria compartida
   - Cada proceso se desregistra al salir

---

## 📚 Compilación Global

### Script de compilación para todos los programas:
```bash
#!/bin/bash
# build_all.sh

echo "Compilando Inicializador..."
cd inicializador && make clean && make

echo "Compilando Emisor..."
cd ../emisor && make clean && make

echo "Compilando Receptor..."
cd ../receptor && make clean && make

echo "Compilando Finalizador..."
cd ../finalizador && make clean && make

echo "Compilación completa!"
```

---

## ✅ Checklist de Implementación

### Por Programa:
- [ ] **Inicializador**
  - [ ] Crear memoria compartida con key fijo
  - [ ] Inicializar semáforos nombrados
  - [ ] Configurar colas en memoria compartida
  - [ ] Validar archivo de entrada

- [ ] **Emisor**
  - [ ] Conectar a memoria compartida existente
  - [ ] Abrir semáforos nombrados
  - [ ] Implementar lectura ordenada del archivo
  - [ ] Encriptación XOR
  - [ ] Modo automático y manual

- [ ] **Receptor** 
  - [ ] Conectar a memoria compartida existente
  - [ ] Abrir semáforos nombrados
  - [ ] Implementar dequeue ordenado
  - [ ] Desencriptación XOR
  - [ ] Escritura a archivo de salida

- [ ] **Finalizador**
  - [ ] Captura de señales (SIGINT)
  - [ ] Activación de flag shutdown
  - [ ] Despertar procesos bloqueados
  - [ ] Recopilación de estadísticas
  - [ ] Limpieza total de recursos

### Pruebas del Sistema:
- [ ] Ejecución con 1 emisor y 1 receptor
- [ ] Ejecución con múltiples emisores
- [ ] Ejecución con múltiples receptores
- [ ] Verificación de orden en output
- [ ] Prueba de finalización ordenada
- [ ] Verificación sin memory leaks (valgrind)