/**
 * @file mi_link.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa encargado de crear un enlace a un fichero mediante la función mi_link()
 */
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_link <disco> </ruta_fichero_original> </ruta_enlace>\n" COLOR_RESET);
        return -1;
    }
    bmount(argv[1]);
    if (strlen(argv[2]) - 1 == '/') {
        fprintf(stderr, "(mi_link)Es un directorio, no un fichero\n");
        return -1;
    }
    if (mi_link(argv[2], argv[3]) < 0) return -1;
    bumount();
    return 0;
}