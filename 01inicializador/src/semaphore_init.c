#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "semaphore_init.h"
#include "constants.h"

/* Apertura (creación) segura: si existen, se des-referencian y recrean. */
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

/* Cierre de handle (no elimina el nombre). */
static void close_handle(sem_t* h) {
    if (h && h != SEM_FAILED) sem_close(h);
}

/* Creación e inicialización de todos los semáforos requeridos. */
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

/* Elimina los semáforos nombrados del sistema. */
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

/* Obtiene (temporalmente) el valor de un semáforo por nombre para imprimir. */
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

/* Imprime los valores actuales de los semáforos POSIX nombrados. */
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

/*
 * Desbloqueo masivo útil durante el apagado ordenado:
 *  - Publica 'buffer_size' veces en ENCRYPT_SPACES e igualmente en DECRYPT_ITEMS.
 *  - Con esto se evita que queden procesos bloqueados al momento de finalizar.
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
