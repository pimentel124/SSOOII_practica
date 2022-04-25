#include "ficheros.h"

int main(int argc, char **argv){
    struct STAT p_stat;
    struct tm * info;
    char adate[24], cdate[24], mdate[24];
    //Check syntax
    if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL ){  
        fprintf(stderr,"Command syntax should be: truncar <nombre_dispositivo> <nº inodo> <nbytes>\n");
        return -1;
    }
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    // Montar el disco
    if (bmount(argv[1]) == -1){
        fprintf(stderr,"Error while mounting\n");
        return -1;
    }
    if (nbytes == 0){
        liberar_inodo(ninodo);
    }else{
        truncar_f(ninodo, nbytes);
    }
    
    mi_stat_f(ninodo, &p_stat);

    strftime(adate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.atime));
    strftime(cdate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.ctime));
    strftime(mdate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.mtime));
    printf("\nDATOS INODO [%i]\n", ninodo);
    printf("tipo: %c\n", p_stat.tipo);
    printf("permisos: %i\n", p_stat.permisos);
    printf("atime: %s\n", adate);
    printf("mtime: %s\n", mdate);
    printf("Ctime: %s\n", cdate);
    printf("nlinks: %u\n", p_stat.nlinks);
    printf("tamEnBytesLog: %u\n", p_stat.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", p_stat.numBloquesOcupados);
    if(bumount(argv[2]) == -1){
        fprintf(stderr,"Truncar.c -- Error while unmounting\n");
        return -1;
    }
return 0;

    
}