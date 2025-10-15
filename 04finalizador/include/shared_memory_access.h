#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include "structures.h"

/**
 * Acceso a SHM y estadísticas para el finalizador
 *
 * attach_shared_memory()  - Adjunta al segmento ya creado (key fija)
 * detach_shared_memory()  - Desadjunta
 * print_statistics()      - Imprime reporte completo (emisores y receptores)
 */

SharedMemory* attach_shared_memory(void);
void detach_shared_memory(SharedMemory* shm);
void print_statistics(SharedMemory* shm);

#endif // SHARED_MEMORY_ACCESS_H
