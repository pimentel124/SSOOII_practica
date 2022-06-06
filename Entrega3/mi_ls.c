/**
 * @file mi_ls.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa que emula el uso de el comando $ ls en el bash de linux. En este caso se emplea la función mi_dir()
 */
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_ls <disco> </ruta_directorio>\n" COLOR_RESET);
        printf("Sintaxis: ./mi_ls <disco> </ruta_directorio> \n");
        return -1;
    }

    char tipo;
    const char *camino = argv[2];
    if (camino[strlen(camino) - 1] == '/') {  //si empieza por / es un directorio
        tipo = 'd';
    } else {                                  //En caso contrario se trata de un fichero
        tipo = 'f';
    }

    if (bmount(argv[1]) == -1) {
        printf("Error (mi_ls.c) en montar el disco \n");
        return -1;
    }

    // struct superbloque SB;
    char buffer[BLOCKSIZE];
    memset(buffer, 0, sizeof(buffer));
    int dir = mi_dir(camino, buffer, tipo);
    if (dir < 0) {
        return -1;
    } else {
        if (tipo == 'd') {                             //si se trata de un directorio
            printf("Total entradas: %d\n\n", dir);     //se muestra el numero de entradas
        }
        printf("Tipo    Permisos    mTime                Tamaño         Nombre\n");
        printf("---------------------------------------------------------------\n");
        printf("%s\n", buffer);
    }
    if (bumount() == -1) return -1;
    return 0;
}