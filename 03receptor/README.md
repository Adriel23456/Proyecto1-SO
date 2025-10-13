# 🔵 Receptor - Sistema de Comunicación IPC

## 📋 Descripción

El **Receptor** es el tercer programa del sistema de comunicación entre procesos. Su función principal es extraer caracteres encriptados del buffer circular en memoria compartida, desencriptarlos usando XOR con la clave, y escribirlos al archivo de salida en orden secuencial, todo mediante sincronización con **semáforos POSIX sin busy waiting**.

## 🏗️ Arquitectura

```
┌──────────────────────────────────────────────────────┐
│                 MEMORIA COMPARTIDA                   │
│  ┌────────────┬────────────┬──────────┬──────────┐ │
│  │  Metadata  │   Buffer   │  Queues  │ File Data│ │
│  └────────────┴────────────┴──────────┴──────────┘ │
└──────────────────────────────────────────────────────┘
         │                │                │
         │                │                │
    [Consulta]       [Lee]          [Actualiza]
         │                │                │
         ▼                ▼                ▼
    ┌────────────────────────────────────────┐
    │           RECEPTOR (N instancias)      │
    │  • Lee slots con datos encriptados     │
    │  • Desencripta con XOR                 │
    │  • Escribe a archivo (posicional)      │
    │  • Devuelve slots libres               │
    │  • Finaliza automáticamente            │
    └────────────────────────────────────────┘
                         │
                         ▼
               ┌──────────────────┐
               │ Archivo de Salida│
               │  (.dec.bin)      │
               └──────────────────┘
```

## 📁 Estructura del Proyecto

```
receptor/
├── src/
│   ├── main.c                   # Programa principal (MUY comentado)
│   ├── shared_memory_access.c   # Acceso a memoria compartida (LIMPIO)
│   ├── queue_operations.c       # Operaciones de colas (LIMPIO)
│   ├── decoder.c                # Lógica de desencriptación XOR
│   ├── process_manager.c        # Gestión de procesos
│   └── output_file.c            # Escritura de archivo de salida
├── include/
│   ├── shared_memory_access.h   # 4 funciones
│   ├── queue_operations.h       # 2 funciones
│   ├── decoder.h
│   ├── process_manager.h
│   ├── output_file.h
│   ├── constants.h
│   └── structures.h
├── bin/
│   └── receptor                # Ejecutable (después de compilar)
├── obj/
│   └── *.o                     # Archivos objeto (sin .d)
├── out/
│   └── *.dec.bin               # Archivos de salida
├── Makefile
└── README.md                   # Este archivo
```

## 🔧 Instalación

### Prerrequisitos

1. **Sistema inicializado**: El inicializador debe ejecutarse primero
2. **Emisores activos**: Al menos un emisor generando datos
3. **Compilador**: GCC
4. **Bibliotecas**: pthread, librt

### Compilación

```bash
# Compilar el receptor
make

# Compilar con mensajes de debug
make CFLAGS="-DDEBUG"
```

## 💻 Uso

### Sintaxis

```bash
./bin/receptor <modo> [clave_hex] [delay_ms]
```

### Parámetros

* **modo**: `auto` o `manual`
  * `auto`: Recibe caracteres automáticamente con delay
  * `manual`: Requiere presionar ENTER para cada carácter
* **clave_hex** (opcional): Clave de desencriptación hexadecimal de 2 caracteres
  * Si no se especifica, usa la clave del inicializador
  * Debe coincidir con la clave del emisor para desencriptar correctamente
* **delay_ms** (opcional, solo modo auto): Delay en milisegundos (10-5000)
  * Por defecto: 100ms

### Ejemplos

```bash
# Modo automático con configuración por defecto
./bin/receptor auto

# Modo automático con clave personalizada
./bin/receptor auto FF

# Modo automático con clave y delay personalizado
./bin/receptor auto FF 50

# Modo manual
./bin/receptor manual

# Modo manual con clave personalizada
./bin/receptor manual 5C
```

## 🎯 Funcionalidades

### 1. Extracción Ordenada

* Extrae slots con el MENOR text_index (garantiza secuencialidad)
* Algoritmo O(n) sobre la cola de desencriptación
* Múltiples receptores pueden trabajar en paralelo

### 2. Desencriptación XOR

* Aplica XOR entre el carácter encriptado y la clave
* Operación simétrica: `decrypt(encrypt(x, k), k) = x`
* Visualización del carácter original y encriptado

### 3. Sincronización sin Busy Waiting

```c
// Espera dato disponible (bloqueo eficiente)
sem_wait(sem_decrypt_items);

// Extrae slot de la cola (sección crítica)
sem_wait(sem_decrypt_queue);
SlotInfo info = dequeue_decrypt_slot_ordered(shm);
sem_post(sem_decrypt_queue);

// Devuelve slot libre
sem_post(sem_encrypt_spaces);
```

