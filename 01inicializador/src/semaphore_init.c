#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "semaphore_init.h"
#include "constants.h"

/**
 * Módulo de Inicialización de Semáforos
 * 
 * Este módulo maneja la creación y gestión de los semáforos POSIX nombrados
 * utilizados para la sincronización del sistema. Los semáforos incluyen:
 * - Mutex global para acceso a memoria compartida
 * - Mutex para colas de encriptación y desencriptación
 * - Semáforos contadores para espacios disponibles e items pendientes
 */

/**
 * @brief Crea un semáforo POSIX nombrado de manera segura
 * 
 * Si el semáforo ya existe, lo elimina y crea uno nuevo con los
 * valores especificados. Esta función garantiza un estado limpio
 * del semáforo.
 * 
 * @param name Nombre del semáforo (debe empezar con /)
 * @param initial_value Valor inicial del semáforo
 * @param out Puntero donde se almacenará el handle del semáforo
 * @return SUCCESS si la operación fue exitosa, ERROR en caso contrario
 */
static int create_named_semaphore(const char* name, unsigned int initial_value, sem_t** out) {
    // Cerramos residuos previos si los hubiera
    sem_unlink(name);

    sem_t* h = sem_open(name, O_CREAT | O_EXCL, IPC_PERMS, initial_value);
    if (h == SEM_FAILED) {
        fprintf(stderr, RED "[ERROR] sem_open('%s') falló: %s\n" RESET, name, strerror(errno));
        return ERROR;
    }
    if (out) *out = h;
    return SUCCESS;
}

/**
 * @brief Cierra el handle de un semáforo
 * 
 * Cierra el handle del semáforo pero mantiene el semáforo en el sistema.
 * No elimina el nombre del semáforo, solo libera recursos del proceso.
 * 
 * @param h Handle del semáforo a cerrar
 */
static void close_handle(sem_t* h) {
    if (h && h != SEM_FAILED) sem_close(h);
}

/**
 * @brief Crea e inicializa todos los semáforos del sistema
 * 
 * Inicializa los cinco semáforos necesarios para la sincronización:
 * 1. Mutex global para acceso a memoria compartida
 * 2. Mutex para cola de encriptación
 * 3. Mutex para cola de desencriptación
 * 4. Contador de espacios disponibles (buffer_size inicial)
 * 5. Contador de items pendientes (0 inicial)
 * 
 * @param buffer_size Tamaño del buffer circular
 * @return SUCCESS si la operación fue exitosa, ERROR en caso contrario
 */
int initialize_semaphores(int buffer_size) {
    sem_t *g = NULL, *eq = NULL, *dq = NULL, *es = NULL, *di = NULL;

    printf("  • Creando semáforos POSIX nombrados:\n");

    if (create_named_semaphore(SEM_NAME_GLOBAL_MUTEX,   1, &g)  != SUCCESS) return ERROR;
    if (create_named_semaphore(SEM_NAME_ENCRYPT_QUEUE,  1, &eq) != SUCCESS) { close_handle(g); return ERROR; }
    if (create_named_semaphore(SEM_NAME_DECRYPT_QUEUE,  1, &dq) != SUCCESS) { close_handle(g); close_handle(eq); return ERROR; }
    if (create_named_semaphore(SEM_NAME_ENCRYPT_SPACES, (unsigned int)buffer_size, &es) != SUCCESS) {
        close_handle(g); close_handle(eq); close_handle(dq); return ERROR;
    }
    if (create_named_semaphore(SEM_NAME_DECRYPT_ITEMS,  0, &di) != SUCCESS) {
        close_handle(g); close_handle(eq); close_handle(dq); close_handle(es);
        return ERROR;
    }

    // Imprimir nombres creados
    printf("    - %s\n", SEM_NAME_GLOBAL_MUTEX);
    printf("    - %s\n", SEM_NAME_ENCRYPT_QUEUE);
    printf("    - %s\n", SEM_NAME_DECRYPT_QUEUE);
    printf("    - %s (valor inicial: %d)\n", SEM_NAME_ENCRYPT_SPACES, buffer_size);
    printf("    - %s (valor inicial: 0)\n",   SEM_NAME_DECRYPT_ITEMS);

    // Mostrar valores de arranque
    print_semaphore_values();

    // Cerramos los handles locales (otros procesos los abrirán por nombre)
    close_handle(g); close_handle(eq); close_handle(dq); close_handle(es); close_handle(di);
    return SUCCESS;
}

/**
 * @brief Elimina todos los semáforos del sistema
 * 
 * Elimina los cinco semáforos POSIX nombrados del sistema.
 * Esta función debe llamarse durante la limpieza final del sistema.
 * Ignora errores si los semáforos ya no existen.
 * 
 * @return SUCCESS si todos los semáforos fueron eliminados, ERROR en caso contrario
 */
