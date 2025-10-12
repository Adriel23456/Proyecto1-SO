# 🟢 Emisor - Sistema de Comunicación IPC

## 📋 Descripción

El **Emisor** es el segundo programa del sistema de comunicación entre procesos. Su función principal es leer caracteres del archivo cargado en memoria compartida, encriptarlos usando XOR con una clave, y colocarlos en slots disponibles del buffer circular, todo mediante sincronización con **semáforos POSIX sin busy waiting**.

## 🏗️ Arquitectura

```
┌──────────────────────────────────────────────────────┐
│                 MEMORIA COMPARTIDA                   │
│  ┌────────────┬────────────┬──────────┬──────────┐ │
│  │  Metadata  │   Buffer   │  Queues  │ File Data│ │
│  └────────────┴────────────┴──────────┴──────────┘ │
└──────────────────────────────────────────────────────┘
         ▲                ▲                ▲
         │                │                │
    [Lee archivo]    [Escribe]      [Actualiza]
         │                │                │
    ┌────────────────────────────────────────┐
    │              EMISOR (N instancias)     │
    │  • Lee caracteres del archivo          │
    │  • Encripta con XOR                    │
    │  • Coloca en slots disponibles         │
    │  • Actualiza colas                     │
    └────────────────────────────────────────┘
```

## 📁 Estructura del Proyecto

```
emisor/
├── src/
│   ├── main.c                   # Programa principal
│   ├── shared_memory_access.c   # Acceso a memoria compartida
│   ├── queue_operations.c       # Operaciones de colas
│   ├── encoder.c                # Lógica de encriptación XOR
│   ├── process_manager.c        # Gestión de procesos
│   └── display.c                # Funciones de visualización
├── include/
│   ├── shared_memory_access.h
│   ├── queue_operations.h
│   ├── encoder.h
│   ├── process_manager.h
│   ├── display.h
│   ├── constants.h
│   └── structures.h
├── bin/
│   └── emisor                  # Ejecutable (después de compilar)
├── obj/
│   └── *.o                     # Archivos objeto
├── Makefile
└── README.md                   # Este archivo
```

## 🔧 Instalación

### Prerrequisitos

1. **Sistema inicializado**: El inicializador debe ejecutarse primero
2. **Compilador**: GCC
3. **Bibliotecas**: pthread, librt

### Compilación

```bash
# Compilar el emisor
make

# Compilar con mensajes de debug
make CFLAGS="-DDEBUG"
```

## 💻 Uso

### Sintaxis

```bash
./bin/emisor <modo> [clave_hex] [delay_ms]
```

### Parámetros

* **modo**: `auto` o `manual`

  * `auto`: Envía caracteres automáticamente con delay
  * `manual`: Requiere presionar ENTER para cada carácter
* **clave_hex** (opcional): Clave de encriptación hexadecimal de 2 caracteres

  * Si no se especifica, usa la clave del inicializador
* **delay_ms** (opcional, solo modo auto): Delay en milisegundos (10-5000)

  * Por defecto: 100ms

### Ejemplos

```bash
# Modo automático con configuración por defecto
./bin/emisor auto

# Modo automático con clave personalizada
./bin/emisor auto FF

# Modo automático con clave y delay personalizado
./bin/emisor auto FF 50

# Modo manual
./bin/emisor manual

# Modo manual con clave personalizada
./bin/emisor manual 5C
```

## 🎯 Funcionalidades

### 1. Lectura Secuencial

* Lee caracteres del archivo en memoria compartida
* Mantiene orden secuencial mediante índice global atómico
* Múltiples emisores pueden trabajar en paralelo

### 2. Encriptación XOR

* Aplica XOR entre el carácter y la clave
* Operación simétrica: `decrypt(encrypt(x, k), k) = x`
* Visualización binaria de la operación

### 3. Sincronización sin Busy Waiting

```c
// Espera espacio disponible (bloqueo eficiente)
sem_wait(sem_encrypt_spaces);

// Obtiene slot de la cola (sección crítica)
sem_wait(sem_encrypt_queue);
int slot = dequeue_encrypt_slot(shm);
sem_post(sem_encrypt_queue);

// Notifica dato disponible
sem_post(sem_decrypt_items);
```

### 4. Gestión de Procesos

* Registro automático en el sistema
* Manejo de señales (SIGINT, SIGTERM, SIGUSR1)
* Desregistro limpio al terminar

### 5. Visualización en Tiempo Real

* Estado de cada carácter enviado
* Progreso global del archivo
* Estado de las colas
* Estadísticas de rendimiento

## 🚀 Comandos Make

```bash
make              # Compilar
make run-auto     # Ejecutar en modo automático
make run-manual   # Ejecutar en modo manual
make run-multiple # Lanzar múltiples emisores
make run-delay    # Ejecutar con delay personalizado
make clean        # Limpiar compilación
make debug        # Ejecutar con Valgrind
make status       # Ver emisores activos
make kill-all     # Terminar todos los emisores
make test         # Test rápido del sistema
make help         # Mostrar ayuda
```

## ⚡ Flujo de Operación

