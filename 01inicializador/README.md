# 🚀 Inicializador - Sistema de Comunicación IPC

## 📋 Descripción

El **Inicializador** es el primer programa del sistema de comunicación entre procesos pesados (heavy processes) mediante memoria compartida. Su función principal es establecer y configurar todas las estructuras de datos compartidas, semáforos y recursos necesarios para la comunicación entre emisores y receptores.

## 🏗️ Arquitectura del Sistema

```
┌──────────────────────────────────────────────────────┐
│                 MEMORIA COMPARTIDA                   │
│  ┌────────────┬────────────┬──────────┬──────────┐   │
│  │  Metadata  │   Buffers  │  Queues  │ File Data│   │
│  └────────────┴────────────┴──────────┴──────────┘   │
└──────────────────────────────────────────────────────┘
         ▲                                  
         │ Configura e Inicializa          
    ┌─────────────┐                         
    │Inicializador│                         
    └─────────────┘                         
```

## 📁 Estructura del Proyecto

```
inicializador/
├── src/
│   ├── main.c                    # Programa principal
│   ├── shared_memory_init.c      # Gestión de memoria compartida
│   ├── queue_manager.c           # Manejo de colas
│   ├── file_processor.c          # Procesamiento de archivos
│   └── semaphore_init.c          # Inicialización de semáforos
├── include/
│   ├── shared_memory_init.h      # Headers de memoria
│   ├── queue_manager.h           # Headers de colas
│   ├── file_processor.h          # Headers de archivos
│   ├── semaphore_init.h          # Headers de semáforos
│   ├── constants.h               # Constantes del sistema
│   └── structures.h              # Estructuras de datos
├── assets/
│   ├── data.txt                  # Archivo de entrada ejemplo
│   └── data.txt.bin              # Archivo binario generado
├── bin/
│   └── inicializador             # Ejecutable (después de compilar)
├── obj/
│   └── *.o                       # Archivos objeto (después de compilar)
├── Makefile                      # Sistema de compilación
├── setup.sh                      # Script de instalación
└── README.md                     # Este archivo
```

## 🔧 Instalación

### Prerrequisitos

- **Sistema Operativo**: Linux (nativo, no máquina virtual)
- **Compilador**: GCC
- **Herramientas**: Make
- **Bibliotecas**: pthread, System V IPC

### Instalación Automática

```bash
# Dar permisos de ejecución al script
chmod +x setup.sh

# Ejecutar el instalador interactivo
./setup.sh

# Seleccionar opción 7 para instalación completa
```

### Instalación Manual

```bash
# Instalar dependencias (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential gcc make

# Compilar el proyecto
make

# Ejecutar con parámetros de ejemplo
make run
```

## 💻 Uso

### Sintaxis

```bash
./bin/inicializador <archivo_entrada> <tamaño_buffer> <clave_encriptación>
```

### Parámetros

- **archivo_entrada**: Ruta al archivo de texto a procesar
- **tamaño_buffer**: Número de slots de caracteres (5-1000)
- **clave_encriptación**: Clave hexadecimal de 2 caracteres (ej: AA, FF, 5C)

### Ejemplos

```bash
# Ejemplo básico
./bin/inicializador assets/data.txt 10 AA

# Buffer grande con clave diferente
./bin/inicializador assets/data.txt 100 5C

# Archivo personalizado
./bin/inicializador /path/to/myfile.txt 20 FF
```

## 🎯 Funcionalidades

### 1. Procesamiento de Archivos
- Lee el archivo de entrada especificado
- Genera un archivo binario `.bin` con el contenido
- Almacena los datos en memoria compartida para acceso de otros procesos

### 2. Memoria Compartida
- Crea un segmento de memoria compartida con key `0x1234`
- Reserva espacio para:
  - Buffer circular de caracteres
  - Colas de sincronización
  - Datos del archivo
  - Metadatos del sistema

### 3. Inicialización de Colas
- **QueueEncript**: Inicializada con todas las posiciones disponibles
- **QueueDeencript**: Inicializada vacía

### 4. Sistema de Semáforos
- **sem_global_mutex**: Control de acceso global
- **sem_encrypt_queue**: Protección de cola de encriptación
- **sem_decrypt_queue**: Protección de cola de desencriptación
- **sem_encrypt_spaces**: Contador de espacios disponibles
- **sem_decrypt_items**: Contador de items para leer

## 📊 Estructura de Datos

