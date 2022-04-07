#include "ficheros.h"

/************************************************** NIVEL 6 **************************************************/

/*
Escribe el contenido procedente de un buffer de memoria, buf_original, de tamaño nbytes,
en un fichero/directorio (correspondiente al inodo pasado como argumento, ninodo): le
indicamos la posición de escritura inicial en bytes lógicos, offset, con respecto al inodo, y
el número de bytes, nbytes, que hay que escribir.
*/
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico, desp2;
    char buf_bloque[BLOCKSIZE];
    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == -1){
        return -1;
    }
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "ERROR no existen permisos de escritura %d: %s\n", errno, strerror(errno));
        return -1;
    }
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;
    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (bread(nbfisico, buf_bloque) == -1){
            return -1;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1){
            return -1;
        }
    } else {  // Más de un bloque involucrado
    
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (bread(nbfisico, buf_bloque) == -1){
            return -1;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1){
            return -1;
        }
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 1);
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE) == -1){
                return -1;
            }
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (bread(nbfisico, buf_bloque) == -1){
            return -1;
        }
        
        memcpy(buf_bloque, buf_original + (nbytes - desp2 - 1), desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == -1){
            return -1;
        }
    }
    if (leer_inodo(ninodo, &inodo) == -1){
        return -1;
    }
    inodo.mtime = time(NULL);
    if ((offset + nbytes) >= inodo.tamEnBytesLog) {
        inodo.ctime = time(NULL);
        inodo.tamEnBytesLog = offset + nbytes;
    }
    if (escribir_inodo(ninodo, inodo) == -1){
        return -1;
    }
    return nbytes;
}
/*  Lee información de un fichero/directorio (correspondiente al nº de inodo, ninodo, pasado
como argumento) y la almacena en un buffer de memoria, buf_original: le indicamos la
posición de lectura inicial offset con respecto al inodo (en bytes) y el número de bytes
nbytes que hay que leer.
*/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    
    unsigned int primerBL, ultimoBL, desp1, desp2, leidos, desp2;
    char buf_bloque[BLOCKSIZE];
    int nbfisico;
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1){
        return -1;
    }
    if ((inodo.permisos & 4) != 4) {  //Esta operación sólo está permitida cuando haya permiso de lectura
        fprintf(stderr, "ERROR no existen permisos de lectura %d: %s\n", errno, strerror(errno));
        return -1;
    }
    if (offset >= inodo.tamEnBytesLog) {
        leidos = 0;
        return leidos;  // No podemos leer nada
    }
    if ((offset + nbytes) >= inodo.tamEnBytesLog) { //pretende leer más allá de EOF
        nbytes = inodo.tamEnBytesLog - offset;  // leemos sólo los bytes que podemos desde el offset hasta EOF
    }
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;
    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1){
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        }
        leidos = nbytes;
    } else  {// Más de un bloque involucrado
    
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1){
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        }
        leidos = BLOCKSIZE - desp1;
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 0);
            if (nbfisico != -1) {
                if (bread(nbfisico, buf_bloque) == -1){
                    return -1;
                }
                memcpy(buf_original + leidos, buf_bloque, BLOCKSIZE);
            }
            leidos += BLOCKSIZE;
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1){
                return -1;
            }
            memcpy(buf_original + leidos, buf_bloque, desp2 + 1);
        }
        leidos += desp2 + 1;
    }
    if (leer_inodo(ninodo, &inodo) == -1){
        return -1;
    }
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1){
        return -1;
    }
    return leidos;
}

/* 
Devuelve la metainformación de un fichero/directorio (correspondiente al nº de inodo
pasado como argumento): tipo, permisos, cantidad de enlaces de entradas en directorio,
tamaño en bytes lógicos, timestamps y cantidad de bloques ocupados en la zona de
datos, es decir todos los campos menos los punteros
*/
int mi_stat_f(unsigned int ninodo, struct STAT *STAT) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    STAT->tipo = inodo.tipo;
    STAT->permisos = inodo.permisos;
    STAT->atime = inodo.atime;
    STAT->mtime = inodo.mtime;
    STAT->ctime = inodo.ctime;
    STAT->nlinks = inodo.nlinks;
    STAT->tamEnBytesLog = inodo.tamEnBytesLog;
    STAT->numBloquesOcupados = inodo.numBloquesOcupados;

    return 0;
}

/*  
Cambia los permisos de un fichero/directorio (correspondiente al nº de inodo pasado
como argumento, ninodo) con el valor que indique el argumento permisos
*/
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1){
        return -1;
    }
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1){
        return -1;
    }
    return 0;
}

/*  -Función: mi_truncar_f.
    -Descripción: Trunca un fichero/directorio a los bytes indicados.
    -Parámetros: Inodo del fichero/directorio, número de bytes a los que se quiere truncar.
    -Return: Devuelve la cantidad de bloques liberados o -1 si hay un error o la operación no está permitida.
*/
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    int nblogico, liberados;
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1)
        return -1;
    if (nbytes > inodo.tamEnBytesLog) {
        fprintf(stderr, "No se puede truncar más allá del EOF: %d\n", inodo.tamEnBytesLog);
        return -1;
    }
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "ERROR: permiso denegado de escritura\n");
        return -1;
    }
    if ((nbytes % BLOCKSIZE) == 0) {
        nblogico = nbytes / BLOCKSIZE;
    } else {
        nblogico = nbytes / BLOCKSIZE + 1;
    }
    liberados = liberar_bloques_inodo(ninodo, nblogico);
    if (liberados == -1)
        return -1;
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= liberados;
    if (escribir_inodo(ninodo, inodo) == -1)
        return -1;
    return liberados;
}

/****************************************** FUNCIONES AUXILIARES **************************************************/

/*	-Función: datos_inodo.
        -Descripción: Imprime los valores de un inodo.
        -Parámetros: Inodo que se va a mostrar
        -Return:
*/
void datos_inodo(struct inodo inodo) {
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    printf("atime: %s\n", atime);
    printf("ctime: %s\n", ctime);
    printf("mtime: %s\n", mtime);
    printf("nlinks: %u\n", inodo.nlinks);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", inodo.numBloquesOcupados);
}

/*	-Función: datos_STAT.
        -Descripción: Imprime los valores de un inodo.
        -Parámetros: STAT del inodo que se va a mostrar.
        -Return:
*/
void datos_STAT(struct STAT STAT) {
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&STAT.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&STAT.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&STAT.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    fprintf(stderr, "tipo=%c\n", STAT.tipo);
    fprintf(stderr, "permisos=%d\n", STAT.permisos);
    fprintf(stderr, "atime: %s\n", atime);
    fprintf(stderr, "ctime: %s\n", ctime);
    fprintf(stderr, "mtime: %s\n", mtime);
    fprintf(stderr, "nlinks=%u\n", STAT.nlinks);
    fprintf(stderr, "tamEnBytesLog=%u\n", STAT.tamEnBytesLog);
    fprintf(stderr, "numBloquesOcupados=%u\n\n", STAT.numBloquesOcupados);
}