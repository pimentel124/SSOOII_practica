/**
 * @file mi_rmdir.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa encargado de eliminar un directorio mediante la funció mi_unlink()
 */
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_rmdir <disco> </ruta>\n" COLOR_RESET);
        return -1;
    }
    // Se comprueba que el parámetro pasado se trate de un directorio (tiene que empezar por /)
    const char *camino = argv[2];
    if (camino[strlen(camino) - 1] != '/') {
        fprintf(stderr, COLOR_ERROR "Error: La ruta especificada no es un directorio" COLOR_RESET "\n");
        return -1;
    }

    if (bmount(argv[1]) == -1) {
        printf("Error (mi_rmdir.c) en montar el disco \n");
        return -1;
    }

    if (mi_unlink(argv[2]) < 0) return -1;

    if (bumount() == -1) {
        printf("Error (mi_rmdir.c) en desmontar el disco \n");
        return -1;
    }
    return 0;
}