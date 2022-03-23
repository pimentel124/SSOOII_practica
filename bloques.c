#include "bloques.h"

//----------------------------------------------------------------
static int descriptor = 0;

int bmount(const char *camino) {
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    if (descriptor < 0) {
        ERROR;
        return -1;
    }
    return descriptor;
}

int bumount() {
    if (close(descriptor) < 0) {
        ERROR;
        return -1;
    }
    return 0;
}

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

int bread(unsigned int nbloque, void *buf) {
    int nbytes;
    if (lseek(descriptor, BLOCKSIZE * nbloque, SEEK_SET) < 0) {

        ERROR;
        return -1;
    }
    if ((nbytes = read(descriptor, buf, BLOCKSIZE)) < 0) {
        
        ERROR;
        return -1;
    }
    return nbytes;
}
