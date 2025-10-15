# 1) ¿Por qué **5 semáforos POSIX** y qué hace cada uno?

En `semaphore_init.h/.c` defines 5 semáforos **nombrados** (persisten en `/dev/shm/sem.*`):

1. **`/sem_global_mutex` (valor inicial = 1)**

   * **Tipo:** mutex (binario).
   * **Protege:** los **metadatos globales** en `SharedMemory`:

     * `current_txt_index`, `total_chars_processed`
     * altas/bajas de procesos (`register_emisor`, `unregister_emisor`, `register_receptor`, etc.)
     * arrays de estadísticas `*_stats`
   * **Quién lo usa:**

     * Emisor: `get_next_text_index()` para hacer `++current_txt_index` de forma **atómica**.
     * Emisor/Receptor: `register_*`, `unregister_*`, `save_*_stats`.
   * **LLamadas concretas:** `sem_wait(g_sem_global)` / `sem_post(g_sem_global)`.

2. **`/sem_encrypt_queue` (valor inicial = 1)**

   * **Tipo:** mutex.
   * **Protege:** la **estructura de la cola de “slots libres”** (`encrypt_queue`), es decir sus `head/tail/size` y el array físico de `SlotRef` en **SHM**.
   * **Quién lo usa:**

     * Emisor: `dequeue_encrypt_slot()` para pedir un slot libre.
     * Emisor/Receptor: `enqueue_encrypt_slot()` para devolver el slot una vez consumido.
   * **LLamadas:** `sem_wait(g_sem_encrypt_queue)` / `sem_post(g_sem_encrypt_queue)` alrededor de **cualquier** operación sobre esa cola.

3. **`/sem_decrypt_queue` (valor inicial = 1)**

   * **Tipo:** mutex.
   * **Protege:** la **cola de “slots con datos”** (`decrypt_queue`), igual: `head/tail/size` + array `SlotRef`.
   * **Quién lo usa:**

     * Emisor: `enqueue_decrypt_slot()` para publicar un carácter ya encriptado.
     * Receptor: `dequeue_decrypt_slot_ordered()` para extraer el **siguiente** elemento a procesar **en orden** (detalle en §3).
   * **LLamadas:** `sem_wait(g_sem_decrypt_queue)` / `sem_post(g_sem_decrypt_queue)`.

4. **`/sem_encrypt_spaces` (valor inicial = `buffer_size`)**

   * **Tipo:** **contador** (semaforo POSIX de conteo).
   * **Significado:** **número de espacios libres** en el **buffer circular** (capacidad N = `buffer_size`).
   * **Quién lo usa:**

     * Emisor hace `sem_wait()` antes de producir: si **no hay espacio**, **bloquea** (sin busy-wait).
     * Receptor hace `sem_post()` cuando **libera** un slot (después de escribir salida), anunciando “hay un hueco más”.
   * **Por eso “énfasis en disponibilidad de espacio”**: este contador **modela exactamente** la capacidad libre del buffer.
   * **Invariante esencial (sin errores):**

     ```
     encrypt_spaces + decrypt_items == buffer_size
     ```

     (ver 5) abajo) —es la sanidad del productor/consumidor.

5. **`/sem_decrypt_items` (valor inicial = 0)**

   * **Tipo:** **contador**.
   * **Significado:** **número de items disponibles** para consumir en la cola de desencriptación.
   * **Quién lo usa:**

     * Emisor hace `sem_post()` al publicar un slot lleno.
     * Receptor hace `sem_wait()` para **bloquear** hasta que exista un item.

### ¿Por qué no menos (o más) semáforos?

* Podríamos haber usado **1 único mutex** para **ambas** colas, pero separarlos (`encrypt_queue` y `decrypt_queue`) aumenta **concurrencia** real: un emisor puede manipular una cola a la vez que un receptor manipula la otra, sin interferirse.
* El **mutex global** evita que mezclemos su responsabilidad con la de las colas (las colas no “saben” de `current_txt_index` ni de altas/bajas de procesos).
* Los **dos contadores** son **imprescindibles** para el patrón **productor/consumidor con buffer acotado** (algoritmo de Dijkstra): uno controla **espacio**, el otro controla **items**.
* Resultado: 3 **mutex** (global + 2 colas) + 2 **contadores** (espacios + items) = **5 semáforos**.

