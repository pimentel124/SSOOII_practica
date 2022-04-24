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
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    if (descriptor < 0) {
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
    if (close(descriptor) < 0) {
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
    int nbytes;
    if (lseek(descriptor, BLOCKSIZE * nbloque, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }

    if ((nbytes = write(descriptor, buf, BLOCKSIZE)) < 0) {

        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return nbytes;
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
    int nbytes;
    if (lseek(descriptor, BLOCKSIZE * nbloque, SEEK_SET) < 0) {

        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    if ((nbytes = read(descriptor, buf, BLOCKSIZE)) < 0) {
        
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return nbytes;
}
