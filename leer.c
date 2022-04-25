#include "ficheros.h"

int main(int argc, char **argv){
    
    int ninodo = atoi(argv[2]); 
    struct inodo inodo;

    int offset = 0;
    int bytes_leidos = 0;
    int total_bytes_leidos = 0;

    int size = 1500;
    char buffer[size];
    
    //Check syntax
    if(argv[1] == NULL || argv[2] == NULL ){  
        fprintf(stderr,"Command syntax should be: leer <nombre_dispositivo> <nº inodo>\n");
        return -1;
    }

    // Montar el disco
    if (bmount(argv[1]) == -1){
        fprintf(stderr,"Error while mounting\n");
        return -1;
    }

    // Rellenamos el buffer con 0's
    if(memset(buffer, 0, size) == NULL){
        fprintf(stderr, "Error while setting memory\n");
        return -1;
    }

    bytes_leidos = mi_read_f(ninodo, buffer, offset, size);
    //printf("FUERA bytes leidos: %d\n", bytes_leidos); //DEBUG

    while(bytes_leidos > 0){
       // printf(" bytes leidos: %d\n", bytes_leidos); //DEBUG
        write(1, buffer, bytes_leidos); //Motrar resultados por pantalla
        total_bytes_leidos += bytes_leidos;
        offset += size;

        if(memset(buffer, 0, size) == NULL){ // Cleansing
            fprintf(stderr, "Error while setting memory\n");
            return -1;
        }

        bytes_leidos = mi_read_f(ninodo, buffer, offset, size);

    }
    
    leer_inodo(ninodo,&inodo);

    printf("\n\ntotal leidos %d\n", total_bytes_leidos);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    bumount();

    
}