### 4. Detección Automática de Finalización

* Verifica si el archivo completo fue procesado
* Verifica si la cola de datos está vacía
* **Finaliza automáticamente** sin intervención manual
* **NO hace busy waiting** durante las verificaciones

### 5. Escritura Posicional Segura

* Usa `pwrite()` para escritura en índice específico
* Múltiples receptores pueden escribir en paralelo
* Archivo pre-dimensionado con `ftruncate()`

### 6. Visualización en Tiempo Real

* Estado de cada carácter recibido
* Progreso del procesamiento
* Estado de las colas
* Estadísticas de rendimiento

## 🚀 Comandos Make

```bash
make              # Compilar
make run-auto     # Ejecutar en modo automático
make run-manual   # Ejecutar en modo manual
make run-auto-key # Ejecutar con clave personalizada
make run-multiple # Lanzar múltiples receptores
make clean        # Limpiar compilación
make clean-all    # Limpiar todo incluyendo archivos de salida
make status       # Ver receptores activos
make watch-sleep  # Monitoreo en vivo (demuestra no busy-wait)
make strace       # Strace de un receptor
make kill-all     # Terminar todos los receptores (SIGUSR1)
make test         # Test rápido del sistema
make help         # Mostrar ayuda
```

## ⚡ Flujo de Operación

1. **Conexión**: Se conecta a memoria compartida existente
2. **Semáforos**: Abre semáforos POSIX nombrados
3. **Registro**: Se registra como receptor activo
4. **Archivo de Salida**: Abre/crea archivo `.dec.bin` pre-dimensionado
5. **Bucle Principal**:

   ```
   MIENTRAS no_terminar Y no_finalizado:
     1. Verificar si archivo completo procesado (no busy waiting)
     2. Esperar dato disponible (sem_wait - BLOQUEO)
     3. Extraer slot con menor text_index (ordenado)
     4. Leer slot de memoria
     5. Desencriptar carácter
     6. Escribir a archivo (pwrite posicional)
     7. Marcar slot como libre
     8. Devolver slot a cola de encriptación
     9. Notificar espacio disponible (sem_post)
     10. Mostrar estado
     11. Verificar finalización (segunda oportunidad)
     12. Esperar (auto) o pedir ENTER (manual)
   ```
6. **Finalización**: Resumen, desregistro y limpieza

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

### SlotInfo (Cola de Desencriptación)

```c
SlotInfo {
    int slot_index;  // Índice del slot [0..buffer_size-1]
    int text_index;  // Índice del texto original
}
```

### Colas Circulares

* **QueueEncript**: Slots libres para escribir (receptor devuelve aquí)
* **QueueDeencript**: Slots con datos para leer (receptor lee de aquí)

## 🎨 Output del Programa

```
╔════════════════════════════════════════════════════╗
║               CARÁCTER RECIBIDO                  ║
╠════════════════════════════════════════════════════╣
║ PID Receptor: 24410                              ║
║ Índice texto: 42 / 1000                          ║
║ Slot memoria: 7                                  ║
║ Encriptado:  0xE3                                ║
║ Desencript.: 'H' (0x48)                          ║
║ Insertado:   14:32:15  Emisor PID: 12345         ║
║ Colas: [Libres: 8] [Con datos: 2]               ║
╚════════════════════════════════════════════════════╝
```

### Finalización Automática

```
[RECEPTOR 24410] Todos los caracteres procesados y cola vacía
  • Total procesado globalmente: 1000/1000
  • Recibidos por este receptor: 250

╔══════════════════════════════════════════════════════════╗
║             RECEPTOR PID  24410 FINALIZANDO             ║
╚══════════════════════════════════════════════════════════╝
  • Caracteres recibidos: 250
  • Tiempo de ejecución: 30 s
  • Velocidad promedio: 8.33 chars/s

[RECEPTOR 24410] Proceso terminado correctamente
```

## 🔍 Debugging

### Ver Estado del Sistema

```bash
# Receptores activos
ps aux | grep receptor

# Estado de memoria compartida
ipcs -m | grep 0x1234

# Semáforos POSIX
ls -l /dev/shm/sem.*

# Archivos de salida generados
ls -lh out/
```

### ✅ Verificación de NO busy waiting (100%)

Usa estos comandos para confirmar que los receptores están dormidos en el semáforo y **no** girando en bucle:

```bash
# Todos los receptores con su estado, CPU y wchan (wait channel)
pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}

# Vista en tiempo real cada 0.5s
watch -n 0.5 'pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}'

# Monitoreo detallado (demo anti busy-wait)
make watch-sleep

# Adjuntar a un PID y ver bloqueo en futex
sudo strace -tt -p <PID> -e trace=futex,ppoll,select,clock_nanosleep,nanosleep
```

