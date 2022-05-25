#include "ficheros.h"

int offsets[5] = {9000, 209000, 30725000, 409605000, 480000000};

int main(int argc, char **argv) {
    // Consulta sintaxis correcta
    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        fprintf(stderr, "Sintaxis: escribir <nombre_dispositivo> <\"$(cat fichero)\"> <diferentes_diferentes_inodos>\nOffsets: 9000, 209000, 30725000, 409605000, 480000000\nSi diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n");
        return -1;
    }

    printf("Longitud del texto: %ld\n\n", strlen(argv[2]));

    // Montar dispositivo virtual
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Escribir.c -- Error al montar\n");
        return -1;
    }

    // Reservar inodo
    int ninodo = reservar_inodo('f', 6);
    if (ninodo == -1) {
        fprintf(stderr, "Escribir.c -- Error al reservar inodo\n");
        return -1;
    }
    // Escritura en todos los offsets del array.
    for (int i = 0; i < (sizeof(offsets) / sizeof(int)); i++) {
        printf("Nº inodo reservado: %d\n", ninodo);
        printf("offset: %d\n", offsets[i]);
        int bytesescritos = mi_write_f(ninodo, argv[2], offsets[i], strlen(argv[2]));
        if (bytesescritos == -1) {
            fprintf(stderr, "Escribir.c -- Error al escribir en el inodo\n");
            return -1;
        }
        printf("Bytes escritos: %d\n", bytesescritos);

        // Información inodo escrito
        struct STAT p_stat;
        if (mi_stat_f(ninodo, &p_stat) == -1) {
            fprintf(stderr, "Escribir.c -- Error al obtener información del inodo\n");
            return -1;
        }

        printf("stat.tamEnBytesLog=%d\n", p_stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%d\n\n", p_stat.numBloquesOcupados);

        // Si diferentes_inodos=0, se reserva un solo inodo para todos los offsets
        if (strcmp(argv[3], "0")) {
            if((ninodo = reservar_inodo('f', 6)) == -1){
                fprintf(stderr, "Escribir.c -- Error al reservar inodo\n");
                return -1;
            }
        }
    }

    // Desmontar dispositivo virtual
    if (bumount() == -1) {
        fprintf(stderr, "Escribir.c -- Error al desmontar\n");
        return -1;
    }

    return 0;
}