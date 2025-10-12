#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // fstat
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "file_processor.h"
#include "constants.h"

/*
 * Procesar el archivo de entrada y generar el .bin
 * - Lee el archivo completo a memoria (buffer dinámico).
 * - Escribe una copia .bin (mismo contenido, orden natural).
 * - Imprime estadísticas básicas del contenido.
 * - Devuelve el puntero al buffer y coloca el tamaño en *file_size.
 *
 * Importante:
 *  - El llamador debe liberar el buffer devuelto con free().
 *  - Se usan tipos size_t para tamaños y conteos de bytes.
 */
unsigned char* process_input_file(const char* filename, size_t* file_size) {
    FILE* input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir el archivo '%s': %s\n" RESET,
                filename, strerror(errno));
        return NULL;
    }

    // Obtener tamaño del archivo de forma robusta
    struct stat st;
    if (fstat(fileno(input_file), &st) == -1) {
        fprintf(stderr, RED "[ERROR] fstat falló: %s\n" RESET, strerror(errno));
        fclose(input_file);
        return NULL;
    }

    size_t size = (size_t)st.st_size;
    if (size == 0) {
        fprintf(stderr, RED "[ERROR] Tamaño de archivo inválido: %zu bytes\n" RESET, size);
        fclose(input_file);
        return NULL;
    }

    // Límite opcional por configuración (si se desea mantener esta salvaguarda)
    if ((size_t)MAX_FILE_SIZE > 0 && size > (size_t)MAX_FILE_SIZE) {
        fprintf(stderr, RED "[ERROR] Tamaño excede límite configurado: %zu > %zu bytes\n" RESET,
                size, (size_t)MAX_FILE_SIZE);
        fclose(input_file);
        return NULL;
    }

    // Reservar memoria para leer todo el archivo
    unsigned char* data = (unsigned char*)malloc(size);
    if (data == NULL) {
        fprintf(stderr, RED "[ERROR] No se pudo reservar memoria: %s\n" RESET, strerror(errno));
        fclose(input_file);
        return NULL;
    }

    // Leer el contenido completo
    size_t bytes_read = fread(data, 1, size, input_file);
    if (bytes_read != size) {
        fprintf(stderr, RED "[ERROR] Lectura incompleta: %zu de %zu bytes\n" RESET,
                bytes_read, size);
        free(data);
        fclose(input_file);
        return NULL;
    }
    fclose(input_file);

    // Generar archivo .bin para verificación/inspección
    char bin_filename[512];
    snprintf(bin_filename, sizeof(bin_filename), "%s.bin", filename);
    if (write_binary_file(bin_filename, data, size) == ERROR) {
        free(data);
        return NULL;
    }

    // Mostrar estadísticas del contenido leído
    print_file_statistics(data, size);

    // Entregar resultados al llamador
    *file_size = size;
    return data;
}

/*
 * Escribir un archivo binario a disco.
 * - Devuelve SUCCESS si se escribió todo el bloque; ERROR en caso contrario.
 * - Establece permisos 0666 para que otros procesos puedan leerlo.
 */
int write_binary_file(const char* filename, const unsigned char* data, size_t size) {
    FILE* bin_file = fopen(filename, "wb");
    if (bin_file == NULL) {
        fprintf(stderr, RED "[ERROR] No se pudo crear '%s': %s\n" RESET, filename, strerror(errno));
        return ERROR;
    }

    size_t bytes_written = fwrite(data, 1, size, bin_file);
    if (bytes_written != size) {
        fprintf(stderr, RED "[ERROR] Escritura incompleta: %zu de %zu bytes\n" RESET,
                bytes_written, size);
        fclose(bin_file);
        return ERROR;
    }

    fclose(bin_file);

    // Permisos de lectura/escritura para asegurar acceso entre procesos
    chmod(filename, 0666);
    return SUCCESS;
}

/*
 * Imprimir estadísticas del archivo:
 * - Total de bytes
 * - Distribución aproximada de caracteres (imprimibles, espacios/tabs, saltos de línea, otros)
 */
void print_file_statistics(const unsigned char* data, size_t size) {
    size_t printable_chars = 0;
    size_t spaces = 0;
    size_t newlines = 0;
    size_t others = 0;

    for (size_t i = 0; i < size; i++) {
        unsigned char c = data[i];
        if (c == '\n') {
            newlines++;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            spaces++;
        } else if (c >= 32 && c < 127) {
            printable_chars++;
        } else {
            others++;
        }
    }

    printf("  • Estadísticas del archivo:\n");
    printf("    - Total de caracteres: %zu\n", size);
    printf("    - Caracteres imprimibles: %zu\n", printable_chars);
    printf("    - Espacios y tabs: %zu\n", spaces);
    printf("    - Saltos de línea: %zu\n", newlines);
    printf("    - Otros caracteres: %zu\n", others);
}

/*
 * Leer un carácter en una posición específica (para emisores).
 * - Si la posición no es válida, retorna '\0'.
 * - Accede al bloque de datos del archivo mapeado en memoria compartida.
 */
char read_char_at_position(SharedMemory* shm, int position) {
    if (position < 0) return '\0';
    if ((size_t)position >= (size_t)shm->file_data_size) return '\0';

    unsigned char* file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    return (char)file_data[position];
}

/*
 * Validar la integridad del archivo en memoria compartida.
 * - Verificación simple: contar bytes no nulos en los primeros 100 (si hay).
 */
int validate_file_in_shared_memory(SharedMemory* shm) {
    unsigned char* file_data = (unsigned char*)((char*)shm + shm->file_data_offset);

    int non_zero_count = 0;
    int limit = (shm->file_data_size < 100) ? shm->file_data_size : 100;
    for (int i = 0; i < limit; i++) {
        if (file_data[i] != 0) {
            non_zero_count++;
        }
    }

    if (non_zero_count == 0) {
        fprintf(stderr, RED "[ERROR] Los datos del archivo en memoria parecen vacíos\n" RESET);
        return ERROR;
    }

    return SUCCESS;
}
