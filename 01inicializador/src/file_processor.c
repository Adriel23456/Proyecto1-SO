#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "file_processor.h"
#include "constants.h"

/*
 * Procesar el archivo de entrada y generar el .bin.
 * Lee el archivo completo en memoria, genera un .bin para inspección
 * y retorna el buffer con su tamaño en *file_size.
 */
unsigned char* process_input_file(const char* filename, size_t* file_size) {
    FILE* input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir el archivo '%s': %s\n" RESET,
                filename, strerror(errno));
        return NULL;
    }

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

    if ((size_t)MAX_FILE_SIZE > 0 && size > (size_t)MAX_FILE_SIZE) {
        fprintf(stderr, RED "[ERROR] Tamaño de archivo excede límite configurado: %zu > %zu bytes\n" RESET,
                size, (size_t)MAX_FILE_SIZE);
        fclose(input_file);
        return NULL;
    }

    unsigned char* data = (unsigned char*)malloc(size);
    if (data == NULL) {
        fprintf(stderr, RED "[ERROR] No se pudo reservar memoria: %s\n" RESET, strerror(errno));
        fclose(input_file);
        return NULL;
    }

    size_t bytes_read = fread(data, 1, size, input_file);
    if (bytes_read != size) {
        fprintf(stderr, RED "[ERROR] Lectura incompleta: %zu de %zu bytes\n" RESET, bytes_read, size);
        free(data);
        fclose(input_file);
        return NULL;
    }
    fclose(input_file);

    char bin_filename[512];
    snprintf(bin_filename, sizeof(bin_filename), "%s.bin", filename);
    if (write_binary_file(bin_filename, data, size) == ERROR) {
        free(data);
        return NULL;
    }

    print_file_statistics(data, size);
    *file_size = size;
    return data;
}

/*
 * Escribir archivo binario con permisos 0666.
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

    chmod(filename, 0666);
    return SUCCESS;
}

/*
 * Estadísticas de contenido del archivo.
 */
void print_file_statistics(const unsigned char* data, size_t size) {
    size_t printable_chars = 0, spaces = 0, newlines = 0, others = 0;
    for (size_t i = 0; i < size; i++) {
        unsigned char c = data[i];
        if (c == '\n') newlines++;
        else if (c == ' ' || c == '\t' || c == '\r') spaces++;
        else if (c >= 32 && c < 127) printable_chars++;
        else others++;
    }

    printf("  • Estadísticas del archivo:\n");
    printf("    - Total de caracteres: %zu\n", size);
    printf("    - Caracteres imprimibles: %zu\n", printable_chars);
    printf("    - Espacios y tabs: %zu\n", spaces);
    printf("    - Saltos de línea: %zu\n", newlines);
    printf("    - Otros caracteres: %zu\n", others);
}

/*
 * Leer un carácter desde la SHM en la posición solicitada.
 */
char read_char_at_position(SharedMemory* shm, int position) {
    if (position < 0) return '\0';
    if ((size_t)position >= (size_t)shm->file_data_size) return '\0';
    unsigned char* file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    return (char)file_data[position];
}

/*
 * Verificación básica de integridad de file_data en SHM.
 */
int validate_file_in_shared_memory(SharedMemory* shm) {
    unsigned char* file_data = (unsigned char*)((char*)shm + shm->file_data_offset);
    int limit = (shm->file_data_size < 100) ? shm->file_data_size : 100;
    int non_zero_count = 0;
    for (int i = 0; i < limit; i++) if (file_data[i] != 0) non_zero_count++;
    if (non_zero_count == 0) {
        fprintf(stderr, RED "[ERROR] Los datos del archivo en memoria parecen vacíos\n" RESET);
        return ERROR;
    }
    return SUCCESS;
}
