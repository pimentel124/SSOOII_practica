/**
 * @file ficheros.c
 * @author Álvaro Pimentel, Andreu Marqués
 *
 */
#include "ficheros.h"

/**
 * @brief Escribe el contenido procedente de un buffer de memoria, buf_original, de tamaño nbytes,
 * en un fichero/directorio (correspondiente al inodo pasado como argumento, ninodo): le
 * indicamos la posición de escritura inicial en bytes lógicos, offset, con respecto al inodo, y
 * el número de bytes, nbytes, que hay que escribir.
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param buf_original  Buffer de memoria con el contenido a escribir
 * @param offset        Posición de escritura inicial en bytes lógicos
 * @param nbytes        Número de bytes a escribir
 * @return int          Devuelve el nbytes, o -1 en caso de error
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico, nBytesWrite = 0, auxBytesWritten = 0;
    char buf_bloque[BLOCKSIZE];
    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error en mi_write_f(), no existen permisos de escritura\npermisos inodo: %d  ||  %d: %s\n", (inodo.permisos & 2), errno, strerror(errno));
        return -1;
    }
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    if (nbfisico == -1) {
        return -1;
    }
    // Leemos el bloque fisico
    if (bread(nbfisico, buf_bloque) == -1) {
        return -1;
    }

    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1) {
            return -1;
        }
        nBytesWrite += nbytes;

    } else if (primerBL < ultimoBL) {
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
        if ((auxBytesWritten = bwrite(nbfisico, buf_bloque)) == -1) {
            return -1;
        }

        nBytesWrite += auxBytesWritten - desp1;

        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 1);
            if ((auxBytesWritten = bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE)) == -1) {
                return -1;
            }
            nBytesWrite += auxBytesWritten;
        }

        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (bread(nbfisico, buf_bloque) == -1) {
            return -1;
        }

        memcpy(buf_bloque, buf_original + (nbytes - desp2 - 1), desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == -1) {
            return -1;
        }
        nBytesWrite += desp2 + 1;
    }
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }

    if (inodo.tamEnBytesLog < (nbytes + offset)) {
        inodo.ctime = time(NULL);
        inodo.tamEnBytesLog = offset + nbytes;
    }
    inodo.mtime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1) {
        return -1;
    }
    if (nBytesWrite != nbytes) {
        return -1;
    }
    return nBytesWrite;
}

/**
 * @brief Lee información de un fichero/directorio (correspondiente al nº de inodo, ninodo, pasado
 * como argumento) y la almacena en un buffer de memoria, buf_original: le indicamos la
 * posición de lectura inicial offset con respecto al inodo (en bytes) y el número de bytes
 * nbytes que hay que leer.
 *
 * @param ninodo            Número de inodo del fichero/directorio
 * @param buf_original      Buffer de memoria donde se almacena la información leída
 * @param offset            Posición de lectura inicial en bytes lógicos
 * @param nbytes            Número de bytes a leer
 * @return int              Devuelve leidos, o -1 en caso de error
 *
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned int primerBL, ultimoBL, desp1, desp2, bytesleidos = 0;
    unsigned char buf_bloque[BLOCKSIZE];
    int nbfisico;
    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    if ((inodo.permisos & 4) != 4) {  // Esta operación sólo está permitida cuando haya permiso de lectura

        fprintf(stderr, "Error en mi_read_f()\nNo existen permisos de lectura, permisos del inodo: %d ||  %d: %s\n", inodo.permisos, errno, strerror(errno));
        return -1;
    }

    if (offset >= inodo.tamEnBytesLog) {
        return bytesleidos;  // No podemos leer nada
    }
    if ((offset + nbytes) >= inodo.tamEnBytesLog) {  // pretende leer más allá de EOF
        nbytes = inodo.tamEnBytesLog - offset;       // leemos sólo los bytes que podemos desde el offset hasta EOF
    }

    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    memset(buf_bloque, 0, sizeof(buf_bloque));
    nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, nbytes);
            bytesleidos = nbytes;
        }
       
    } else if (primerBL < ultimoBL) {  // Más de un bloque involucrado

        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                fprintf(stderr, "Error en lectura --> Error en ficheros_basico.c");
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        }
        bytesleidos = BLOCKSIZE - desp1;
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 0);
            if (nbfisico != -1) {
                if (bread(nbfisico, buf_bloque) == -1) {
                    return -1;
                }
                memcpy(buf_original + bytesleidos, buf_bloque, BLOCKSIZE);
            }
            bytesleidos += BLOCKSIZE;
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                return -1;
            }
            memcpy(buf_original + (nbytes - desp2 - 1), buf_bloque, desp2 + 1);
        }
        bytesleidos += desp2 + 1;
    }
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    // Se actualizan los metadatos

    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1) {
        return -1;
    }
    if (nbytes != bytesleidos) {
        return -1;
    }
    return bytesleidos;
}

/**
 * @brief Devuelve la metainformación de un fichero/directorio (correspondiente al nº de inodo
 * pasado como argumento): tipo, permisos, cantidad de enlaces de entradas en directorio,
 * tamaño en bytes lógicos, timestamps y cantidad de bloques ocupados en la zona de
 * datos, es decir todos los campos menos los punteros
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param STAT          Puntero a estructura de tipo STAT que almacena la información
 * @return int          Devuelve 0 si todo va bien, o -1 en caso de error
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

/**
 * @brief Cambia los permisos de un fichero/directorio (correspondiente al nº de inodo pasado
 * como argumento, ninodo) con el valor que indique el argumento permisos
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param permisos      Nuevos permisos
 * @return int          Devuelve 0 si todo va bien, o -1 en caso de error
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1) {
        fprintf(stderr, "Error en ficheros.c --> error al escribir inodo \n");
        return -1;
    }
    return 0;
}

/**
 * @brief Trunca un fichero/directorio (correspondiente al nº de inodo, ninodo, pasado
 * como argumento) a los bytes indicados como nbytes, liberando los bloques necesarios
 *
 * @param ninodo    Número de inodo del fichero/directorio
 * @param nbytes    Número de bytes a los que se quiere truncar
 * @return int      Devuelve la cantidad de bloques liberados o -1 si hay un error o la operación no está permitida.
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;
    unsigned int nblogico = 0;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    // Miramos si tenemos permisos
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error en ficheros.c mi_truncar_f() --> Permisos denegados de escritura\n");
        return -1;
    }

    if (nbytes > inodo.tamEnBytesLog) {
        //fprintf(stderr, "Error en ficheros.c mi_truncar_f() nbytes --> %d: \n", nbytes);
        return -1;
    }
    // Calculamos nblogico
    if (nbytes % BLOCKSIZE == 0) {
        nblogico = nbytes / BLOCKSIZE;
    } else {
        nblogico = nbytes / BLOCKSIZE + 1;
    }
    int bloq_Liberados = liberar_bloques_inodo(nblogico, &inodo);
    inodo.numBloquesOcupados -= bloq_Liberados;
    // Actualizamos mtime, ctime y tamEnBytesLog
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    inodo.tamEnBytesLog = nbytes;
    if (escribir_inodo(ninodo, inodo) == -1) {
        fprintf(stderr, "Error en ficheros.c mi_truncar_f() escribir inodo --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return bloq_Liberados;
}