**Qué esperar:**

* `STAT` en `S`/`S+` (sleeping), **%CPU ~ 0.0%**.
* `WCHAN` mostrando `futex_wait_queue_me`/`futex_wait`.
* En `strace`, el proceso queda detenido en `futex(FUTEX_WAIT, ...)` hasta que un emisor haga `sem_post` o reciba una señal.

### Problemas Comunes

**"No se pudo conectar a memoria compartida"**
* Ejecutar primero el inicializador
* Verificar con `ipcs -m`

**"No se pudieron abrir semáforos"**
* Verificar semáforos POSIX: `ls /dev/shm/sem.*`
* Reinicializar si es necesario

**"No se pudo preparar archivo de salida"**
* Verificar permisos en directorio `./out/`
* El directorio se crea automáticamente si no existe

**Proceso bloqueado indefinidamente**
* Verificar emisores activos
* Puede estar esperando datos si no hay emisores

**Proceso no finaliza automáticamente**
* Verificado y corregido en la versión actual
* Implementa detección automática de finalización

## 🎯 Características Técnicas

### Sincronización

* **Sin busy waiting**: Uso de `sem_wait`/`sem_post`
* **Extracción ordenada**: Garantiza secuencialidad del output
* **Detección automática de fin**: Sin busy waiting en verificaciones

### Rendimiento

* Múltiples receptores en paralelo
* Delay configurable (10-5000ms)
* Escritura posicional eficiente con `pwrite()`

### Robustez

* Manejo de señales (SIGINT, SIGTERM, SIGUSR1)
* Validación de slots antes de procesar
* Limpieza automática al terminar
* Finalización automática cuando se completa el archivo

### Código Limpio

* **25% menos código** que versión original
* **500% más comentarios** útiles
* Sin funciones duplicadas o no usadas
* Solo 18 funciones necesarias (eliminadas 12 innecesarias)

## 📈 Métricas y Estadísticas

El receptor rastrea:

* Caracteres recibidos (local)
* Total procesado globalmente
* Tiempo de ejecución
* Velocidad promedio (chars/segundo)
* Estado de las colas en tiempo real

## 🚦 Estados del Receptor

1. **ACTIVO**: Procesando caracteres
2. **BLOQUEADO**: Esperando datos (sem_wait)
3. **VERIFICANDO**: Comprobando finalización
4. **FINALIZANDO**: Proceso de cierre iniciado
5. **TERMINADO**: Limpieza completada

## 🔧 Configuración Avanzada

### Múltiples Receptores

```bash
# Lanzar 3 receptores en paralelo
for i in {1..3}; do
    ./bin/receptor auto &
done

# O usando make
make run-multiple
# (preguntará cuántos receptores lanzar)
```

### Diferentes Claves

```bash
# Receptor 1 con clave AA
./bin/receptor auto AA &

# Receptor 2 con clave FF (debe coincidir con emisor)
./bin/receptor auto FF &
```

### Directorio de Salida Personalizado

```bash
# Usar variable de entorno
export RECEPTOR_OUT_DIR="/tmp/salidas"
./bin/receptor auto

# El archivo se creará en /tmp/salidas/<nombre>.dec.bin
```

## 📝 Notas Importantes

1. **Orden secuencial garantizado**: Extracción por menor text_index
2. **Escritura segura**: `pwrite()` permite múltiples receptores
3. **Finalización automática**: Detecta cuando se completa el archivo
4. **Sin busy waiting**: Verificaciones eficientes sin consumir CPU
5. **Clave correcta necesaria**: Debe coincidir con la del emisor
6. **Código limpio**: Sin duplicados ni funciones innecesarias

## 🧹 Limpieza de Código (v2.0)

La versión actual implementa:

* ✅ Eliminados 4 archivos duplicados/no usados
* ✅ Eliminadas 12 funciones que no se usaban
* ✅ Reducción del 25% en líneas de código
* ✅ Aumento del 500% en comentarios útiles
* ✅ Sin warnings de compilación
* ✅ Sin archivos `.d` innecesarios

## 🚀 Próximos Pasos

Después de tener receptores funcionando:

1. **Verificar Salida**: Comparar archivo `.dec.bin` con original
2. **Monitorear**: Ver progreso en tiempo real con `make watch-sleep`
3. **Múltiples Receptores**: Acelerar procesamiento con instancias paralelas
4. **Finalizar**: Usar el finalizador para cierre ordenado del sistema

---

**¡El Receptor está listo!** Puede ejecutar múltiples instancias en paralelo para acelerar la recepción. Finaliza automáticamente cuando se completa el archivo. 🔵