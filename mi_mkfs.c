#include "ficheros_basico.h"

// Uso de mi_mkfs: ./mi_mkfs <nombre_dispositivo> <nbloques>
// argc=3            ^ argv[0]     ^ argv[1]         ^ argv[2]

int main(int argc, char **argv) {

    // Comprobar si los agumentos son validos
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr,"mi_mkfs.c -- Los argumentos no son validos. Uso: mi_mkfs <nombre_disp> <nbloques>\n");
        return -1;
    }

    if(bmount(argv[1])==-1) {   // Abrimos el fichero de disco, si no existe se crea
        fprintf(stderr,"mi_mkf.c -- Error al montar el dispositivo\n");
        return -1;
    }; 

    unsigned char buffer[BLOCKSIZE]; //el buffer de memoria empleado puede ser un array de tipo unsigned char del tamaño de un bloque
    memset(buffer,'\0',BLOCKSIZE);  // Bloque vacío

    unsigned int nbloques = atoi(argv[2]);
    unsigned int ninodos = nbloques / 4;

    printf("Creación bloques vacios, ahora escribo\n");

    // Inicializamos a 0s el fichero empleado como dispositivo virtual
    for(int i = 0; i < nbloques; i++) {
        if(bwrite(i,buffer)==-1) {
            fprintf(stderr,"mi_mkfs.c -- Error al escribir en el dispositivo\n");
            return -1;
        }
    }

    printf("Escritura bloques vacíos completada.\n");

    //Inicialización de los datos del superbloque
    initSB(nbloques, ninodos);
    printf("InitSB completado.\n");
    //Inicialización del mapa de bits (todos a 0)
    initMB();
    printf("InitMB completado.\n");
    //Creación de la lista enlazada de inodos
    initAI();
    printf("InitAI completado.\n");
    
    //Creaciòn del directorio raiz
    reservar_inodo('d', 7);
    printf("Creación directorio raíz completada.\n");

    if(bumount()==-1){
        fprintf(stderr,"Error al desmontar el dispositivo\n");
        return -1;
    };
      // Se desmonta el fichero
    printf("FS desmontado.\n");

    return 0;

}