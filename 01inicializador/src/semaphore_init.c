#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include "semaphore_init.h"
#include "constants.h"

// Unión para operaciones de semáforos (requerida por System V)
union semun {
    int val;                    // Valor para SETVAL
    struct semid_ds *buf;       // Buffer para IPC_STAT, IPC_SET
    unsigned short *array;      // Array para GETALL, SETALL
    struct seminfo *__buf;      // Buffer para IPC_INFO (Linux)
};

// Inicializar todos los semáforos del sistema
int initialize_semaphores(int buffer_size) {
    key_t key = SEM_BASE_KEY;
    
    // Intentar eliminar semáforos previos si existen
    int old_semid = semget(key, 0, 0);
    if (old_semid != -1) {
        printf(YELLOW "  ! Semáforos existentes detectados, eliminando...\n" RESET);
        if (semctl(old_semid, 0, IPC_RMID) == -1) {
            fprintf(stderr, RED "  [WARNING] No se pudieron eliminar semáforos previos\n" RESET);
        }
    }
    
    // Crear el conjunto de semáforos
    int sem_id = semget(key, NUM_SEMAPHORES, IPC_CREAT | IPC_EXCL | IPC_PERMS);
    if (sem_id == -1) {
        fprintf(stderr, RED "[ERROR] semget falló: %s\n" RESET, strerror(errno));
        return ERROR;
    }
    
    printf("  • Conjunto de semáforos creado con ID: %d\n", sem_id);
    printf("  • Key: 0x%04X\n", key);
    printf("  • Número de semáforos: %d\n", NUM_SEMAPHORES);
    
    // Inicializar cada semáforo con su valor inicial
    union semun arg;
    
    // sem_global_mutex: mutex para acceso global (inicializado a 1)
    arg.val = 1;
    if (semctl(sem_id, SEM_GLOBAL_MUTEX, SETVAL, arg) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo inicializar sem_global_mutex: %s\n" RESET, 
                strerror(errno));
        semctl(sem_id, 0, IPC_RMID);
        return ERROR;
    }
    
    // sem_encrypt_queue: mutex para cola de encriptación (inicializado a 1)
    arg.val = 1;
    if (semctl(sem_id, SEM_ENCRYPT_QUEUE, SETVAL, arg) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo inicializar sem_encrypt_queue: %s\n" RESET, 
                strerror(errno));
        semctl(sem_id, 0, IPC_RMID);
        return ERROR;
    }
    
    // sem_decrypt_queue: mutex para cola de desencriptación (inicializado a 1)
    arg.val = 1;
    if (semctl(sem_id, SEM_DECRYPT_QUEUE, SETVAL, arg) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo inicializar sem_decrypt_queue: %s\n" RESET, 
                strerror(errno));
        semctl(sem_id, 0, IPC_RMID);
        return ERROR;
    }
    
    // sem_encrypt_spaces: contador de espacios disponibles (inicializado a buffer_size)
    arg.val = buffer_size;
    if (semctl(sem_id, SEM_ENCRYPT_SPACES, SETVAL, arg) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo inicializar sem_encrypt_spaces: %s\n" RESET, 
                strerror(errno));
        semctl(sem_id, 0, IPC_RMID);
        return ERROR;
    }
    
    // sem_decrypt_items: contador de items disponibles (inicializado a 0)
    arg.val = 0;
    if (semctl(sem_id, SEM_DECRYPT_ITEMS, SETVAL, arg) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo inicializar sem_decrypt_items: %s\n" RESET, 
                strerror(errno));
        semctl(sem_id, 0, IPC_RMID);
        return ERROR;
    }
    
    // Verificar valores iniciales
    print_semaphore_values(sem_id, buffer_size);
    
    return sem_id;
}

// Obtener el ID del conjunto de semáforos existente
int get_semaphore_set() {
    key_t key = SEM_BASE_KEY;
    
    // Obtener el conjunto existente (no crear nuevo)
    int sem_id = semget(key, NUM_SEMAPHORES, 0);
    if (sem_id == -1) {
        fprintf(stderr, RED "[ERROR] No se encontraron semáforos con key 0x%04X: %s\n" RESET, 
                key, strerror(errno));
        return ERROR;
    }
    
    return sem_id;
}

