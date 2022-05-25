/**
 * @file leer.c
 * @author Álvaro Pimentel, Andreu Marqués
 *
 */

#include "ficheros.h"

int main(int argc, char **argv) {
    // Check syntax
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "Command syntax should be: leer <nombre_dispositivo> <nº inodo>\n");
        return -1;
    }

    struct inodo inodo;
    struct superbloque SB;

    int bytes_leidos = 0, offset = 0, total_bytes_leidos = 0, size = 1500;
    char buffer[size];

    // Montar el disco
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error montando el disco\n");
        return -1;
    }
    if(bread(0, &SB) == -1){
        fprintf(stderr, "Error leyendo el superbloque\n");
        return -1;
    }

    // Rellenamos el buffer con 0's
    memset(buffer, 0, size);

    int ninodo = atoi(argv[2]);  // Obtenemos el nº de inodo

    bytes_leidos = mi_read_f(ninodo, buffer, offset, size);
    //fprintf(stderr,"FUERA bytes leidos: %d\n", bytes_leidos); //DEBUG

    while (bytes_leidos > 0) {
        total_bytes_leidos += bytes_leidos;
        write(1, buffer, bytes_leidos);  // Motrar resultados por pantalla
        
        //fprintf(stderr,"Total_leidos: %d\n", total_bytes_leidos); //DEBUG
        memset(buffer, 0, size);
        offset += size;


        bytes_leidos = mi_read_f(ninodo, buffer, offset, size);
        if(bytes_leidos == -1 ){
            fprintf(stderr, "Error bytes\n");
            fprintf(stderr, "total leidos %d\n", total_bytes_leidos);
            return -1;
        }

    }

    // leer inodo

    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Leer.c -- Error leyendo el inodo\n");
        return -1;
    }

    fprintf(stderr, "\ntotal leidos %d\n", total_bytes_leidos);
    fprintf(stderr, "tamEnBytesLog: %u\n", inodo.tamEnBytesLog);

    if (bumount() == -1) {
        fprintf(stderr, "Leer.c -- Error desmontando el disco\n");
        return -1;
    }
    return 0;
}