#include "output_file.h"

/**
 * Módulo de Manejo de Archivos de Salida
 * 
 * Este módulo se encarga de gestionar los archivos de salida donde
 * se escriben los datos desencriptados. Maneja la creación de directorios,
 * apertura de archivos y escritura segura de datos en posiciones
 * específicas del archivo.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/**
 * @brief Asegura que un directorio existe
 * 
 * Verifica si un directorio existe y si no, intenta crearlo.
 * También verifica que la ruta sea realmente un directorio.
 * 
 * @param dir Ruta del directorio a verificar/crear
 * @return 0 si el directorio existe o fue creado, -1 en caso de error
 */
static int ensure_dir(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        errno = ENOTDIR;
        return -1;
    }
    return mkdir(dir, 0777);
}

/**
 * @brief Versión segura de basename que no modifica el input
 * 
 * Extrae el nombre de archivo de una ruta sin modificar la cadena original.
 * Si la ruta está vacía o es solo "/", usa "output" como valor predeterminado.
 * 
 * @param path Ruta de la cual extraer el nombre de archivo
 * @param out Buffer donde se almacenará el resultado
 * @param outsz Tamaño del buffer de salida
 */
static void safe_basename(const char* path, char* out, size_t outsz) {
    if (!path || !out || outsz == 0) return;
    
    // Encontrar la última barra
    const char* last_slash = strrchr(path, '/');
    const char* name = last_slash ? (last_slash + 1) : path;
    
    // Si está vacío o es solo "/", usar "output"
    if (!name[0]) {
        strncpy(out, "output", outsz - 1);
        out[outsz - 1] = '\0';
        return;
    }
    
    strncpy(out, name, outsz - 1);
    out[outsz - 1] = '\0';
}

/**
 * @brief Abre o crea un archivo de salida para escribir datos desencriptados
 * 
 * Crea un archivo de salida basado en el nombre del archivo de entrada.
 * El archivo se crea en el directorio especificado por la variable de entorno
 * RECEPTOR_OUT_DIR, o en ./out si no está definida. Si no se puede crear
 * en la ubicación preferida, intenta crear en el directorio actual.
 * 
 * @param shm_input_filename Nombre del archivo de entrada (desde memoria compartida)
 * @param file_size Tamaño esperado del archivo (para pre-dimensionar)
 * @param out_path Buffer donde se almacenará la ruta del archivo creado
 * @param out_path_sz Tamaño del buffer out_path
 * @return Descriptor del archivo abierto, o -1 en caso de error
 */
int open_output_file(const char* shm_input_filename,
                     int file_size,
                     char* out_path,
                     size_t out_path_sz) {
    if (!shm_input_filename || !out_path || out_path_sz < 8) {
        errno = EINVAL; return -1;
    }

    // Directorio de salida configurable por variable de entorno
    const char* dir = getenv("RECEPTOR_OUT_DIR");
    if (!dir || !*dir) dir = "./out";

    if (ensure_dir(dir) == -1 && errno != EEXIST) {
        // si falló crear ./out (o RECEPTOR_OUT_DIR), usamos el cwd como fallback
        dir = ".";
    }

    char base[PATH_MAX + 1];
    safe_basename(shm_input_filename, base, sizeof(base));
    
    // Si no hay basename válido, usar "output"
    if (!base[0]) {
        strncpy(base, "output", sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
    }

    // Ruta final preferida: <dir>/<basename>.dec.bin
    int need = snprintf(out_path, out_path_sz, "%s/%s.dec.bin", dir, base);
    if (need < 0 || (size_t)need >= out_path_sz) {
        errno = ENAMETOOLONG;
        return -1;
    }

    int fd = open(out_path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        // último fallback: ./<basename>.dec.bin
        need = snprintf(out_path, out_path_sz, "./%s.dec.bin", base);
        if (need < 0 || (size_t)need >= out_path_sz) {
            errno = ENAMETOOLONG; return -1;
        }
        fd = open(out_path, O_CREAT | O_RDWR, 0666);
        if (fd == -1) return -1;
    }

    // Pre-dimensionar para permitir escrituras aleatorias con pwrite
    if (file_size > 0) {
        if (ftruncate(fd, file_size) == -1) {
            // No es fatal: seguimos, pero dejamos el aviso
            fprintf(stderr, "WARN: ftruncate('%s', %d) falló: %s\n",
                    out_path, file_size, strerror(errno));
        }
    }
    return fd;
}

/**
 * @brief Escribe un carácter desencriptado en una posición específica del archivo
 * 
 * Utiliza pwrite para escribir un carácter en una posición específica del archivo
 * sin modificar el puntero de lectura/escritura del archivo.
 * 
 * @param fd Descriptor del archivo donde escribir
 * @param index Posición donde escribir el carácter
 * @param ch Carácter a escribir
 * @return 0 en caso de éxito, -1 en caso de error
 */
int write_decoded_char(int fd, int index, unsigned char ch) {
    if (fd < 0 || index < 0) { errno = EINVAL; return -1; }
    ssize_t w = pwrite(fd, &ch, 1, (off_t)index);
    return (w == 1) ? 0 : -1;
}

/**
 * @brief Cierra un archivo de salida
 * 
 * Cierra el descriptor de archivo proporcionado si es válido.
 * 
 * @param fd Descriptor del archivo a cerrar
 * @return 0 en caso de éxito, -1 en caso de error
 */
int close_output_file(int fd) {
    if (fd >= 0) return close(fd);
    return 0;
}