### CharacterSlot
```c
typedef struct {
    unsigned char ascii_value;    // Valor ASCII encriptado
    int slot_index;              // Índice del slot (estático)
    time_t timestamp;            // Hora de introducción
    int is_valid;               // Flag de validez
    int text_index;             // Índice en el texto original
    pid_t emisor_pid;           // PID del emisor
} CharacterSlot;
```

### SharedMemory
```c
typedef struct {
    int shm_id;                  // ID de memoria compartida
    int buffer_size;             // Tamaño del buffer
    unsigned char encryption_key; // Clave XOR
    Queue encrypt_queue;         // Cola de encriptación
    Queue decrypt_queue;         // Cola de desencriptación
    // ... más campos
} SharedMemory;
```

## 🛠️ Comandos Make

```bash
make            # Compilar el proyecto
make run        # Ejecutar con parámetros de ejemplo
make clean      # Limpiar archivos compilados
make clean-all  # Limpiar todo incluyendo archivos generados
make clean-ipc  # Limpiar memoria compartida y semáforos
make status     # Ver estado del sistema IPC
make debug      # Ejecutar con Valgrind
make help       # Mostrar ayuda
```

## 🔍 Debugging

### Ver Estado del Sistema

```bash
# Estado de memoria compartida
ipcs -m

# Estado de semáforos
ipcs -s

# Estado completo
make status
```

### Limpiar Recursos IPC

```bash
# Automático (con confirmación)
make clean-ipc

# Manual
ipcrm -M 0x1234  # Eliminar memoria compartida
ipcrm -S 0x5000  # Eliminar semáforos
```

## ⚡ Características Técnicas

### Sincronización sin Busy Waiting
- Uso de semáforos System V para bloqueo eficiente
- Los procesos duermen cuando no hay recursos disponibles
- El kernel maneja la cola de procesos en espera

### Garantía de Orden Secuencial
- Índice global compartido para mantener orden
- Colas con información de índices de texto
- Dequeue ordenado para reconstrucción correcta

### Gestión de Memoria Eficiente
- Memoria alineada a páginas del sistema
- Pool de nodos para las colas (sin malloc/free)
- Offsets calculados para acceso directo

## 🐛 Solución de Problemas

### Error: "No se pudo crear memoria compartida"
```bash
# Verificar y limpiar memoria existente
ipcs -m | grep 0x1234
make clean-ipc
```

### Error: "Segmentation fault"
```bash
# Ejecutar con Valgrind para detectar el error
make debug
```

### Error: "Semáforos no encontrados"
```bash
# Verificar que el inicializador se ejecutó primero
./bin/inicializador assets/data.txt 10 AA
```

## 📈 Flujo de Ejecución

1. **Validación**: Verifica argumentos y archivo de entrada
2. **Procesamiento**: Lee el archivo y genera versión binaria
3. **Memoria**: Crea y configura memoria compartida
4. **Inicialización**: Configura buffers, colas y metadatos
5. **Semáforos**: Crea e inicializa semáforos de sincronización
6. **Finalización**: Termina dejando recursos listos para otros procesos

## 🎨 Output del Programa

El programa muestra información detallada con colores para facilitar el seguimiento:
- 🟢 **Verde**: Operaciones exitosas
- 🟡 **Amarillo**: Pasos en progreso
- 🔵 **Azul**: Información general
- 🔴 **Rojo**: Errores
- 🟣 **Magenta**: Finalización

## 📝 Notas Importantes

1. **No eliminar memoria**: El inicializador NO elimina la memoria compartida al terminar
2. **Ejecución única**: Debe ejecutarse una sola vez antes que otros procesos
3. **Clave hexadecimal**: La clave debe ser exactamente 2 caracteres hexadecimales
4. **Tamaño límites**: El buffer debe estar entre 5 y 1000 slots

## 🚦 Próximos Pasos

Después de ejecutar el inicializador exitosamente:

1. **Ejecutar Emisores**: `./emisor auto|manual [clave]`
2. **Ejecutar Receptores**: `./receptor auto|manual [clave]`
3. **Finalizar Sistema**: `./finalizador`

## 📚 Referencias

- [System V IPC Documentation](https://man7.org/linux/man-pages/man7/sysvipc.7.html)
- [Semaphore Operations](https://man7.org/linux/man-pages/man2/semop.2.html)
- [Shared Memory](https://man7.org/linux/man-pages/man2/shmget.2.html)

## 👥 Autor

Sistema desarrollado como parte del proyecto de Sistemas Operativos para implementar comunicación eficiente entre procesos pesados sin busy waiting.

## 📄 Licencia

Proyecto académico - Uso educativo

---

**¡El sistema está listo!** Ejecute el inicializador y luego lance emisores y receptores para comenzar la comunicación. 🚀