### Detalle Linux/POXIS

* **Creación**: `sem_open(name, O_CREAT|O_EXCL, 0666, initial)` (en `initialize_semaphores`) → núcleo crea un objeto `sem.*` en `tmpfs` (`/dev/shm`).
* **Cierre**: `sem_close()` libera el **handle** del proceso; **no** borra el objeto global.
* **Borrado**: `sem_unlink(name)` (lo haces en `cleanup_semaphores`) elimina el nombre global (como `unlink()` de un archivo).
* **Bloqueo**: `sem_wait()` duerme el hilo/proceso (internamente usa **futex**), 0% CPU; si recibe una señal retorna `-1` con `errno=EINTR` (lo manejas).
* **Visibilidad de memoria (orden)**: las operaciones de semáforo son **barreras** suficientes para que lo que el productor escribió en SHM **sea visible** al consumidor **después** del `sem_post()` correspondiente. No necesitas `mfence` manual.

---

# 2) ¿Por qué **System V SHM** para el `.txt` y estructuras?

Usas **System V Shared Memory** (`shmget/shmat/shmctl`) en `shared_memory_init.c`:

* **`shmget(key, size, IPC_CREAT|IPC_EXCL|0666)`**
  Reservas **un solo segmento contiguo** que contiene:
  `[SharedMemory][CharacterSlot buffer][file_data][enc_queue_array][dec_queue_array]`
  El tamaño lo calculas con `sysconf(_SC_PAGESIZE)` y alineas a página. Antes validas contra **`/proc/sys/kernel/shmmax`** (con `read_shmmax_bytes()`).

* **Ventajas concretas de System V aquí:**

  1. **Descubrimiento simple por clave numérica** (`0x1234`): cualquier proceso “huérfano” puede re-adjuntarse con `shmget()+shmat()` sin compartir file descriptors.
  2. **Una región monolítica** facilita tu diseño de **offsets** (no punteros crudos) para que funcionen en **todos** los procesos (evitas “punteros inválidos” tras `shmat` en distintas direcciones).
  3. **Herramientas estándar**: `ipcs -m`, `ipcrm -M 0x1234` (lo expones en el Makefile con `status`/`clean-ipc`).
  4. **Ciclo de vida controlado**: con `shmctl(shmid, IPC_RMID)` marcas para borrado (aun con procesos adjuntos, se borra al último `shmdt`).

* **`shmat(shmid, NULL, 0)`**
  Adjunta el segmento al espacio de direcciones del proceso y te devuelve un **puntero base**. Desde ahí, accedes por **offsets**:

  * `shm->buffer_offset` → `CharacterSlot*`
  * `shm->file_data_offset` → bytes del `.txt`
  * `encrypt_queue.array_offset` / `decrypt_queue.array_offset` → arrays de `SlotRef`

* **Por qué no POSIX `shm_open`?** También valía. Elegiste System V por **portabilidad clásica**, tooling (`ipcs/ipcrm`), **clave numérica** sin necesidad de nombres de archivo, y porque ya mezclas POSIX **semaforía** nombrada (muy cómoda) con SHM System V (muy robusta para un bloque grande y único). Esta mezcla es **común** en Linux.

> Nota de límites/espacio real del sistema:
>
> * Tamaño del segmento SHM: limitado por `shmmax` y `shmall`. Lo imprimes en `make status`.
> * Semáforos POSIX y `shm_open` viven en `/dev/shm` (tmpfs); también muestras `df -h /dev/shm`.

---

# 3) ¿Cómo garantizamos **secuencialidad** y **protección total**?

Tu sistema tiene **múltiples emisores** y **múltiples receptores**. La secuencialidad del texto (orden 0..N-1) y la ausencia de condiciones de carrera salen de **tres ideas**:

## 3.1 Asignación **atómica** del índice del texto