1. **Conexión**: Se conecta a memoria compartida existente
2. **Semáforos**: Abre semáforos POSIX nombrados
3. **Registro**: Se registra como emisor activo
4. **Bucle Principal**:

   ```
   MIENTRAS no_terminar Y quedan_caracteres:
     1. Obtener siguiente índice (atómico)
     2. Leer carácter del archivo
     3. Esperar espacio disponible (sem_wait)
     4. Obtener slot libre de cola
     5. Encriptar y almacenar
     6. Añadir a cola de desencriptación
     7. Notificar dato disponible (sem_post)
     8. Mostrar estado
     9. Esperar (auto) o pedir ENTER (manual)
   ```
5. **Finalización**: Desregistro y limpieza

## 📊 Estructuras de Datos

### Slot de Carácter

```c
CharacterSlot {
    unsigned char ascii_value;  // Valor encriptado
    int slot_index;             // Índice del slot
    time_t timestamp;           // Hora de inserción
    int is_valid;               // Flag de validez
    int text_index;             // Índice en texto original
    pid_t emisor_pid;           // PID del emisor
}
```

### Colas Circulares

* **QueueEncript**: Slots libres para escribir
* **QueueDeencript**: Slots con datos para leer

## 🎨 Output del Programa

```
╔════════════════════════════════════════════════════╗
║               CARÁCTER ENVIADO                    ║
╠════════════════════════════════════════════════════╣
║ PID Emisor: 12345                                 ║
║ Índice texto: 42 / 1000                           ║
║ Slot memoria: 7                                   ║
║ Original: 'H' (0x48)                              ║
║ Encriptado: 0xE3                                  ║
║ Hora: 14:32:15                                    ║
║ Colas: [Libres: 8] [Con datos: 2]                ║
╚════════════════════════════════════════════════════╝
```

## 🔍 Debugging

### Ver Estado del Sistema

```bash
# Emisores activos
ps aux | grep emisor

# Estado de memoria compartida
ipcs -m | grep 0x1234

# Semáforos POSIX
ls -l /dev/shm/sem.*
```

### ✅ Verificación de NO busy waiting (100%)

Usa estos comandos para confirmar que los emisores están dormidos en el semáforo y **no** girando en bucle:

```bash
# Todos los emisores con su estado, CPU y wchan (wait channel)
pgrep emisor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}

# Vista en tiempo real cada 0.01s
watch -n 0.01 'pgrep emisor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}'

# Adjuntar a un PID y ver bloqueo en futex (despertará con sem_post o señal)
sudo strace -tt -p <PID> -e trace=futex,ppoll,select,clock_nanosleep,nanosleep
```

**Qué esperar:**

* `STAT` en `S`/`S+` (sleeping), **%CPU ~ 0.0**.
* `WCHAN` mostrando algo tipo `futex_wait_queue_me`/`futex_wait`.
* En `strace`, el proceso queda detenido en una llamada `futex(FUTEX_WAIT, ...)` hasta que otro proceso haga `sem_post` o reciba una señal.

### Problemas Comunes

**"No se pudo conectar a memoria compartida"**

* Ejecutar primero el inicializador
* Verificar con `ipcs -m`

**"No se pudo abrir semáforo"**

* Verificar semáforos POSIX: `ls /dev/shm/sem.*`
* Reinicializar si es necesario

**Proceso bloqueado**

* Verificar receptores activos
* Puede estar esperando espacio en buffer

## 🎯 Características Técnicas

### Sincronización

* **Sin busy waiting**: Uso de `sem_wait`/`sem_post`
* **Secciones críticas**: Protegidas con mutexes
* **Orden garantizado**: Índice global atómico

### Rendimiento

* Múltiples emisores en paralelo
* Delay configurable (10-5000ms)
* Buffer circular eficiente

### Robustez

* Manejo de señales
* Validación de datos
* Limpieza automática al terminar

## 📈 Métricas y Estadísticas

El emisor rastrea:

* Caracteres enviados
* Tiempo de ejecución
* Velocidad promedio (chars/segundo)
* Estado de las colas en tiempo real

## 🚦 Estados del Emisor

1. **ACTIVO**: Procesando caracteres
2. **BLOQUEADO**: Esperando espacio (sem_wait)
3. **FINALIZANDO**: Recibió señal de término
4. **TERMINADO**: Limpieza completada

## 🔧 Configuración Avanzada

### Múltiples Emisores

```bash
# Lanzar 3 emisores en paralelo
for i in {1..3}; do
    ./bin/emisor auto &
done
```

### Diferentes Claves

```bash
# Emisor 1 con clave AA
./bin/emisor auto AA &

# Emisor 2 con clave FF
./bin/emisor auto FF &
```

## 📝 Notas Importantes

1. **Orden secuencial**: Garantizado por índice global
2. **Sin pérdida de datos**: Bloqueo hasta tener espacio
3. **Limpieza automática**: Al recibir señales
4. **Compatibilidad**: Con semáforos POSIX del inicializador

## 🚀 Próximos Pasos

Después de tener emisores funcionando:

1. **Ejecutar Receptores**: Para leer los datos encriptados
2. **Monitorear**: Ver progreso en tiempo real
3. **Finalizar**: Usar el finalizador para cierre ordenado

---

**¡El Emisor está listo!** Puede ejecutar múltiples instancias en paralelo para acelerar el procesamiento. 🚀
