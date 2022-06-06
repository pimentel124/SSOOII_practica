/**
 * @file mi_touch.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa encargado de crear un fichero mediante la función mi_creatw()
 */
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_touch <disco> <permisos> </ruta>\n" COLOR_RESET);
        exit(1);
    }

    // Diferenciamos entre fichero y directorio
    const char *camino = argv[3];
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error (mi_touch.c): No es un fichero");
        return -1;
    }

    char *nombre_fichero = argv[1];
    if (bmount(nombre_fichero) == -1) {
        printf("Error (mi_touch.c) en montar el disco %s\n", nombre_fichero);
        exit(1);
    }

    int permisos = atoi(argv[2]);
    // Hay que comprobar que permisos sea un nº válido (0-7).
    if (permisos < 0 || permisos > 7) {
        printf("Error : modo inválido: <<%d>> \n", permisos);
        exit(1);
    }

    if (mi_creat(camino, permisos) == -1) {
        exit(1);
    }

    if (bumount() == -1) {
        printf("Error (mi_touch.c) en desmontar el disco %s\n", nombre_fichero);
        exit(1);
    }
    return 0;
}