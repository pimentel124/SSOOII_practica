/**
 * @file mi_escribir.c
 * @author Álvaro Pimentel, Andreu Marqués
 *
 */
#include "directorios.h"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" COLOR_RESET);
        return -1;
    }
    if (bmount(argv[1]) < 0) return -1;
    int escritos = mi_write(argv[2], argv[3], atoi(argv[4]), strlen(argv[3]));
    if (escritos < 0) {
        if (escritos == ERROR_PERMISO_ESCRITURA) {
            fprintf(stderr, COLOR_ERROR "No hay permisos de escritura\n" COLOR_RESET);
        } else {
            fprintf(stderr, "(mi_escribir.c) Error al escribir los bytes\n");
        }
        escritos = 0;
    }
    printf("Longitud del texto: %lu \nBytes escritos: %d\n", strlen(argv[3]), escritos);
    if (bumount() == -1) return -1;
    return 0;
}