* Función: `get_next_text_index(SharedMemory* shm, sem_t* sem_global)`
* **Algoritmo:**

  ```c
  sem_wait(sem_global);
  index = shm->current_txt_index;
  if (index < shm->total_chars_in_file) {
      shm->current_txt_index++;
      shm->total_chars_processed++;
  }
  sem_post(sem_global);
  ```
* **Garantía:** cada emisor obtiene un **índice único** y **creciente**.
  Ningún carácter se **salta** ni se **duplica**.
  *(Mutex: `/sem_global_mutex`)*

## 3.2 Publicación de items y orden de consumo

* El Emisor, para **cada carácter**:

  1. **Espacio**: `sem_wait(/sem_encrypt_spaces)` → si no hay hueco, **duerme**.
  2. **Tomar slot libre**:
     `sem_wait(/sem_encrypt_queue)` → `dequeue_encrypt_slot()` → `sem_post(/sem_encrypt_queue)`
     *(Mutex de la cola de libres)*
  3. **Leer carácter**: `read_char_at_position(shm, txt_index)` (solo lectura de la región de archivo en SHM).
  4. **Encriptar y almacenar** en `CharacterSlot` con `store_character()`:

     * escribe `ascii_value`, `is_valid=1`, `text_index`, `timestamp`, `emisor_pid`
     * **No** hace falta un mutex aquí porque el **slot te pertenece** (salió de la cola de libres); nadie más lo toca.
  5. **Publicar** en cola de datos:
     `sem_wait(/sem_decrypt_queue)` → `enqueue_decrypt_slot(slot_index, txt_index)` → `sem_post(/sem_decrypt_queue)`
     *(Mutex de la cola de datos)*
  6. **Avisar item**: `sem_post(/sem_decrypt_items)`.

* El Receptor:

  1. **Esperar item**: `sem_wait(/sem_decrypt_items)` (bloqueante, sin busy-wait).
  2. **Elegir el **más antiguo** por índice de texto**:
     `sem_wait(/sem_decrypt_queue)` → `dequeue_decrypt_slot_ordered()` → `sem_post(/sem_decrypt_queue)`
     Esta función busca **O(n)** el `text_index` **mínimo** y “rota” la cola para **poperlo**.
     → Así garantizas que los receptores **siempre consumen en orden 0..N-1**, **aunque** los emisores produzcan **fuera de orden**.
     *(Si no te importara el orden en tiempo real, bastaría con escribir cada byte en su `index` con `pwrite`, y el fichero final igual quedaría perfecto. Tú además **visualizas** en orden).*
  3. **Leer `CharacterSlot`** y **desencriptar** (`xor_apply`).
  4. **Escribir salida en offset exacto** (`write_decoded_char()` hace `pwrite(fd, &byte, 1, index)`) → seguro entre procesos, **no depende del orden de llegada**.
  5. **Marcar slot libre** y **devolverlo**:

     * `is_valid=0` (tu slot, nadie te compite)
     * `sem_wait(/sem_encrypt_queue)` → `enqueue_encrypt_slot(slot_index)` → `sem_post(/sem_encrypt_queue)`
     * `sem_post(/sem_encrypt_spaces)` (**liberaste** un hueco de buffer).

### Invariantes claves (y cómo tus semáforos las hacen cumplir)

* **Capacidad**: `0 ≤ encrypt_queue.size ≤ buffer_size` y `0 ≤ decrypt_queue.size ≤ buffer_size`.
  Los mutex de cola impiden corrupción de sus contadores/cabezas/colas.
* **Backpressure correcto**:

  * Emisor **no** puede producir si `encrypt_spaces == 0` → duerme en `sem_wait`.
  * Receptor **no** puede consumir si `decrypt_items == 0` → duerme en `sem_wait`.
* **Conservación**: `encrypt_spaces + decrypt_items == buffer_size`.

  * Emisor **consume** 1 espacio y **crea** 1 item.
  * Receptor **consume** 1 item y **crea** 1 espacio.
* **Happens-before** importante:

  * Emisor **escribe** el `CharacterSlot` **antes** de hacer `sem_post(/sem_decrypt_items)`.
  * Receptor **lee** el slot **después** de despertar por ese `post`.
    → Las llamadas al semáforo sirven como **barrera** de memoria inter-proceso.

### Terminación limpia

