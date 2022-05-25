#include "bloques.h"

//----------------------------------------------------------------
static int descriptor = 0;

/**
 * @brief   Monta el sistema de ficheros en el dispositivo
 *
 * @param camino    Ruta del fichero de disco
 * @return int      Devuelve el descriptor del fichero de disco o -1 en caso de error
 */
int bmount(const char *camino) {
    umask(000);
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    if (descriptor == -1) {
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return descriptor;
}

/**
 * @brief   Desmonta el sistema de ficheros del dispositivo
 *
 * @return int    Devuelve 0 en caso de éxito o -1 en caso de error
 */
int bumount() {
    descriptor = close(descriptor);
    if (descriptor == -1) {
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief Escribe un bloque en el dispositivo virtual, en el bloque físico especificado por nbloque
 *
 * @param nbloque   Número de bloque físico
 * @param buf       Puntero al buffer con los datos a escribir
 * @return int      Devuelve nbytes en caso de éxito o -1 en caso de error
 */
int bwrite(unsigned int nbloque, const void *buf) {
    
    if (lseek(descriptor,nbloque * BLOCKSIZE, SEEK_SET) == -1) {
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }

    else{
        int escritos = write(descriptor, buf, BLOCKSIZE);
        if (escritos == -1){
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
        }
        return escritos;
    }
    
}

/**
 * @brief Lee 1 bloque del dispositivo virtual, que se corresponde con el bloque físico especificado
 * por nbloque
 *
 * @param nbloque   Número de bloque físico
 * @param buf       Puntero al buffer donde se guardarán los datos leídos
 * @return int      Devuelve nbytes en caso de éxito o -1 en caso de error
 */
int bread(unsigned int nbloque, void *buf) {
    int bloqueLogico;
    if (lseek(descriptor, BLOCKSIZE * nbloque, SEEK_SET) == -1) {
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    else
    {
        int bloqueLogico = read(descriptor, buf, BLOCKSIZE);
        if (bloqueLogico == -1) {
            fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
            return -1;
        }
        return bloqueLogico;
    }
    
}

