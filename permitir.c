#include "ficheros.h"

int main(int argc, char const **argv){
    
    //Check syntax
    if(argv[1] == NULL || argv[2] == NULL ){  
        fprintf(stderr,"Command syntax should be: permitir <nombre_dispositivo> <ninodo> <permisos>\n");
        return -1;
    }


    if (bmount(argv[1]) == -1)
    {
        fprintf(stderr, "Permitir.c -- Error al montar\n");
        return -1;
    }
    if(mi_chmod_f(atoi(argv[2]), atoi(argv[3])) == -1)
    {
        fprintf(stderr, "Permitir.c -- Error al cambiar permisos\n");
        return -1;
    }

    if (bumount() == -1)
    {
        fprintf(stderr, "Permitir.c -- Error al desmontar\n");
        return -1;
    }

    return 0;
    
}