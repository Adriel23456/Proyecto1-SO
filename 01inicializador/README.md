# 🚀 Inicializador - Sistema de Comunicación IPC

## 📋 Descripción

El **Inicializador** es el primer programa del sistema de comunicación entre procesos pesados (heavy processes) mediante **memoria compartida System V** y **semáforos POSIX nombrados**.  
Su función principal es establecer y configurar todas las estructuras de datos compartidas, semáforos y recursos necesarios para la comunicación entre emisores y receptores, asegurando sincronización **sin busy waiting**.

---

## 🏗️ Arquitectura del Sistema

```

┌──────────────────────────────────────────────────────┐
│ MEMORIA COMPARTIDA                                   │
│ ┌────────────┬────────────┬──────────┬──────────┐     │
│ │ Metadata   │ Buffers    │ Queues   │ File Data│     │
│ └────────────┴────────────┴──────────┴──────────┘     │
└──────────────────────────────────────────────────────┘
▲
│ Configura e Inicializa
┌─────────────┐
│Inicializador│
└─────────────┘

```

---

## 📁 Estructura del Proyecto

```

inicializador/
├── src/
│   ├── main.c                # Programa principal
│   ├── shared_memory_init.c  # Gestión de memoria compartida
│   ├── queue_manager.c       # Manejo de colas
│   ├── file_processor.c      # Procesamiento de archivos
│   └── semaphore_init.c      # Inicialización de semáforos POSIX
├── include/
│   ├── shared_memory_init.h  # Headers de memoria
│   ├── queue_manager.h       # Headers de colas
│   ├── file_processor.h      # Headers de archivos
│   ├── semaphore_init.h      # Headers de semáforos
│   ├── constants.h           # Constantes del sistema
│   └── structures.h          # Estructuras de datos
├── assets/
│   ├── data.txt              # Archivo de entrada ejemplo
│   └── data.txt.bin          # Archivo binario generado
├── bin/
│   └── inicializador         # Ejecutable (después de compilar)
├── obj/
│   └── *.o                   # Archivos objeto (después de compilar)
├── Makefile                  # Sistema de compilación
├── setup.sh                  # Script de instalación
└── README.md                 # Este archivo

````

---

## 🔧 Instalación

### Prerrequisitos

- **Sistema Operativo:** Linux (nativo, no virtualizado)
- **Compilador:** GCC
- **Herramientas:** Make
- **Bibliotecas:** pthread, librt (según distro), System V IPC

### Instalación Automática

```bash
# Dar permisos de ejecución al script
chmod +x setup.sh

# Ejecutar el instalador interactivo
./setup.sh
````

Seleccionar la **opción 7** para instalación completa.

### Instalación Manual

```bash
# Instalar dependencias (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential gcc make gdb valgrind

# Compilar el proyecto
make

# Ejecutar con parámetros de ejemplo
make run
```

---

## 💻 Uso

### Sintaxis

```bash
./bin/inicializador <archivo_entrada> <tamaño_buffer> <clave_encriptación>
```

### Parámetros

* **archivo_entrada:** Ruta al archivo de texto a procesar.
* **tamaño_buffer:** Número de slots de caracteres (≥ 1; depende de la RAM y `/dev/shm`).
* **clave_encriptación:** Clave hexadecimal de 2 caracteres (ej: `AA`, `FF`, `5C`).

### Ejemplos

```bash
# Ejemplo básico
./bin/inicializador assets/data.txt 10 AA

# Buffer grande con clave diferente
./bin/inicializador assets/data.txt 100 5C

# Archivo personalizado
./bin/inicializador /path/to/myfile.txt 2000 FF
```

---

## 🎯 Funcionalidades

### 1. Procesamiento de Archivos

* Lee el archivo de entrada.
* Genera un archivo binario `.bin`.
* Carga los datos en memoria compartida.

### 2. Memoria Compartida

Crea un segmento con `key = 0x1234` y reserva:

* Buffer circular de caracteres.
* Colas de sincronización.
* Datos del archivo.
* Metadatos del sistema.

### 3. Inicialización de Colas

* `QueueEncript`: Iniciada con todas las posiciones disponibles.
* `QueueDeencript`: Iniciada vacía.

### 4. Sistema de Semáforos POSIX

Semáforos nombrados:

* `SEM_NAME_GLOBAL_MUTEX`
* `SEM_NAME_ENCRYPT_QUEUE`
* `SEM_NAME_DECRYPT_QUEUE`
* `SEM_NAME_ENCRYPT_SPACES`
* `SEM_NAME_DECRYPT_ITEMS`

Sincronización mediante `sem_wait` / `sem_post`, **sin busy waiting**.

---

## 📊 Estructuras de Datos

### CharacterSlot