int cleanup_semaphores(void) {
    int ok = SUCCESS;
    if (sem_unlink(SEM_NAME_GLOBAL_MUTEX)   == -1 && errno != ENOENT) ok = ERROR;
    if (sem_unlink(SEM_NAME_ENCRYPT_QUEUE)  == -1 && errno != ENOENT) ok = ERROR;
    if (sem_unlink(SEM_NAME_DECRYPT_QUEUE)  == -1 && errno != ENOENT) ok = ERROR;
    if (sem_unlink(SEM_NAME_ENCRYPT_SPACES) == -1 && errno != ENOENT) ok = ERROR;
    if (sem_unlink(SEM_NAME_DECRYPT_ITEMS)  == -1 && errno != ENOENT) ok = ERROR;

    if (ok == SUCCESS) {
        printf(GREEN "  ✓ Semáforos POSIX eliminados correctamente\n" RESET);
    } else {
        fprintf(stderr, RED "  [ERROR] No se pudieron eliminar uno o más semáforos POSIX\n" RESET);
    }
    return ok;
}

/**
 * @brief Lee el valor actual de un semáforo por nombre
 * 
 * Abre temporalmente un semáforo, lee su valor actual y lo cierra.
 * Útil para monitoreo y debugging del estado del sistema.
 * 
 * @param name Nombre del semáforo
 * @param out Puntero donde se almacenará el valor
 * @return SUCCESS si la lectura fue exitosa, ERROR en caso contrario
 */
static int get_value_of(const char* name, int* out) {
    sem_t* h = sem_open(name, 0);
    if (h == SEM_FAILED) return ERROR;
    int v = 0;
    if (sem_getvalue(h, &v) == -1) {
        sem_close(h);
        return ERROR;
    }
    sem_close(h);
    if (out) *out = v;
    return SUCCESS;
}

/**
 * @brief Muestra los valores actuales de todos los semáforos
 * 
 * Imprime un reporte del estado actual de todos los semáforos
 * del sistema, incluyendo sus valores y propósito. Útil para
 * diagnóstico y monitoreo del sistema.
 */
void print_semaphore_values(void) {
    int v;

    printf("\n  • Valores actuales de semáforos POSIX:\n");

    if (get_value_of(SEM_NAME_GLOBAL_MUTEX, &v) == SUCCESS)
        printf("    %s: %d (mutex global)\n", SEM_NAME_GLOBAL_MUTEX, v);
    else
        printf("    %s: <no disponible>\n", SEM_NAME_GLOBAL_MUTEX);

    if (get_value_of(SEM_NAME_ENCRYPT_QUEUE, &v) == SUCCESS)
        printf("    %s: %d (mutex cola encriptación)\n", SEM_NAME_ENCRYPT_QUEUE, v);
    else
        printf("    %s: <no disponible>\n", SEM_NAME_ENCRYPT_QUEUE);

    if (get_value_of(SEM_NAME_DECRYPT_QUEUE, &v) == SUCCESS)
        printf("    %s: %d (mutex cola desencriptación)\n", SEM_NAME_DECRYPT_QUEUE, v);
    else
        printf("    %s: <no disponible>\n", SEM_NAME_DECRYPT_QUEUE);

    if (get_value_of(SEM_NAME_ENCRYPT_SPACES, &v) == SUCCESS)
        printf("    %s: %d (espacios disponibles)\n", SEM_NAME_ENCRYPT_SPACES, v);
    else
        printf("    %s: <no disponible>\n", SEM_NAME_ENCRYPT_SPACES);

    if (get_value_of(SEM_NAME_DECRYPT_ITEMS, &v) == SUCCESS)
        printf("    %s: %d (items para leer)\n", SEM_NAME_DECRYPT_ITEMS, v);
    else
        printf("    %s: <no disponible>\n", SEM_NAME_DECRYPT_ITEMS);

    printf("\n");
}

/**
 * @brief Desbloquea todos los procesos durante el apagado
 * 
 * Realiza un "desbloqueo masivo" incrementando los semáforos
 * de espacios y items buffer_size veces. Esto garantiza que
 * ningún proceso quede bloqueado indefinidamente durante
 * el apagado del sistema.
 * 
 * @param buffer_size Tamaño del buffer circular
 */
void wake_all_blocked_processes(int buffer_size) {
    sem_t *es = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    sem_t *di = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);

    if (es != SEM_FAILED) {
        for (int i = 0; i < buffer_size; i++) sem_post(es);
        sem_close(es);
    }
    if (di != SEM_FAILED) {
        for (int i = 0; i < buffer_size; i++) sem_post(di);
        sem_close(di);
    }
    printf(YELLOW "  ! Procesos bloqueados despertados (POSIX)\n" RESET);
}
