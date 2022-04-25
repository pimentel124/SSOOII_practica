#include "ficheros.h"

int printSTAT(int ninodo);

struct superbloque SB;
char adate[24], cdate[24], mdate[24];  // Tiempos
struct STAT p_stat;
int offset[5] = {9000, 209000, 30725000, 409605000, 480000000};
struct tm * info;
int aux1, aux2 =0;


int main(int argc, char **argv){

    if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL){ // Checkear syntax 
        fprintf(stderr,"Command syntax should be: escribir <nombre_dispositivo> <\"$(cat fichero)\"> <diferentes_diferentes_inodos>\n");
        return -1;
    }

    unsigned int length = strlen(argv[2]);
    int diferentes_inodos = atoi(argv[3]);
    int nBytes, ninodo = 0;
    const char *dir;
    dir = argv[1];
    char buff[length];
    
    if (bmount(dir) == -1){
        fprintf(stderr,"Error while mounting\n");
        return -1;
    }

    if(bread(posSB, &SB) == -1){ // miramos los valores actuales del superbloque
        fprintf(stderr, "Error while reading\n");
        return -1;
    }

    if(diferentes_inodos == 0){             // Mismo inodo para todos los offsets
        ninodo = reservar_inodo('f', 6); 
        strcpy(buff, argv[2]);              // Copiamos el texto pasado por parametro al buffer
        for(int i = 0; i < 5; i++){         // Recorremos todos los offset 

            printf("Se ha reservado el inodo[%d] con offset: %d\n ", ninodo, offset[i]);
            //  nBytes += mi_write_f(ninodo, buff, offset[i], length); // Guardamos la cantidad de bytes escritos
            aux1 = mi_write_f(ninodo, buff, offset[i], length);
            if(memset(buff, 0, sizeof(buff)) == NULL){
                fprintf(stderr, "Error");
                return -1;
            }

            aux2 = mi_read_f(ninodo, buff, offset[i], length); //No sabemos si esto es necesario, pero antes estaba comentado y no ha cambiado su comportamiento
            nBytes += aux1;
            

            // DEBUG
            printf("write : %d\t read: %d\n",aux1, aux2 );
        
            // DEBUG
            printSTAT(ninodo);

        }
    }else if(diferentes_inodos == 1){ // Puede dar -1 al haber un error
        strcpy(buff, argv[2]);
        for(int i = 0; i < 5; i++){
            ninodo = reservar_inodo('f', 6);
            printf("Se ha reservado el inodo[%d] con offset: %d ", ninodo, offset[i]);
            aux1 = mi_write_f(ninodo, buff, offset[i], length);
            if(memset(buff, 0, sizeof(buff)) == NULL){
                fprintf(stderr, "Error");
                return -1;
            }
            aux2 = mi_read_f(ninodo, buff, offset[i], length);
            nBytes += aux1;

            // DEBUG
            printf("write : %d\t read: %d\n",aux1, aux2 );
            printSTAT(ninodo);
        }
        
    }

    if(bumount() == -1){
        fprintf(stderr, "Error while unmounting\n");
        return -1;
    }
    return 0;
}

int printSTAT(int ninodo){
    mi_stat_f(ninodo, &p_stat);

    strftime(adate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.atime));
    strftime(cdate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.ctime));
    strftime(mdate, 24, "%a %d-%m-%Y %H:%M:%S", info = localtime(&p_stat.mtime));
    printf("DATOS INODO [%i]\n\
    tipo= %c\n\
    permisos= %i\n\
    atime: %s\n\
    ctime: %s\n\
    mtime: %s\n\
    nlinks: %i\n\
    tamEnBytesLog= %i\n\
    numBloquesOcupados= %i\n",
    ninodo, p_stat.tipo, p_stat.permisos, adate, cdate, mdate, p_stat.nlinks, p_stat.tamEnBytesLog, p_stat.numBloquesOcupados);
    return 0;
}