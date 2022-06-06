/**
 * @file mi_rm.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa encargado de eliminar un fichero mediante la función mi_unlink()
 */
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_rm <disco> </ruta>\n" COLOR_RESET);
        return -1;
    }
    // Diferenciamos entre fichero y directorio
    const char *camino = argv[2];
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error (mi_rm.c): No es un fichero");
        return -1;
    }
    bmount(argv[1]);
    if (mi_unlink(argv[2]) < 0) return -1;
    bumount();
    return 0;
}