```c
typedef struct {
    unsigned char ascii_value;
    int slot_index;
    time_t timestamp;
    int is_valid;
    int text_index;
    pid_t emisor_pid;
} CharacterSlot;
```

### SharedMemory

```c
typedef struct {
    int shm_id;
    int buffer_size;
    unsigned char encryption_key;
    Queue encrypt_queue;
    Queue decrypt_queue;
    // ... otros campos
} SharedMemory;
```

---

## 🛠️ Comandos Make

```bash
make            # Compilar el proyecto
make run        # Ejecutar con parámetros de ejemplo
make clean      # Limpiar archivos compilados
make clean-all  # Limpiar todo (incluyendo binarios)
make clean-ipc  # Eliminar SHM y semáforos POSIX
make status     # Ver estado de SHM y semáforos POSIX
make help       # Mostrar ayuda
```

---

## 🔍 Debugging

### Ver Estado del Sistema

```bash
# Memoria compartida System V
ipcs -m

# Semáforos POSIX nombrados
ls -l /dev/shm/sem.*
```

### Limpiar Recursos IPC

```bash
make clean-ipc
```

---

## ⚡ Sincronización sin Busy Waiting

Los semáforos POSIX permiten que los procesos se bloqueen automáticamente cuando no hay recursos, evitando el uso de bucles activos (spin loops).
El kernel gestiona la cola de espera, garantizando eficiencia.

---

## 🧭 Límites del Sistema y Diagnóstico Rápido

Estos comandos ayudan a verificar los límites del entorno para semáforos y memoria compartida.

### POSIX Semaphores (Valor máximo según slots y buffer)

```bash
# Valor máximo permitido por el runtime POSIX
getconf SEM_VALUE_MAX

# Espacio disponible para objetos POSIX (incl. semáforos)
df -h /dev/shm

# Listar semáforos POSIX existentes
ls -l /dev/shm/sem.*
```

### Memoria Compartida System V

```bash
# Resumen de límites de SHM
ipcs -lm

# Máximo tamaño de un segmento (bytes)
cat /proc/sys/kernel/shmmax

# Total de páginas disponibles (x tamaño de página)
cat /proc/sys/kernel/shmall

# Tamaño de página
getconf PAGESIZE
```

> 💡 **Recomendación:**
> Si `buffer_size` es muy grande, verifica `df -h /dev/shm`.
> Si el archivo es enorme, revisa `shmmax` y `shmall`.

---

## 🐛 Solución de Problemas

### Error: “No se pudo crear memoria compartida”

```bash
ipcs -m | grep 0x1234
make clean-ipc
```

### Error: “Semáforos no encontrados”

Asegúrate de ejecutar primero el inicializador:

```bash
./bin/inicializador assets/data.txt 10 AA
```

### Error de Memoria Insuficiente

Verifica:

```bash
cat /proc/sys/kernel/shmmax
cat /proc/sys/kernel/shmall
df -h /dev/shm
```

---

## 📈 Flujo de Ejecución

1. **Validación:** Verifica parámetros y archivo.
2. **Procesamiento:** Lee y genera binario.
3. **Memoria:** Crea y configura SHM.
4. **Inicialización:** Configura colas y metadatos.
5. **Semáforos:** Crea semáforos POSIX nombrados.
6. **Finalización:** Deja todo listo para emisores/receptores.

---

## 🎨 Output del Programa

* 🟢 **Verde:** Éxito
* 🟡 **Amarillo:** En progreso
* 🔵 **Azul:** Información
* 🔴 **Rojo:** Error
* 🟣 **Magenta:** Finalización

---

## 📝 Notas Importantes

* El inicializador **no elimina** memoria ni semáforos al finalizar.
  Usa `make clean-ipc` o el **finalizador** para ello.
* Los semáforos POSIX viven en `/dev/shm/sem.*`.
* El tamaño máximo del buffer depende de `RAM`, `/dev/shm`, `shmmax` y `shmall`.
* La clave hexadecimal debe tener exactamente **2 caracteres**.

---

## 🚦 Próximos Pasos

```bash
# Ejecutar emisores
./emisor auto|manual [clave]

# Ejecutar receptores
./receptor auto|manual [clave]

# Finalizar sistema
./finalizador
# o
make clean-ipc
```

---

## 📚 Referencias

* `man 7 sem_overview`
* `man 3 sem_open`, `man 3 sem_unlink`
* `man 7 sysvipc`
* `man 2 shmget`, `man 2 shmat`

---

## 👥 Autor

Sistema desarrollado como parte del proyecto académico de **Sistemas Operativos**, implementando comunicación eficiente entre procesos pesados sin busy waiting.

---

## 📄 Licencia

Proyecto académico — Uso educativo.

---

> ✅ **¡El sistema está listo!**
> Ejecuta el **inicializador** y luego lanza emisores y receptores para iniciar la comunicación. 🚀