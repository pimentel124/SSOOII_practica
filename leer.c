#include "ficheros.h"

int main(int argc, char **argv) {
    // Check syntax
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "Command syntax should be: leer <nombre_dispositivo> <nº inodo>\n");
        return -1;
    }
    struct inodo inodo;

    int offset = 0;
    int bytes_leidos = 0;
    int total_bytes_leidos = 0;

    int size = 1500;
    char buffer[size];

    // Montar el disco
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error montando el disco\n");
        return -1;
    }

    // Rellenamos el buffer con 0's
    if (memset(buffer, 0, size) == NULL) {
        fprintf(stderr, "Error while setting memory\n");
        return -1;
    }

    // Leemos el inodo
    int ninodo = atoi(argv[2]);  // Obtenemos el nº de inodo

    bytes_leidos = mi_read_f(ninodo, buffer, offset, size);
    // printf("FUERA bytes leidos: %d\n", bytes_leidos); //DEBUG

    while (bytes_leidos > 0) {
        total_bytes_leidos += bytes_leidos;
        // printf(" bytes leidos: %d\n", bytes_leidos); //DEBUG
        write(1, buffer, bytes_leidos);  // Motrar resultados por pantalla

        if (memset(buffer, 0, size) == NULL) {  // Cleansing
            fprintf(stderr, "Leer.c -- Error limiando memoria\n");
            return -1;
        }

        offset += size;

        bytes_leidos = mi_read_f(ninodo, buffer, offset, size);

        if (bytes_leidos == -1) {
            fprintf(stderr, "leer.c -- Error leyendo el inodo\n");
            return -1;
        }
    }
    // leer inodo

    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Leer.c -- Error leyendo el inodo\n");
        return -1;
    }

    printf("\n\ntotal leidos %d\n", total_bytes_leidos);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);

    if (bumount() == -1) {
        fprintf(stderr, "Leer.c -- Error desmontando el disco\n");
        return -1;
    }
    return 0;
}