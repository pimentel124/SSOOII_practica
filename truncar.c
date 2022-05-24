#include "ficheros.h"

int main(int argc, char **argv) {
    struct STAT p_stat;
    struct tm *info;
    char adate[80], cdate[80], mdate[80];
    // Check syntax
    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        fprintf(stderr, "Command syntax should be: truncar <nombre_dispositivo> <nº inodo> <nbytes>\n");
        return -1;
    }
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    // Montar el disco
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error while mounting\n");
        return -1;
    }
    if (nbytes == 0) {
        liberar_inodo(ninodo);
    } else {
        truncar_f(ninodo, nbytes);
    }

    mi_stat_f(ninodo, &p_stat);
    printf("DATOS INODO: %d\n", ninodo);
    printf("Tipo: %c\n", p_stat.tipo);
    printf("Permisos: %d\n", p_stat.permisos);
    info = localtime(&p_stat.atime);
    strftime(adate, sizeof(adate), "%a %Y-%m-%d %H:%M:%S", info);
    info = localtime(&p_stat.mtime);
    strftime(mdate, sizeof(mdate), "%a %Y-%m-%d %H:%M:%S", info);
    info = localtime(&p_stat.ctime);
    strftime(cdate, sizeof(cdate), "%a %Y-%m-%d %H:%M:%S", info);
    printf("atime: %s\n", adate);
    printf("mtime: %s\n", mdate);
    printf("ctime: %s\n", cdate);
    printf("numlinks: %d\n", p_stat.nlinks);
    fprintf(stderr, "tamEnBytesLog: %d\n", p_stat.tamEnBytesLog);
    fprintf(stderr, "numBloquesOcupados: %d\n", p_stat.numBloquesOcupados);

    if (bumount() == -1) {
        fprintf(stderr, "Truncar.c -- Error while unmounting\n");
        return -1;
    }
    return 0;
}