* Emisor, al detectar `txt_index >= total_chars_in_file`, **devuelve** el `slot` si lo había tomado y **libera** `encrypt_spaces` para no taponear.
* Receptor verifica (dos veces, antes y después de procesar) si:
  `total_chars_processed >= total_chars_in_file` **y** `decrypt_queue.size == 0`.
  Si sí, sale.
* En apagado, `wake_all_blocked_processes(buffer_size)` hace múltiples `sem_post` a **items** y **spaces** para levantar a cualquiera dormido y permitir que salgan.

---

# 4) Mapa de “qué función toma qué semáforo” (rápido para auditar)

* **Global SHM / contadores / registro**

  * `get_next_text_index` → `sem_wait(/sem_global_mutex)`
  * `register_emisor/_receptor`, `unregister_*`, `save_*_stats` → `sem_wait(/sem_global_mutex)`
* **Cola de libres (encrypt_queue)**

  * `dequeue_encrypt_slot` (emisor) → **dentro** de `sem_wait(/sem_encrypt_queue)`
  * `enqueue_encrypt_slot` (emisor/receptor) → **dentro** de `sem_wait(/sem_encrypt_queue)`
* **Cola de datos (decrypt_queue)**

  * `enqueue_decrypt_slot` (emisor) → **dentro** de `sem_wait(/sem_decrypt_queue)`
  * `dequeue_decrypt_slot_ordered` (receptor) → **dentro** de `sem_wait(/sem_decrypt_queue)`
* **Contadores**

  * Emisor: `sem_wait(/sem_encrypt_spaces)` → `sem_post(/sem_decrypt_items)`
  * Receptor: `sem_wait(/sem_decrypt_items)` → `sem_post(/sem_encrypt_spaces)`

*(Leer/escribir el **slot** concreto no lleva mutex porque el **dueño** del slot está bien definido por las colas; el resto duerme bloqueado por los contadores/mutex.)*

---

# 5) Detalles finos de Linux que estás usando bien

* **System V SHM**:

  * `shmget` con `IPC_CREAT|IPC_EXCL`: creas desde cero; si existe, primero intentas eliminarlo (`shmctl(IPC_RMID)`).
  * `shmat`: mapea el segmento.
  * `shmdt`: des-adjunta.
  * `shmctl(IPC_RMID)`: marca para borrado.
* **POSIX semáforos nombrados** (en `/dev/shm/sem.*`):

  * `sem_open`, `sem_close`, `sem_unlink`, `sem_getvalue`.
  * Bloqueo eficiente (futex); `EINTR` controlado.
* **I/O de salida robusto**:

  * `open_output_file`: `ftruncate(fd, file_size)` prepara el archivo **tamaño exacto** para `pwrite`.
  * `write_decoded_char`: `pwrite(fd, &ch, 1, index)` → **seguro entre procesos** y **no depende del orden**.
* **Señales** (limpieza ordenada): `SIGINT`, `SIGTERM`, `SIGUSR1` → setean `should_terminate`, y tus bucles re-chequean.
* **No busy-wait**: puedes comprobar con `make watch-sleep` o `strace` que verás llamadas a `futex`/`nanosleep`, no bucles activos.

---

# 6) TL;DR de diseño (con la “disponibilidad de espacio” al centro)

* **`/sem_encrypt_spaces`** lleva la **verdad** sobre **cuántos slots libres** hay en el buffer.

  * Emisor **no** puede escribir sin antes **consumir** uno.
  * Receptor **devuelve** un espacio al terminar.
* **`/sem_decrypt_items`** dice cuántos **items** hay listos para consumir.
* **Mutex por cola** evita corrupción de sus estructuras.
* **Mutex global** hace **atómicas** las operaciones sobre contadores y registro.
* **Orden** del texto original se preserva por dos vías:

  1. Asignación atómica de `current_txt_index` a los emisores.
  2. En los receptores, `dequeue_decrypt_slot_ordered()` + `pwrite(index)`.

Con esto, tienes **seguridad**, **progreso garantizado** (sin deadlocks ni esperas activas), y **orden secuencial** del texto, incluso con **muchos procesos** corriendo a la vez.