// Operación wait (P) en un semáforo
int sem_wait_custom(int sem_id, int sem_num) {
    struct sembuf operation;
    
    operation.sem_num = sem_num;  // Número del semáforo
    operation.sem_op = -1;         // Decrementar
    operation.sem_flg = 0;         // Bloquear si es necesario
    
    if (semop(sem_id, &operation, 1) == -1) {
        if (errno != EINTR) {  // Ignorar interrupciones por señales
            fprintf(stderr, RED "[ERROR] sem_wait en semáforo %d: %s\n" RESET, 
                    sem_num, strerror(errno));
        }
        return ERROR;
    }
    
    return SUCCESS;
}

// Operación post (V) en un semáforo
int sem_post_custom(int sem_id, int sem_num) {
    struct sembuf operation;
    
    operation.sem_num = sem_num;  // Número del semáforo
    operation.sem_op = 1;          // Incrementar
    operation.sem_flg = 0;         // No bloquear
    
    if (semop(sem_id, &operation, 1) == -1) {
        fprintf(stderr, RED "[ERROR] sem_post en semáforo %d: %s\n" RESET, 
                sem_num, strerror(errno));
        return ERROR;
    }
    
    return SUCCESS;
}

// Operación trywait en un semáforo (no bloqueante)
int sem_trywait_custom(int sem_id, int sem_num) {
    struct sembuf operation;
    
    operation.sem_num = sem_num;  // Número del semáforo
    operation.sem_op = -1;         // Decrementar
    operation.sem_flg = IPC_NOWAIT; // No bloquear
    
    if (semop(sem_id, &operation, 1) == -1) {
        if (errno == EAGAIN) {
            return ERROR;  // Semáforo no disponible
        }
        fprintf(stderr, RED "[ERROR] sem_trywait en semáforo %d: %s\n" RESET, 
                sem_num, strerror(errno));
        return ERROR;
    }
    
    return SUCCESS;
}

// Obtener el valor actual de un semáforo
int get_semaphore_value(int sem_id, int sem_num) {
    int value = semctl(sem_id, sem_num, GETVAL);
    if (value == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo obtener valor del semáforo %d: %s\n" RESET, 
                sem_num, strerror(errno));
        return -1;
    }
    return value;
}

// Imprimir los valores actuales de todos los semáforos
void print_semaphore_values(int sem_id, int buffer_size) {
    printf("\n  • Valores iniciales de los semáforos:\n");
    
    int val = get_semaphore_value(sem_id, SEM_GLOBAL_MUTEX);
    printf("    [%d] sem_global_mutex: %d (mutex global)\n", SEM_GLOBAL_MUTEX, val);
    
    val = get_semaphore_value(sem_id, SEM_ENCRYPT_QUEUE);
    printf("    [%d] sem_encrypt_queue: %d (mutex cola encriptación)\n", SEM_ENCRYPT_QUEUE, val);
    
    val = get_semaphore_value(sem_id, SEM_DECRYPT_QUEUE);
    printf("    [%d] sem_decrypt_queue: %d (mutex cola desencriptación)\n", SEM_DECRYPT_QUEUE, val);
    
    val = get_semaphore_value(sem_id, SEM_ENCRYPT_SPACES);
    printf("    [%d] sem_encrypt_spaces: %d/%d (espacios disponibles)\n", 
           SEM_ENCRYPT_SPACES, val, buffer_size);
    
    val = get_semaphore_value(sem_id, SEM_DECRYPT_ITEMS);
    printf("    [%d] sem_decrypt_items: %d (items para leer)\n", SEM_DECRYPT_ITEMS, val);
}

// Limpiar todos los semáforos (solo el finalizador debe hacer esto)
int cleanup_semaphores() {
    int sem_id = get_semaphore_set();
    if (sem_id == ERROR) {
        return ERROR;
    }
    
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        fprintf(stderr, RED "[ERROR] No se pudieron eliminar los semáforos: %s\n" RESET, 
                strerror(errno));
        return ERROR;
    }
    
    printf(GREEN "  ✓ Semáforos eliminados correctamente\n" RESET);
    return SUCCESS;
}

// Despertar todos los procesos bloqueados (para finalización)
void wake_all_blocked_processes(int sem_id, int buffer_size) {
    // Incrementar sem_encrypt_spaces múltiples veces para despertar emisores
    for (int i = 0; i < buffer_size; i++) {
        sem_post_custom(sem_id, SEM_ENCRYPT_SPACES);
    }
    
    // Incrementar sem_decrypt_items múltiples veces para despertar receptores
    for (int i = 0; i < buffer_size; i++) {
        sem_post_custom(sem_id, SEM_DECRYPT_ITEMS);
    }
    
    printf(YELLOW "  ! Todos los procesos bloqueados han sido despertados\n" RESET);
}